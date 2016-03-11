//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: OSSIM Kakadu based nitf writer.
//
//----------------------------------------------------------------------------
// $Id: ossimKakaduNitfWriter.cpp 22111 2013-01-12 18:44:25Z dburken $

#include "ossimKakaduNitfWriter.h"
#include "ossimKakaduCommon.h"
#include "ossimKakaduCompressor.h"
#include "ossimKakaduKeywords.h"
#include "ossimKakaduMessaging.h"

#include <ossim/base/ossimBooleanProperty.h>
#include <ossim/base/ossimDate.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimEndian.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimNumericProperty.h>
#include <ossim/base/ossimTrace.h>

#include <ossim/projection/ossimMapProjection.h>

#include <ossim/imaging/ossimImageData.h>
#include <ossim/imaging/ossimImageSource.h>

#include <ossim/support_data/ossimNitfCommon.h>
#include <ossim/support_data/ossimNitfFileHeader.h>
#include <ossim/support_data/ossimNitfFileHeaderV2_1.h>
#include <ossim/support_data/ossimNitfImageHeader.h>
#include <ossim/support_data/ossimNitfImageHeaderV2_1.h>
#include <ossim/support_data/ossimNitfJ2klraTag.h>

#include <ostream>

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
     m_compressor(new ossimKakaduCompressor()),
     m_outputStream(0),
     m_ownsStreamFlag(false)
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
   //---
   setWriteExternalGeometryFlag(true);

   // Set the output image type in the base class.
   setOutputImageType(getShortName());
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
      theInputConnection->setTileSize( DEFAULT_TILE_SIZE );
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
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " entered..." << endl;
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
                           DEFAULT_TILE_SIZE,
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
   std::streampos endOfFilePos;
   
   // Container record withing NITF file header.
   ossimNitfImageInfoRecordV2_1 imageInfoRecord;
   
   // NITF file header.
   ossimRefPtr<ossimNitfFileHeaderV2_1> fHdr = new ossimNitfFileHeaderV2_1();
   
   // Note the sub header length and image length will be set later.      
   fHdr->addImageInfoRecord(imageInfoRecord);
   
   fHdr->setDate(ossimDate());
   fHdr->setTitle(ossimString("")); // ???
   
   // Write to stream capturing the stream position for later.
   fHdr->writeStream(*m_outputStream);
   endOfFileHdrPos = m_outputStream->tellp();
   
   // NITF image header.
   ossimRefPtr<ossimNitfImageHeaderV2_1> iHdr = new ossimNitfImageHeaderV2_1();
   
   // Set the compression type:
   iHdr->setCompression(ossimString("C8"));
   
   // Set the Image Magnification (IMAG) field.
   iHdr->setImageMagnification(ossimString("1.0"));
   
   // Set the pixel type (PVTYPE) field.
   iHdr->setPixelType(ossimNitfCommon::getNitfPixelType(SCALAR));
   
   // Set the actual bits per pixel (ABPP) field.
   iHdr->setActualBitsPerPixel(ossim::getActualBitsPerPixel(SCALAR));
   
   // Set the bits per pixel (NBPP) field.
   iHdr->setBitsPerPixel(ossim::getBitsPerPixel(SCALAR));
   
   iHdr->setNumberOfBands(BANDS);
   iHdr->setImageMode('B'); // IMODE field to blocked.
   
   if( (BANDS == 3) && (SCALAR == OSSIM_UCHAR) )
   {
      iHdr->setRepresentation("RGB");
      iHdr->setCategory("VIS");
   }
   else if(BANDS == 1)
   {
      iHdr->setRepresentation("MONO");
      iHdr->setCategory("MS");
   }
   else
   {
      iHdr->setRepresentation("MULTI");
      iHdr->setCategory("MS");
   }
   
   ossimNitfImageBandV2_1 bandInfo;
   for(ossim_uint32 band = 0; band < BANDS; ++band)
   {
      std::ostringstream out;
      
      out << std::setfill('0')
          << std::setw(2)
          << band;
      
      bandInfo.setBandRepresentation(out.str().c_str());
      iHdr->setBandInfo(band, bandInfo);
   }
   
   ossim_uint32 outputTilesWide = theInputConnection->getNumberOfTilesHorizontal();
   ossim_uint32 outputTilesHigh = theInputConnection->getNumberOfTilesVertical();
   
   iHdr->setBlocksPerRow(outputTilesWide);
   iHdr->setBlocksPerCol(outputTilesHigh);
   iHdr->setNumberOfPixelsPerBlockRow(DEFAULT_TILE_SIZE.y);
   iHdr->setNumberOfPixelsPerBlockCol(DEFAULT_TILE_SIZE.x);
   iHdr->setNumberOfRows(theInputConnection->getAreaOfInterest().height());
   iHdr->setNumberOfCols(theInputConnection->getAreaOfInterest().width());
   
   // Write the geometry info to the image header.
   writeGeometry(iHdr.get(), theInputConnection.get());

#if 0
   // Add the J2KLRA TRE:
   ossimRefPtr<ossimNitfJ2klraTag> j2klraTag = new ossimNitfJ2klraTag();
   m_compressor->initialize( j2klraTag.get() );
   j2klraTag->setBandsO( BANDS );
   ossimRefPtr<ossimNitfRegisteredTag> tag = j2klraTag.get();
   ossimNitfTagInformation tagInfo( tag );
   iHdr->addTag( tagInfo );
#endif
   
   // Write the image header to stream capturing the stream position.
   iHdr->writeStream(*m_outputStream);
   endOfImgHdrPos = m_outputStream->tellp();
   
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " DEBUG:"
         << "\noutputTilesWide:  " << outputTilesWide
         << "\noutputTilesHigh:  " << outputTilesHigh
         << "\nnumberOfTiles:    " << TILES
         << "\nimageRect: " << theInputConnection->getAreaOfInterest()
         << std::endl;
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
      
      if (needsAborting())
      {
         setPercentComplete(100.0);
         break;
      }
      else
      {
         ossim_float64 tile = tileNumber;
         ossim_float64 numTiles = TILES;
         setPercentComplete(tile / numTiles * 100.0);
      }
      
   } // End of tile loop in the line (height) direction.
         
   m_compressor->finish();
   
   // Get the file length.
   endOfFilePos = m_outputStream->tellp();
   
   //---
   // Seek back to set some things that were not know until now and
   // rewrite the nitf file and image header.
   //---
   m_outputStream->seekp(0, std::ios_base::beg);
   
   // Set the file length.
   std::streamoff length = endOfFilePos;
   fHdr->setFileLength(static_cast<ossim_uint64>(length));
   
   // Set the file header length.
   length = endOfFileHdrPos;
   fHdr->setHeaderLength(static_cast<ossim_uint64>(length));            
   // Set the image sub header length.
   length = endOfImgHdrPos - endOfFileHdrPos;
   
   imageInfoRecord.setSubheaderLength(static_cast<ossim_uint64>(length));
   
   // Set the image length.
   length = endOfFilePos - endOfImgHdrPos;
   imageInfoRecord.setImageLength(static_cast<ossim_uint64>(length));
   
   fHdr->replaceImageInfoRecord(0, imageInfoRecord);
   
   setComplexityLevel(length, fHdr.get());
   
   // Rewrite the header.
   fHdr->writeStream(*m_outputStream);
   
   // Set the compression rate now that the image size is known.
   ossimString comrat = ossimNitfCommon::getCompressionRate(
      theInputConnection->getAreaOfInterest(),
      BANDS,
      SCALAR,
      static_cast<ossim_uint64>(length));
   iHdr->setCompressionRateCode(comrat);
   
   // Rewrite the image header.
   iHdr->writeStream(*m_outputStream);

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
      os->open(theFilename.c_str(), ios::out | ios::binary);
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
         << std::endl;
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
      if ( m_compressor->setProperty(property) == false )
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
      p = ossimNitfWriterBase::getProperty(name);
   }
   
   return p;
}

void ossimKakaduNitfWriter::getPropertyNames(
   std::vector<ossimString>& propertyNames)const
{
   m_compressor->getPropertyNames(propertyNames);

   ossimNitfWriterBase::getPropertyNames(propertyNames);
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
