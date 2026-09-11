//---
//
// License: MIT
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: OSSIM Kakadu based nitf writer.
//
//---
// $Id$

#include "ossimKakaduNitfWriter.h"
#include "ossimKakaduCommon.h"
#include "ossimKakaduCompressor.h"
#include "ossimKakaduKeywords.h"
#include "ossimKakaduMessaging.h"

#include <ossim/base/ossimDate.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimEndian.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimNumericProperty.h>
#include <ossim/base/ossimProperty.h>
#include <ossim/base/ossimStringProperty.h>
#include <ossim/base/ossimTrace.h>

#include <ossim/projection/ossimMapProjection.h>

#include <ossim/imaging/ossimImageData.h>
#include <ossim/imaging/ossimImageSource.h>

#include <ossim/support_data/ossimNitfDesInformation.h>
#include <ossim/support_data/ossimNitfCommon.h>
#include <ossim/support_data/ossimNitfFileHeader.h>
#include <ossim/support_data/ossimNitfFileHeaderV2_1.h>
#include <ossim/support_data/ossimNitfImageHeader.h>
#include <ossim/support_data/ossimNitfImageHeaderV2_1.h>
#include <ossim/support_data/ossimNitfJ2klraTag.h>

#include <ostream>
#include <vector>

static const ossimIpt DEFAULT_TILE_SIZE(1024, 1024);

RTTI_DEF1(ossimKakaduNitfWriter, "ossimKakaduNitfWriter", ossimNitfWriterBase)

//---
// For trace debugging (to enable at runtime do:
// your_app -T "ossimKakaduNitfWriter:debug" your_app_args
//---
static const ossimTrace traceDebug( ossimString("ossimKakaduNitfWriter:debug") );

//---
// For the "ident" program which will find all exanded $Id: ossimKakaduNitfWriter.cpp 22111 2013-01-12 18:44:25Z dburken $
// them.
//---
#if OSSIM_ID_ENABLED
static const char OSSIM_ID[] = "$Id: ossimKakaduNitfWriter.cpp 22111 2013-01-12 18:44:25Z dburken $";
#endif

ossimKakaduNitfWriter::ossimKakaduNitfWriter()
   : ossimNitfWriterBase(),
     m_fileHeader(new ossimNitfFileHeaderV2_1),
     m_imageHeader(new ossimNitfImageHeaderV2_1),
     m_dataExtensionSegments(0),
     m_compressor(new ossimKakaduCompressor()),
     m_outputStream(0),
     m_ownsStreamFlag(false),
     m_blockSize(DEFAULT_TILE_SIZE.x, DEFAULT_TILE_SIZE.y)
{
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimKakaduNitfWriter::ossimKakaduNitfWriter entered"
         << std::endl;
#if OSSIM_ID_ENABLED
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "OSSIM_ID:  "
         << OSSIM_ID
         << std::endl;
#endif
   }

   //---
   // Since the internal nitf tags are not very accurate, write an external
   // geometry out as default behavior.  Users can disable this via the
   // property interface or keyword list.
   //
   // Update: External geom shut off.
   // You can use --writer-prop "create_external_geometry=1" to enable via command
   // line apps, e.g. ossim-chipper
   // drb 22 Oct. 2025
   //---
   // setWriteExternalGeometryFlag(true);

   // Set the output image type in the base class.
   setOutputImageType(getShortName());

   // Set any site defaults.
   initializeDefaultsFromConfigFile(
      dynamic_cast<ossimNitfFileHeaderV2_X*>(m_fileHeader.get()),
      dynamic_cast<ossimNitfImageHeaderV2_X*>(m_imageHeader.get()) );
}

ossimKakaduNitfWriter::~ossimKakaduNitfWriter()
{
   // This will flush stream and delete it if we own it.
   close();

   if (m_compressor)
   {
      delete m_compressor;
      m_compressor = 0;
   }
}

ossimString ossimKakaduNitfWriter::getShortName() const
{
   return ossimString("ossim_kakadu_nitf_j2k");
}

ossimString ossimKakaduNitfWriter::getLongName() const
{
   return ossimString("ossim kakadu nitf j2k writer");
}

ossimString ossimKakaduNitfWriter::getClassName() const
{
   return ossimString("ossimKakaduNitfWriter");
}

bool ossimKakaduNitfWriter::writeFile()
{
   // This method is called from ossimImageFileWriter::execute().

   bool result = false;
   
   if( theInputConnection.valid() &&
       (getErrorStatus() == ossimErrorCodes::OSSIM_OK) )
   {
      // Set the tile size for all processes.
      theInputConnection->setTileSize( m_blockSize );
      theInputConnection->setToStartOfSequence();
      
      //---
      // Note only the master process used for writing...
      //---
      if(theInputConnection->isMaster())
      {
         if (!isOpen())
         {
            open();
         }
         
         if ( isOpen() )
         {
            result = writeStream();
         }
      }
      else // Slave process.
      {
         // This will return after all tiles for this node have been processed.
         theInputConnection->slaveProcessTiles();

         result = true;
      }
   }
      
   return result;
}

bool ossimKakaduNitfWriter::writeStream()
{
   static const char MODULE[] = "ossimKakaduNitfWriter::writeStream";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   if ( !theInputConnection || !m_outputStream || !theInputConnection->isMaster() )
   {
      return false;
   }

   const ossim_uint32    TILES  = theInputConnection->getNumberOfTiles();
   const ossim_uint32    BANDS  = theInputConnection->getNumberOfOutputBands();
   const ossimScalarType SCALAR = theInputConnection->getOutputScalarType();
   
   if (m_compressor->getAlphaChannelFlag())
   {
      //--- 
      // Someone can set this through the generic setProperty interface.
      // Unset, currently only supported in jp2 writer.
      // Could be used here but I think we would have to update the
      // nitf tags.
      //---
      m_compressor->setAlphaChannelFlag(false);
   }

   // Create the compressor.  Can through an exception.
   try
   {
      m_compressor->create(m_outputStream,
                           SCALAR,
                           BANDS,
                           theInputConnection->getAreaOfInterest(),
                           m_blockSize,
                           TILES,
                           false);
   }
   catch (const ossimException& e)
   {
      ossimNotify(ossimNotifyLevel_WARN) << e.what() << std::endl;
      return false;
   }

   std::streampos endOfFileHdrPos;
   std::streampos endOfImgHdrPos;
   std::streampos endOfImgPos;
   std::streampos endOfFilePos;
   
   // Container record withing NITF file header.
   ossimNitfImageInfoRecordV2_1 imageInfoRecord;
   
   // Note the sub header length and image length will be set later.      
   m_fileHeader->addImageInfoRecord(imageInfoRecord);
   m_fileHeader->setDate(ossimDate());
   m_fileHeader->setTitle(ossimString("")); // ???

   //---
   // Apply anything set through setFileHeaderProperty() and
   // setImageHeaderProperty().  Both headers are serialised below -- the file
   // header on the next line -- so this is the last point at which a property
   // can still reach the output.
   //---
   addFileHeaderProperties( m_fileHeader.get() );
   
   // Write to stream capturing the stream position for later.
   m_fileHeader->writeStream(*m_outputStream);
   endOfFileHdrPos = m_outputStream->tellp();
   
   // Set the compression type:
   m_imageHeader->setCompression(ossimString("C8"));
   
   // Set the Image Magnification (IMAG) field.
   m_imageHeader->setImageMagnification(ossimString("1.0"));
   
   // Set the pixel type (PVTYPE) field.
   m_imageHeader->setPixelType(ossimNitfCommon::getNitfPixelType(SCALAR));
   
   // Set the actual bits per pixel (ABPP) field.
   ossim_uint32 abpp = ossim::getActualBitsPerPixel(SCALAR);
   m_imageHeader->setActualBitsPerPixel( abpp );
   
   // Set the bits per pixel (NBPP) field.
   m_imageHeader->setBitsPerPixel(ossim::getBitsPerPixel(SCALAR));
   
   m_imageHeader->setNumberOfBands(BANDS);
   m_imageHeader->setImageMode('B'); // IMODE field to blocked.
   
   if( (BANDS == 3) && (SCALAR == OSSIM_UCHAR) )
   {
      m_imageHeader->setRepresentation("RGB");
      m_imageHeader->setCategory("VIS");
   }
   else if(BANDS == 1)
   {
      m_imageHeader->setRepresentation("MONO");
      m_imageHeader->setCategory("MS");
   }
   else
   {
      m_imageHeader->setRepresentation("MULTI");
      m_imageHeader->setCategory("MS");
   }
   
   ossimNitfImageBandV2_1 bandInfo;
   for(ossim_uint32 band = 0; band < BANDS; ++band)
   {
      std::ostringstream out;
      
      out << std::setfill('0')
          << std::setw(2)
          << band;
      
      bandInfo.setBandRepresentation(out.str().c_str());
      m_imageHeader->setBandInfo(band, bandInfo);
   }
   
   ossim_uint32 outputTilesWide = theInputConnection->getNumberOfTilesHorizontal();
   ossim_uint32 outputTilesHigh = theInputConnection->getNumberOfTilesVertical();
   
   m_imageHeader->setBlocksPerRow(outputTilesWide);
   m_imageHeader->setBlocksPerCol(outputTilesHigh);
   m_imageHeader->setNumberOfPixelsPerBlockRow(m_blockSize.y);
   m_imageHeader->setNumberOfPixelsPerBlockCol(m_blockSize.x);
   m_imageHeader->setNumberOfRows(theInputConnection->getAreaOfInterest().height());
   m_imageHeader->setNumberOfCols(theInputConnection->getAreaOfInterest().width());
   
   // Write the geometry info to the image header.
   writeGeometry(m_imageHeader.get(), theInputConnection.get());

   // Add the J2KLRA TRE:
   ossimRefPtr<ossimNitfJ2klraTag> j2klraTag = new ossimNitfJ2klraTag();
   m_compressor->initialize( j2klraTag.get(), abpp );
   j2klraTag->setBandsO( BANDS );
   ossimRefPtr<ossimNitfRegisteredTag> tag = j2klraTag.get();
   ossimNitfTagInformation tagInfo( tag );
   m_imageHeader->addTag( tagInfo );
   
   // Write the image header to stream capturing the stream position.
   //---
   // Caller properties are an OVERRIDE, so they are applied here -- after
   // the defaults above and immediately before serialisation -- not
   // earlier.  A writer cannot know the spectral nature of a single band
   // and defaults ICAT accordingly; an explicit ICAT from the caller has
   // to win over that guess.
   //---
   addImageHeaderProperties( m_imageHeader.get() );
   m_imageHeader->writeStream(*m_outputStream);
   endOfImgHdrPos = m_outputStream->tellp();
   
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " DEBUG:"
         << "\noutputTilesWide:  " << outputTilesWide
         << "\noutputTilesHigh:  " << outputTilesHigh
         << "\nnumberOfTiles:    " << TILES
         << "\nimageRect: " << theInputConnection->getAreaOfInterest()
         << "\n";
   }
   
   // Tile loop in the line direction.
   ossim_uint32 tileNumber = 0;
   bool result = true;
   for(ossim_uint32 y = 0; y < outputTilesHigh; ++y)
   {
      // Tile loop in the sample (width) direction.
      for(ossim_uint32 x = 0; x < outputTilesWide; ++x)
      {
         // Grab the resampled tile.
         ossimRefPtr<ossimImageData> t = theInputConnection->getNextTile();
         if (t.valid() && ( t->getDataObjectStatus() != OSSIM_NULL ) )
         {
            if ( ! m_compressor->writeTile( *(t.get()) ) )
            {
               ossimNotify(ossimNotifyLevel_WARN)
                  << MODULE << " ERROR:"
                  << "Error returned writing tile:  "
                  << tileNumber
                  << std::endl;
               result = false;
            }
         }
         else
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << MODULE << " ERROR:"
               << "Error returned writing tile:  " << tileNumber
               << std::endl;
            result = false;
         }
         if ( !result )
         {
            // This will bust out of both loops.
            x = outputTilesWide;
            y = outputTilesHigh;
         }
         
         // Increment tile number for percent complete.
         ++tileNumber;
         
      } // End of tile loop in the sample (width) direction.

      //---
      // Flush on each row.
      // NOTE: Calling flush() before a full row was complete was causing
      // bad blocks, only on the first row.
      //
      // NOTE: If only one tile, let the flush() in finish() do it. Getting
      // white tile doing here. drb - 20190109 vs7_A_6
      //---
      if ( result && (TILES > 1) )
      {
         if ( m_compressor->flush() == false )
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << MODULE << " ERROR:"
               << "Error on flush! Current tile number:  " << tileNumber
               << std::endl;
            result = false;
         }
      }
      
      if (needsAborting())
      {
         setPercentComplete(100.0);
         break;
      }
      else
      {
         setPercentComplete((ossim_float64)tileNumber/(ossim_float64)TILES * 100.0);
      }
      
   } // End of tile loop in the line (height) direction.
         
   m_compressor->finish();

   endOfImgPos = m_outputStream->tellp();
   
   if (m_fileHeader->getDesInfoList().size())
   {
      //---
      // Write out the Data Extension Segments(DES):
      // getDesInfoList() is a std::vector<ossimNitfDesInformation>&
      //---
      auto& v = m_fileHeader->getDesInfoList();
      for ( auto&& i : v )
      {
         i.writeStream(*m_outputStream);
      }
   }
   
   // Get the file length.
   endOfFilePos = m_outputStream->tellp();
   
   //---
   // Seek back to set some things that were not know until now and
   // rewrite the nitf file and image header.
   //---
   m_outputStream->seekp(0, std::ios_base::beg);
   
   // Set the file length.
   std::streamoff length = endOfFilePos;
   m_fileHeader->setFileLength(static_cast<ossim_uint64>(length));
   
   // Set the file header length.
   length = endOfFileHdrPos;
   m_fileHeader->setHeaderLength(static_cast<ossim_uint64>(length));            

   // Set the image sub header length.
   length = endOfImgHdrPos - endOfFileHdrPos;
   imageInfoRecord.setSubheaderLength(static_cast<ossim_uint64>(length));
   
   // Set the image length.
   length = endOfImgPos - endOfImgHdrPos;
   imageInfoRecord.setImageLength(static_cast<ossim_uint64>(length));
   
   m_fileHeader->replaceImageInfoRecord(0, imageInfoRecord);
   
   setComplexityLevel(length, m_fileHeader.get());
   
   // Rewrite the header.
   m_fileHeader->writeStream(*m_outputStream);
   
   // Set the compression rate now that the image size is known.
   ossimString comrat = ossimNitfCommon::getCompressionRate(
      theInputConnection->getAreaOfInterest(),
      BANDS,
      SCALAR,
      static_cast<ossim_uint64>(length));
   m_imageHeader->setCompressionRateCode(comrat);
   
   // Rewrite the image header.
   m_imageHeader->writeStream(*m_outputStream);

   close();

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " exit status = " << (result?"true":"false\n")
         << std::endl;
   }
   
   return result;
}

bool ossimKakaduNitfWriter::saveState(ossimKeywordlist& kwl,
                                      const char* prefix)const
{
   m_compressor->saveState(kwl, prefix);
   
   return ossimNitfWriterBase::saveState(kwl, prefix);
}

bool ossimKakaduNitfWriter::loadState(const ossimKeywordlist& kwl,
                                      const char* prefix)
{
   m_compressor->loadState(kwl, prefix);
   
   return ossimNitfWriterBase::loadState(kwl, prefix);
}

bool ossimKakaduNitfWriter::isOpen() const
{
   return (m_outputStream) ? true : false;
}

bool ossimKakaduNitfWriter::open()
{
   bool result = false;
   
   close();

   // Check for empty filenames.
   if (theFilename.size())
   {
      std::ofstream* os = new std::ofstream();
      os->open(theFilename.c_str(), std::ios::out | std::ios::binary);
      if(os->is_open())
      {
         m_outputStream = os;
         m_ownsStreamFlag = true;
         result = true;
      }
      else
      {
         delete os;
         os = 0;
      }
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimKakaduNitfWriter::open()\n"
         << "File " << theFilename << (result ? " opened" : " not opened")
         << "\n";
    }

   return result;
}

void ossimKakaduNitfWriter::close()
{
   if (m_outputStream)      
   {
      m_outputStream->flush();

      if (m_ownsStreamFlag)
      {
         delete m_outputStream;
         m_outputStream = 0;
         m_ownsStreamFlag = false;
      }
   }
}

ossimString ossimKakaduNitfWriter::getExtension() const
{
   return ossimString("ntf");
}

void ossimKakaduNitfWriter::addRegisteredTag(ossimRefPtr<ossimNitfRegisteredTag> registeredTag,
   bool unique, const ossim_uint32& ownerIndex, const ossimString& tagType)
{
   ossimNitfTagInformation tagInfo;
   tagInfo.setTagData(registeredTag.get());
   tagInfo.setTagType(tagType);

   switch (ownerIndex)
   {
      case 0:
      {
         m_fileHeader->addTag(tagInfo, unique);
         break;
      }

      case 1:
      {
         m_imageHeader->addTag(tagInfo, unique);
         break;
      }

      default:
      {
         // Do nothing
      }
   }
}

void ossimKakaduNitfWriter::addDesInfo(const ossimNitfDesInformation& des)
{
   m_fileHeader->addDes(des);
}

bool ossimKakaduNitfWriter::getOutputHasInternalOverviews( void ) const
{ 
   return true;
}

void ossimKakaduNitfWriter::getImageTypeList(std::vector<ossimString>& imageTypeList)const
{
   imageTypeList.push_back(getShortName());
}

bool ossimKakaduNitfWriter::hasImageType(const ossimString& imageType) const
{
   bool result = false;
   if ( (imageType == getShortName()) ||
        (imageType == "image/ntf") )
   {
      result = true;
   }
   return result;
}

void ossimKakaduNitfWriter::setProperty(ossimRefPtr<ossimProperty> property)
{
   if ( property.valid() )
   {
      if( property->getName() == "block_size" )
      {
         ossimIpt blockSize;
         blockSize.x = property->valueToString().toInt32();
         blockSize.y = blockSize.x;
         setTileSize(blockSize);
      }
      else if ( m_compressor->setProperty(property) == false )
      {
         // Not a compressor property.
         ossimNitfWriterBase::setProperty(property);
      }
   }   
}

ossimRefPtr<ossimProperty> ossimKakaduNitfWriter::getProperty(
   const ossimString& name)const
{
   ossimRefPtr<ossimProperty> p = m_compressor->getProperty(name);
   if ( !p )
   {
      if(name == "block_size")
      {
         ossimRefPtr<ossimStringProperty> stringProp =
            new ossimStringProperty(name,
                                    ossimString::toString(m_blockSize.x),
                                    false); // editable flag
         // stringProp->addConstraint(ossimString("128"));
         stringProp->addConstraint(ossimString("256"));      
         stringProp->addConstraint(ossimString("512"));      
         stringProp->addConstraint(ossimString("1024"));      
         p = stringProp.get();
      }
      else
      {
         p = ossimNitfWriterBase::getProperty(name);
      }
   }
   
   return p;
}

void ossimKakaduNitfWriter::getPropertyNames(
   std::vector<ossimString>& propertyNames)const
{
   m_compressor->getPropertyNames(propertyNames);
   ossimNitfWriterBase::getPropertyNames(propertyNames);
   propertyNames.push_back("block_size");
}

bool ossimKakaduNitfWriter::setOutputStream(std::ostream& stream)
{
   if (m_ownsStreamFlag && m_outputStream)
   {
      delete m_outputStream;
   }
   m_outputStream = &stream;
   m_ownsStreamFlag = false;
   return true;
}

void ossimKakaduNitfWriter::setTileSize(const ossimIpt& tileSize)
{
   if ( (tileSize.x == 256 || tileSize.x == 512 || tileSize.x == 1024) &&
        (tileSize.x == tileSize.y) )
   {
      m_blockSize = tileSize;
   }
   else //  if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << "ossimKakaduNitfWriter::setTileSize WARNING!"
         << "\nInvalid block size: " << tileSize
         << "\nBlock size constrained to 256, 512 or 1024 and square."
         << "\nSize remains: " << m_blockSize
         << std::endl;
   }
}
