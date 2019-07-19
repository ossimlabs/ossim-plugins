//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: OSSIM Kakadu based jp2 writer class definition.
//
//----------------------------------------------------------------------------
// $Id: ossimKakaduJp2Writer.cpp 23121 2015-01-30 01:24:56Z dburken $

#include "ossimKakaduJp2Writer.h"
#include "ossimKakaduCommon.h"
#include "ossimKakaduCompressor.h"
#include "ossimKakaduMessaging.h"

#include <ossim/base/ossimDate.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimEndian.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimPreferences.h>
#include <ossim/base/ossimTrace.h>

#include <ossim/projection/ossimMapProjection.h>

#include <ossim/imaging/ossimImageData.h>
#include <ossim/imaging/ossimImageGeometry.h>
#include <ossim/imaging/ossimImageSource.h>

#include <ossim/support_data/ossimGeoTiff.h>

#include <jp2.h>
#include <ostream>

using namespace std;

static const ossimIpt DEFAULT_TILE_SIZE(1024, 1024);

RTTI_DEF1(ossimKakaduJp2Writer, "ossimKakaduJp2Writer", ossimImageFileWriter)

//---
// For trace debugging (to enable at runtime do:
// your_app -T "ossimKakaduJp2Writer:debug" your_app_args
//---
static const ossimTrace traceDebug( ossimString("ossimKakaduJp2Writer:debug") );

//---
// For the "ident" program which will find all exanded:
// $Id: ossimKakaduJp2Writer.cpp 23121 2015-01-30 01:24:56Z dburken $
//---
#if OSSIM_ID_ENABLED
static const char OSSIM_ID[] = "$Id: ossimKakaduJp2Writer.cpp 23121 2015-01-30 01:24:56Z dburken $";
#endif

ossimKakaduJp2Writer::ossimKakaduJp2Writer()
   : ossimImageFileWriter(),
     m_compressor(new ossimKakaduCompressor()),
     m_outputStream(0),
     m_ownsStreamFlag(false)
{
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimKakaduJp2Writer::ossimKakaduJp2Writer entered"
         << std::endl;
#if OSSIM_ID_ENABLED
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "OSSIM_ID:  "
         << OSSIM_ID
         << std::endl;
#endif
   }

   // Set the output image type in the base class.
   setOutputImageType(getShortName());
}

ossimKakaduJp2Writer::~ossimKakaduJp2Writer()
{
   // This will flush stream and delete it if we own it.
   close();

   if (m_compressor)
   {
      delete m_compressor;
      m_compressor = 0;
   }
}

ossimString ossimKakaduJp2Writer::getShortName() const
{
   return ossimString("ossim_kakadu_jp2");
}

ossimString ossimKakaduJp2Writer::getLongName() const
{
   return ossimString("ossim kakadu jp2 writer");
}

ossimString ossimKakaduJp2Writer::getClassName() const
{
   return ossimString("ossimKakaduJp2Writer");
}

bool ossimKakaduJp2Writer::writeFile()
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

bool ossimKakaduJp2Writer::writeStream()
{
   static const char MODULE[] = "ossimKakaduJp2Writer::writeStream";

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
                              numberOfTiles,
                              true);
      }
      catch (const ossimException& e)
      {
         ossimNotify(ossimNotifyLevel_WARN) << e.what() << std::endl;
         return false;
      } 

      writeGeotiffBox( m_compressor );
      writeGmlBox( m_compressor );

      m_compressor->openJp2Codestream();

      bool needAlpha = m_compressor->getAlphaChannelFlag();

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

      m_compressor->finish();

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

bool ossimKakaduJp2Writer::writeGeotiffBox(ossimKakaduCompressor* compressor)
{
   bool result = false;
   
   if ( theInputConnection.valid() && compressor )
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
         
         result = compressor->writeGeotiffBox(geom.get(), rect, tmpFile, getPixelType());
      }
   }

   return result;
   
} // End: ossimKakaduJp2Writer::writeGeotffBox

bool ossimKakaduJp2Writer::writeGmlBox(ossimKakaduCompressor* compressor)
{
   bool result = false;
   
   if ( theInputConnection.valid() && compressor )
   {
      ossimRefPtr<ossimImageGeometry> geom = theInputConnection->getImageGeometry();
      if ( geom.valid() )
      {
         // Output rect.
         ossimIrect rect = theInputConnection->getBoundingRect();

         result = compressor->writeGmlBox( geom.get(), rect );
      }
   }

   return result;
   
} // End: ossimKakaduJp2Writer::writeGmlBox

bool ossimKakaduJp2Writer::saveState(ossimKeywordlist& kwl,
                                      const char* prefix)const
{
   m_compressor->saveState(kwl, prefix);
   
   return ossimImageFileWriter::saveState(kwl, prefix);
}

bool ossimKakaduJp2Writer::loadState(const ossimKeywordlist& kwl,
                                      const char* prefix)
{
   m_compressor->loadState(kwl, prefix);
   
   return ossimImageFileWriter::loadState(kwl, prefix);
}

bool ossimKakaduJp2Writer::isOpen() const
{
   return (m_outputStream?true:false);
}

bool ossimKakaduJp2Writer::open()
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
         << "ossimKakaduJp2Writer::open()\n"
         << "File " << theFilename << (result ? " opened" : " not opened")
         << std::endl;
   }

   return result;
}

void ossimKakaduJp2Writer::close()
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

void ossimKakaduJp2Writer::getImageTypeList(std::vector<ossimString>& imageTypeList)const
{
   imageTypeList.push_back(getShortName());
}

ossimString ossimKakaduJp2Writer::getExtension() const
{
   return ossimString("jp2");
}

bool ossimKakaduJp2Writer::getOutputHasInternalOverviews( void ) const
{ 
   return true;
}

bool ossimKakaduJp2Writer::hasImageType(const ossimString& imageType) const
{
   bool result = false;
   if ( (imageType == getShortName()) ||
        (imageType == "image/jp2") )
   {
      result = true;
   }
   return result;
}

void ossimKakaduJp2Writer::setProperty(ossimRefPtr<ossimProperty> property)
{
   if ( property.valid() )
   {
      if ( m_compressor->setProperty(property) == false )
      {
         // Not a compressor property.
         ossimImageFileWriter::setProperty(property);
      }
   }
}

ossimRefPtr<ossimProperty> ossimKakaduJp2Writer::getProperty(
   const ossimString& name)const
{
   ossimRefPtr<ossimProperty> p = m_compressor->getProperty(name);
   
   if ( !p )
   {
      p = ossimImageFileWriter::getProperty(name);
   }
   
   return p;
}

void ossimKakaduJp2Writer::getPropertyNames(
   std::vector<ossimString>& propertyNames)const
{
   m_compressor->getPropertyNames(propertyNames);
   
   ossimImageFileWriter::getPropertyNames(propertyNames);
}

bool ossimKakaduJp2Writer::setOutputStream(std::ostream& stream)
{
   if (m_ownsStreamFlag && m_outputStream)
   {
      delete m_outputStream;
   }
   m_outputStream = &stream;
   m_ownsStreamFlag = false;
   return true;
}
