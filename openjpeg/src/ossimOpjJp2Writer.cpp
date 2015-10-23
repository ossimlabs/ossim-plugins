//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: OSSIM Open JPEG writer.
//
//----------------------------------------------------------------------------
// $Id: ossimOpjJp2Writer.cpp 11652 2007-08-24 17:14:15Z dburken $

#include "ossimOpjJp2Writer.h"
#include "ossimOpjCompressor.h"

#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimEndian.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimIoStream.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimNumericProperty.h>
#include <ossim/base/ossimScalarTypeLut.h>
#include <ossim/base/ossimStringProperty.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/projection/ossimMapProjection.h>
#include <ossim/imaging/ossimImageData.h>
#include <ossim/imaging/ossimImageSource.h>
#include <ossim/imaging/ossimScalarRemapper.h>
#include <ossim/support_data/ossimJp2Info.h>

#include <ctime>

RTTI_DEF1(ossimOpjJp2Writer,
	  "ossimOpjJp2Writer",
	  ossimImageFileWriter)

//---
// For trace debugging (to enable at runtime do:
// your_app -T "ossimOpjJp2Writer:debug" your_app_args
//---
static ossimTrace traceDebug("ossimOpjJp2Writer:debug");

static const ossimIpt DEFAULT_TILE_SIZE(1024, 1024);

//---
// For the "ident" program which will find all expanded $Id$
// them.
//---
#if OSSIM_ID_ENABLED
static const char OSSIM_ID[] = "$Id$";
#endif

ossimOpjJp2Writer::ossimOpjJp2Writer()
   : ossimImageFileWriter(),
     m_outputStream(0),
     m_ownsStreamFlag(false),
     m_compressor(new ossimOpjCompressor()),
     m_do_geojp2(true),
     m_do_gmljp2(true)
{
   //---
   // Uncomment for debug mode:
   // traceDebug.setTraceFlag(true);
   //---

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimOpjJp2Writer::ossimOpjJp2Writer entered" << std::endl;
#if OSSIM_ID_ENABLED
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "OSSIM_ID:  "
         << OSSIM_ID
         << std::endl;
#endif
   }

   // Since there is no internal geometry set the flag to write out one.
   // setWriteExternalGeometryFlag(true);

   // Set the output image type in the base class.
   setOutputImageType(getShortName());
}

ossimOpjJp2Writer::ossimOpjJp2Writer( const ossimString& typeName )
   : ossimImageFileWriter(),
     m_outputStream(0),
     m_ownsStreamFlag(false),
     m_compressor(new ossimOpjCompressor()),
     m_do_geojp2(typeName.contains("ossim_opj_geojp2")),
     m_do_gmljp2(typeName.contains("ossim_opj_gmljp2"))
{
   //---
   // Uncomment for debug mode:
   // traceDebug.setTraceFlag(true);
   //---

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimOpjJp2Writer::ossimOpjJp2Writer entered" << std::endl;
#if OSSIM_ID_ENABLED
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "OSSIM_ID:  "
         << OSSIM_ID
         << std::endl;
#endif
   }

   // Since there is no internal geometry set the flag to write out one.
   // setWriteExternalGeometryFlag(true);

   // Set the output image type in the base class.
   setOutputImageType(getShortName());

   // If typeName is unrecognized, write out both geotiff and gmljp2 headers
   if ( !m_do_geojp2 && !m_do_gmljp2 )
   {
       m_do_geojp2 = true;
       m_do_gmljp2 = true;
   }
}

ossimOpjJp2Writer::~ossimOpjJp2Writer()
{
   // This will flush stream and delete it if we own it.
   close();
}

ossimString ossimOpjJp2Writer::getShortName() const
{
   return ossimString("ossim_opj_jp2");
}

ossimString ossimOpjJp2Writer::getLongName() const
{
   return ossimString("ossim open jpeg writer");
}

ossimString ossimOpjJp2Writer::getClassName() const
{
   return ossimString("ossimOpjJp2Writer");
}

bool ossimOpjJp2Writer::writeFile()
{
   // This method is called from ossimImageFileWriter::execute().

   bool result = false;
   
   if( theInputConnection.valid() &&
       (getErrorStatus() == ossimErrorCodes::OSSIM_OK) )
   {

      // Make sure Area of Interest is an even multiple of tiles
      ossimIrect areaOfInterest = theInputConnection->getAreaOfInterest();
      ossimIpt imageSize(areaOfInterest.size());
      ossimIpt imageLr(areaOfInterest.lr());
      ossim_uint32 xBoundaryAdjustFactor = DEFAULT_TILE_SIZE.x - (imageSize.x % DEFAULT_TILE_SIZE.x);
      ossim_uint32 yBoundaryAdjustFactor = DEFAULT_TILE_SIZE.y - (imageSize.y % DEFAULT_TILE_SIZE.y);
      imageLr.x += xBoundaryAdjustFactor;
      imageLr.y += yBoundaryAdjustFactor;
      areaOfInterest.set_lr(imageLr); 
      theInputConnection->setAreaOfInterest(areaOfInterest);

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
   
} // End: ossimOpjJp2Writer::writeFile()

bool ossimOpjJp2Writer::writeStream()
{
   static const char MODULE[] = "ossimOpjJp2Writer::write";
   
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " entered..." << endl;
   }
   
   bool result = false;
   
   if (theInputConnection.valid() && theInputConnection->isMaster() &&
       m_outputStream )
   {
      result = true; // Assuming good at this point...
      
      ossim_uint32 outputTilesWide =
         theInputConnection->getNumberOfTilesHorizontal();
      ossim_uint32 outputTilesHigh =
         theInputConnection->getNumberOfTilesVertical();
      ossim_uint32 numberOfTiles =
         theInputConnection->getNumberOfTiles();
      ossim_uint32 tileNumber = 0;
      
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << MODULE << " DEBUG:"
            << "\noutputTilesWide:  " << outputTilesWide
            << "\noutputTilesHigh:  " << outputTilesHigh
            << "\nnumberOfTiles:    " << numberOfTiles
            << "\nimageRect: " << theInputConnection->getAreaOfInterest()
            << std::endl;
      }

      ossimScalarType scalarType = theInputConnection->getOutputScalarType();

      const ossim_uint32 BANDS = theInputConnection->getNumberOfOutputBands();

      // Create the compressor.  Can through an exception.
      try
      {
         m_compressor->create(m_outputStream,
                              scalarType,
                              BANDS,
                              theInputConnection->getAreaOfInterest(),
                              DEFAULT_TILE_SIZE,
                              true);
      }
      catch (const ossimException& e)
      {
         ossimNotify(ossimNotifyLevel_WARN) << e.what() << std::endl;
         return false;
      }

      m_compressor->openJp2Codestream();

      // Flush the stream since we're going to mess with it.
      m_outputStream->flush();

      // cout << "tellp 1: " << m_outputStream->tellp();

      // Back up to start of jp2c lbox.
      m_outputStream->seekp( -8, std::ios_base::cur );
      std::streamoff origJp2cBoxPos = m_outputStream->tellp();

      // cout << "origJp2cBoxPos: " << origJp2cBoxPos << endl;
      
      // Copy the lbox and tbox for jp2c (codestream).
      std::vector<ossim_uint8> jp2cHdr;
      copyData( origJp2cBoxPos, 8, jp2cHdr );

      // Write the geotiff and gml boxes:
      // Overwrite!!!  Supreme hack. (drb - 20150326)
      if ( m_do_geojp2 == true )
         writeGeotiffBox(m_outputStream, m_compressor);
      if ( m_do_gmljp2 == true )
         writeGmlBox(m_outputStream, m_compressor);
      m_outputStream->flush();

      std::streamoff newJp2cBoxPos = m_outputStream->tellp();
      // cout << "newJp2cBoxPos: " << newJp2cBoxPos << endl;

      // Write the jp2c hdr after the geotiff box:
      m_outputStream->write( (char*)&jp2cHdr, jp2cHdr.size() );

      // Copy the lbox and tbox for geotiff box
      std::vector<ossim_uint8> geotiffHdr;
      copyData( origJp2cBoxPos, 8, geotiffHdr );

      bool needAlpha = m_compressor->getAlphaChannelFlag();
      ossim_uint32 tileIndex = 0;
      
      // Tile loop in the line direction.
      for(ossim_uint32 y = 0; y < outputTilesHigh; ++y)
      {
         // Tile loop in the sample (width) direction.
         for(ossim_uint32 x = 0; x < outputTilesWide; ++x)
         {
            // Grab the resampled tile.
            ossimRefPtr<ossimImageData> t = theInputConnection->getNextTile();
            if (t.valid() && ( t->getDataObjectStatus() != OSSIM_NULL ) )
            {
               if (needAlpha)
               {
                  t->computeAlphaChannel();
               }
               if ( ! m_compressor->writeTile( t.get(), tileIndex++) )
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
            if (result == false)
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
            ossim_float64 numTiles = numberOfTiles;
            setPercentComplete(tile / numTiles * 100.0);
         }
         
      } // End of tile loop in the line (height) direction.

      if (m_outputStream)      
      {
         m_outputStream->flush();
      }
      
      m_compressor->finish();

      // Grab the jp2c hdr again in case the lbox changes.
      copyData( origJp2cBoxPos, 8, jp2cHdr );

      // Re-write the geotiff lbox and tbox:
      m_outputStream->seekp( origJp2cBoxPos, std::ios_base::beg );
      m_outputStream->write( (char*)&geotiffHdr.front(), geotiffHdr.size() );

      // Re-write the jp2c hdr at new position:
      m_outputStream->seekp( newJp2cBoxPos, std::ios_base::beg );
      m_outputStream->write( (char*)&jp2cHdr.front(), jp2cHdr.size() );

      m_outputStream->flush();
      close();

   } // matches: if (theInputConnection.valid() ...

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " exit status = " << (result?"true":"false\n")
         << std::endl;
   }
   
   return result;
}

bool ossimOpjJp2Writer::isOpen() const
{
   if (m_outputStream)
   {
     return true;
   }
   return false;
}

bool ossimOpjJp2Writer::open()
{
   bool result = false;
   
   close();

   // Check for empty filenames.
   if ( theFilename.size() )
   {
      std::ofstream* os = new std::ofstream();
      os->open(theFilename.c_str(), ios::out | ios::binary);
      if(os->is_open())
      {
         // cout << "opened " << theFilename << endl;
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
         << "ossimOpjJp2Writer::open()\n"
         << "File " << theFilename << (result ? " opened" : " not opened")
         << std::endl;
   }

   return result;
}

void ossimOpjJp2Writer::close()
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

bool ossimOpjJp2Writer::saveState(ossimKeywordlist& kwl,
                                const char* prefix)const
{
   return ossimImageFileWriter::saveState(kwl, prefix);
}

bool ossimOpjJp2Writer::loadState(const ossimKeywordlist& kwl,
                                const char* prefix)
{
   const char* value;
   
   value = kwl.find(prefix, ossimKeywordNames::OVERVIEW_FILE_KW);
   if(value)
   {
      m_overviewFlag = ossimString(value).toBool();
   }
   
   return ossimImageFileWriter::loadState(kwl, prefix);
}

void ossimOpjJp2Writer::setProperty(ossimRefPtr<ossimProperty> property)
{
   if (!property)
   {
      return;
   }
}

ossimRefPtr<ossimProperty> ossimOpjJp2Writer::getProperty(const ossimString& name)const
{
   return ossimImageFileWriter::getProperty(name);
}

void ossimOpjJp2Writer::getPropertyNames(std::vector<ossimString>& propertyNames)const
{
   ossimImageFileWriter::getPropertyNames(propertyNames);
}

void ossimOpjJp2Writer::getImageTypeList(std::vector<ossimString>& imageTypeList)const
{
   imageTypeList.push_back( getShortName() );
   imageTypeList.push_back( "ossim_opj_geojp2" );
   imageTypeList.push_back( "ossim_opj_gmljp2" );
}

ossimString ossimOpjJp2Writer::getExtension() const
{
   return ossimString("jp2");
}

bool ossimOpjJp2Writer::getOutputHasInternalOverviews( void ) const
{ 
   return true;
}

bool ossimOpjJp2Writer::hasImageType(const ossimString& imageType) const
{
   bool result = false; 
   if ( (imageType == getShortName()) ||
        (imageType == "image/jp2")    ||
        (imageType == "image/j2k")    ||
        (imageType == "ossim_opj_geojp2") ||
        (imageType == "ossim_opj_gmljp2") )
   {
      result = true;
   }
   return result;
}

bool ossimOpjJp2Writer::setOutputStream(std::ostream& stream)
{
   if (m_ownsStreamFlag && m_outputStream)
   {
      delete m_outputStream;
   }
   m_outputStream = &stream;
   m_ownsStreamFlag = false;
   return true;
}

bool ossimOpjJp2Writer::writeGeotiffBox(std::ostream* stream, ossimOpjCompressor* compressor)
{
   bool result = false;
   
   if ( theInputConnection.valid() && stream && compressor )
   {
      ossimRefPtr<ossimImageGeometry> geom = theInputConnection->getImageGeometry();
      if ( geom.valid() )
      {
         //---
         // Make a temp file.  No means currently write a tiff straight to
         // memory.
         //---
         ossimFilename tmpFile = theFilename.fileNoExtension();
         tmpFile += "-tmp.tif";
         
         // Output rect.
         ossimIrect rect = theInputConnection->getBoundingRect();
         
         result = compressor->writeGeotiffBox(stream, geom.get(), rect, tmpFile, getPixelType());
      }
   }

   return result;
   
} // End: ossimKakaduJp2Writer::writeGeotffBox

bool ossimOpjJp2Writer::writeGmlBox(std::ostream* stream, ossimOpjCompressor* compressor)
{
   bool result = false;
   
   if ( theInputConnection.valid() && stream && compressor )
   {
      ossimRefPtr<ossimImageGeometry> geom = theInputConnection->getImageGeometry();
      if ( geom.valid() )
      {
         // Output rect.
         ossimIrect rect = theInputConnection->getBoundingRect();
         
         result = compressor->writeGmlBox(stream, geom.get(), rect, getPixelType());
      }
   }

   return result;
   
} // End: ossimKakaduJp2Writer::writeGmlBox

void ossimOpjJp2Writer::copyData(
   const std::streamoff& pos , ossim_uint32 size, std::vector<ossim_uint8>& data ) const
{
   std::ifstream str;
   str.open( theFilename.c_str(), std::ios_base::in | std::ios_base::binary);
   if ( str.is_open() )
   {
      data.resize(size);
      str.seekg( pos, std::ios_base::beg );
      str.read( (char*)&data.front(), size );
   }
}
