//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  David Burken
//
// Description:
//
// Class definition for JPEG2000 (J2K) reader.
//
//----------------------------------------------------------------------------
// $Id: ossimKakaduJ2kReader.cpp 23209 2015-03-30 01:01:36Z dburken $

#include "ossimKakaduJ2kReader.h"
#include "ossimKakaduCommon.h"
#include "ossimKakaduMessaging.h"

#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimEndian.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimPreferences.h>
#include <ossim/base/ossimScalarTypeLut.h>
#include <ossim/base/ossimTrace.h>
 
#include <ossim/imaging/ossimTiffTileSource.h>
#include <ossim/imaging/ossimImageDataFactory.h>

#include <ossim/support_data/ossimJ2kSizRecord.h>
#include <ossim/support_data/ossimJ2kSotRecord.h>

#include <kdu_sample_processing.h>
#include <kdu_region_decompressor.h>
#include <kdu_threads.h>

#include <fstream>
#include <iostream>
 
 
#ifdef OSSIM_ID_ENABLED
static const char OSSIM_ID[] = "$Id";
#endif
 
static const ossimTrace traceDebug( ossimString("ossimKakaduJ2kReader:debug") );
static const ossimTrace traceDump( ossimString("ossimKakaduJ2kReader:dump" ) );

RTTI_DEF1_INST(ossimKakaduJ2kReader,
               "ossimKakaduJ2kReader",
               ossimImageHandler)
 
ossimKakaduJ2kReader::ossimKakaduJ2kReader()
   : ossimImageHandler(),
     theCodestream(),
     theThreadEnv(0),
     theOpenTileThreadQueue(0),
     theMinDwtLevels(0),
     theFileStr(),
     theSizRecord(),
     theScalarType(OSSIM_SCALAR_UNKNOWN),
     theImageRect(),
     theTile(),
     theCacheTile(),
     theTileSizeX(0),
     theTileSizeY(0),
     theCacheId(-1)
{
   kdu_customize_warnings(&pretty_cout); // Deliver warnings to stdout.
   kdu_customize_errors(&pretty_cerr); // Deliver errors to stderr + throw exc
}

ossimKakaduJ2kReader::~ossimKakaduJ2kReader()
{
   closeEntry();
}

ossimString ossimKakaduJ2kReader::getShortName()const
{
   return ossimString("ossim_kakadu_j2k_reader");
}

ossimString ossimKakaduJ2kReader::getLongName()const
{
   return ossimString("ossim kakadu j2k reader");
}

ossimString ossimKakaduJ2kReader::getClassName()const
{
   return ossimString("ossimKakaduJ2kReader");
}

void ossimKakaduJ2kReader::getDecimationFactor(ossim_uint32 resLevel,
                                               ossimDpt& result) const
{
   if (resLevel == 0)
   {
      //---
      // Assumption r0 or first layer is full res.  Might need to change to
      // use nitf IMAG field.
      //---
      result.x = 1.0;
      result.y = 1.0;
   }
   else if ( theOverview.valid() && (resLevel > theMinDwtLevels) &&
             (resLevel < getNumberOfDecimationLevels()) )
   {
      //---
      // External overview file.
      // 
      // Use the real lines and samples in case an resLevel is skipped.
      //
      // Note we must subtract the internal overviews as the external
      // overview reader does not know about them.
      //---
      ossim_float64 r0 = getNumberOfSamples(0);
      ossim_float64 rL =
         theOverview->getNumberOfSamples(resLevel-theMinDwtLevels);
      
      if (r0) // Divide by zero check
      {
         result.x = rL/r0;
      }
      else
      {
         result.x = ossim::nan();
      }
      r0 = getNumberOfLines(0);
      rL = theOverview->getNumberOfLines(resLevel-theMinDwtLevels);
      
      if (r0) // Divide by zero check.
      {
         result.y = rL/r0;
      }
      else
      {
         result.y = ossim::nan();
      }
   }
   else
   {
      // Internal overviews are on power of two decimation.
      result.x = 1.0 / pow((double)2, (double)resLevel);
      result.y = result.x;
   }
}

void ossimKakaduJ2kReader::getDecimationFactors(
   vector<ossimDpt>& decimations) const
{
   const ossim_uint32 LEVELS = getNumberOfDecimationLevels();
   decimations.resize(LEVELS);
   for (ossim_uint32 level = 0; level < LEVELS; ++level)
   {
      getDecimationFactor(level, decimations[level]);
   }
}

ossim_uint32 ossimKakaduJ2kReader::getNumberOfDecimationLevels()const
{
   ossim_uint32 result = 1; // Add r0

   if (theMinDwtLevels)
   {
      //---
      // Add internal overviews.
      //---
      result += theMinDwtLevels;
   }

   if (theOverview.valid())
   {
      //---
      // Add external overviews.
      //
      // NOTE: The ossimTiffTileSource will count r0 if present or it will
      //       add 1 to the decimation count if r0 is NOT present. Since
      //       since r0 has already be added subtract one.
      //---
      result += theOverview->getNumberOfDecimationLevels()-1;
   }

   return result;
}

ossim_uint32 ossimKakaduJ2kReader::getNumberOfLines(
   ossim_uint32 resLevel) const
{
   ossim_uint32 result = 0;
   if (resLevel < getNumberOfDecimationLevels())
   {
      result = theSizRecord.m_Ysiz;
      if ( resLevel > 0 )
      {
         ossimDpt dpt;
         getDecimationFactor(resLevel, dpt);
         if ( !dpt.hasNans() )
         {
            result = static_cast<ossim_uint32>(result * dpt.y);
         }
      }
   }
   return result;
}

ossim_uint32 ossimKakaduJ2kReader::getNumberOfSamples(
   ossim_uint32 resLevel) const
{
   ossim_uint32 result = 0;
   if (resLevel < getNumberOfDecimationLevels())
   {
      result = theSizRecord.m_Xsiz;
      if ( resLevel > 0 )
      {
         ossimDpt dpt;
         getDecimationFactor(resLevel, dpt);
         result = static_cast<ossim_uint32>(result * dpt.x);
      }
   }
   return result;
}

bool ossimKakaduJ2kReader::open()
{
   static const char MODULE[] = "ossimKakaduJ2kReader::open";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " entered...\n";
   }
   
   bool result = false;
   
   if(isOpen())
   {
      closeEntry();
   }

   // Open up a stream to the file.
   theFileStr.open(theImageFile.c_str(), ios::in | ios::binary);
   if ( theFileStr.good() )
   {
      //---
      // Check for the Start Of Codestream (SOC) and Size (SIZ) markers which
      // are required as first and second fields in the main header.
      //---
      ossim_uint16 soc;
      ossim_uint16 siz;
      theFileStr.read((char*)&soc,  2);
      theFileStr.read((char*)&siz,  2);
      
      if (ossim::byteOrder() == OSSIM_LITTLE_ENDIAN) // Alway big endian.
      {
         ossimEndian().swap(soc);
         ossimEndian().swap(siz);
      }

      const ossim_uint16 SOC_MARKER = 0xff4f; // start of codestream marker
      const ossim_uint16 SIZ_MARKER = 0xff51; // size maker

      if ( (soc == SOC_MARKER) && (siz == SIZ_MARKER) )
      {
         // Read in and store the size record.
         theSizRecord.parseStream(theFileStr);

         // Position to start of code stream prior to create call.
         theFileStr.seekg(0);
 
         //---
         // Initialize the codestream.  The class ossimKakaduNitfReader is a
         // kdu_compressed source so we feed ourself to the codestream.
         //
         // TODO:  Currently no kdu_thread_env.  This should be implemented for
         // speed...
         //---
         
         //---
         // Construct multi-threaded processing environment if required.
         // Temp hard coded to a single thread.
         //---
         
         if (theThreadEnv)
         {
            theThreadEnv->terminate(NULL, true);
            theThreadEnv->destroy();
         }
         else
         {
            theThreadEnv = new kdu_core::kdu_thread_env();
         }
         
         theThreadEnv->create(); // Creates the single "owner" thread

         // Check for threads in prefs file.
         ossim_uint32 threads = 1;
         const char* lookup = ossimPreferences::instance()->findPreference("kakadu_threads");
         if ( lookup )
         {
            threads = ossimString::toUInt32(lookup);
            if ( threads > 1 )
            {
               for (ossim_uint32 nt=1; nt < threads; ++nt)
               {
                  if ( !theThreadEnv->add_thread() )
                  {
                     if (traceDebug())
                     {
                        ossimNotify(ossimNotifyLevel_WARN)
                           << "Unable to create thread!\n";
                     }
                  }
               }
            }
         }
         
         theOpenTileThreadQueue = theThreadEnv->add_queue(NULL,NULL,"open_tile_q");
         
         theCodestream.create(this, theThreadEnv);
         
         if ( theCodestream.exists() )
         {
            //---
            // We have to store things here in this non-const method because
            // NONE of the kakadu methods are const.
            //---
            theMinDwtLevels = theCodestream.get_min_dwt_levels();
            
            theCodestream.set_persistent(); // ????
            theCodestream.enable_restart(); // ????
            
            kdu_core::kdu_dims region_of_interest;
            region_of_interest.pos.x = 0;
            region_of_interest.pos.y = 0;
            region_of_interest.size.x = getNumberOfSamples(0);
            region_of_interest.size.y = getNumberOfLines(0);

            theCodestream.apply_input_restrictions(
               0, // first_component
               0, // max_components (0 = all remaining will appear)
               0, // highest resolution level
               0, // max_layers (0 = all layers retained)
               &region_of_interest, // expanded out to block boundary.
               //KDU_WANT_CODESTREAM_COMPONENTS);
               kdu_core::KDU_WANT_OUTPUT_COMPONENTS);
            
            // Set the scalar:
            theScalarType = theSizRecord.getScalarType();
            if (theScalarType != OSSIM_SCALAR_UNKNOWN)
            {
               //---
               // NOTE: Please leave commented out code for now.
               //---
               // Capture the sub image offset.
               // theSubImageOffset.x = theSizRecord.theXOsiz;
               // theSubImageOffset.y = theSizRecord.theYOsiz;
               
               // Initialize the image rect.
               theImageRect = ossimIrect(0,
                                         0,
                                         theSizRecord.m_Xsiz-1,
                                         theSizRecord.m_Ysiz-1);

               // Initialize the cache.
               if (theCacheId != -1)
               {
                  ossimAppFixedTileCache::instance()->deleteCache(theCacheId);
                  theCacheId = -1;
               }
               ossimIpt tileSize(theSizRecord.m_XTsiz, theSizRecord.m_YTsiz);

               // Stretch to tile boundary for the cache.
               ossimIrect fullImgRect = theImageRect;
               fullImgRect.stretchToTileBoundary(tileSize);
               
               // Set up the tile cache.
               theCacheId = ossimAppFixedTileCache::instance()->
                  newTileCache(fullImgRect, tileSize);

               // Add the sub image rect after the 
               
               // Initialize the tile we will return.
               initializeTile();

               // Call the base complete open to pick up overviews.
               completeOpen();
               
               // We should be good now so set the return result to true.
               result = true;

               if (traceDebug())
               {
                  ossimNotify(ossimNotifyLevel_DEBUG)
                     << "\nSIZ marker segment"
                     << theSizRecord
                     << "theCodestream.get_num_components(false): "
                     << theCodestream.get_num_components(false)
                     << "\ntheCodestream.get_num_components(true): "
                     << theCodestream.get_num_components(true)
                     << "\ntheCodestream.get_bit_depth(0, true): "
                     << theCodestream.get_bit_depth(0, true)
                     << "\ntheCodestream.get_signed(0, true): "
                     << theCodestream.get_signed(0, true)
                     << "\ntheCodestream.get_min_dwt_levels(): "
                     << theCodestream.get_min_dwt_levels()
                     << "\ntheImageRect: " << theImageRect
                     << "\nFull image rect: " << fullImgRect
                     << "\nthreads: " << threads
                     << "\n";
                  
                  vector<ossimDpt> decimations;
                  getDecimationFactors(decimations);
                  for (ossim_uint32 i = 0; i < decimations.size(); ++i)
                  {
                     ossimNotify(ossimNotifyLevel_DEBUG)
                        << theCodestream.get_min_dwt_levels()
                        << "Decimation factor[" << i << "]: "
                        << decimations[i]
                        << "\nsamples[" << i << "]: "
                        << getNumberOfSamples(i)
                        << "\nlines[" << i << "]: "
                        << getNumberOfLines(i)
                        << std::endl;
                     
                  }
               }
            }
               
         } // matches: if ( theCodestream.exists() )
         
      } //  matches: if ( (soc == SOC_MARKER) && (siz == SIZ_MARKER) )
      
   } // matches: if ( theFileStr.good() )
   else
   {
      if(traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << MODULE << " ERROR:"
            << "\nCannot open:  " << theImageFile.c_str() << endl;
      }
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " exit status = " << (result?"true":"false\n")
         << std::endl;
   }

   return result;
}

bool ossimKakaduJ2kReader::isOpen()const
{
   // return theFileStr.is_open();

   // Temp fix for gcc's that don't have a const "ifstream::is_open() const"
   return theTile.valid();
}

void ossimKakaduJ2kReader::closeEntry()
{
   theFileStr.close();

   // Cleanup processing environment
   if ( theThreadEnv )
   {
      theThreadEnv->join(NULL,true); // Wait until all internal processing is complete.
      theThreadEnv->terminate(theOpenTileThreadQueue, true);
      theThreadEnv->cs_terminate(theCodestream);   // Terminates background codestream processing.
      theThreadEnv->destroy();
      delete theThreadEnv;
      theThreadEnv = 0;
   }
   
   if(theCodestream.exists())
   {
      theCodestream.destroy();
   }

   if (theOpenTileThreadQueue)
   {
      theOpenTileThreadQueue = 0;
   }

   if (theCacheId != -1)
   {
      ossimAppFixedTileCache::instance()->deleteCache(theCacheId);
      theCacheId = -1;
   }

   theTile      = 0;
   theCacheTile = 0;
   theTileSizeX = 0;
   theTileSizeY = 0;

   ossimImageHandler::close();
}

bool ossimKakaduJ2kReader::loadState(const ossimKeywordlist& kwl,
                                     const char* prefix)
{
   bool result = false;
   
   if ( ossimImageHandler::loadState(kwl, prefix) )
   {
      result = open();
   }
   
   return result;
}


ossimRefPtr<ossimImageData> ossimKakaduJ2kReader::getTile(
   const ossimIrect& rect, ossim_uint32 resLevel)
{
   // This tile source bypassed, or invalid res level, return a blank tile.
   if(!isSourceEnabled() || !isOpen() || !isValidRLevel(resLevel))
   {
      return ossimRefPtr<ossimImageData>();
   }

   if (theTile.valid())
   {
      // Rectangle must be set prior to getOverviewTile call.
      theTile->setImageRectangle(rect);

      if (resLevel)
      {
         if ( getOverviewTile(resLevel, theTile.get() ) == false )
         {
            theTile->makeBlank();
         }
      }
      else
      {
         //---
         // See if the whole tile is going to be filled, if not, start out with
         // a blank tile so data from a previous load gets wiped out.
         //---
         if ( !rect.completely_within(theImageRect) )
         {
            // Start with a blank tile.
            theTile->makeBlank();
         }
         
         //---
         // See if any point of the requested tile is in the image.
         //---
         if ( rect.intersects(theImageRect) )
         {
            ossimIrect clipRect = rect.clipToRect(theImageRect);

            ossimIrect exandedRect  = clipRect;

            //---
            // Shift the upper left corner of the "clip_rect" to the an even
            // j2k tile boundry.  
            //---
            exandedRect.stretchToTileBoundary(ossimIpt(theTileSizeX,
                                                       theTileSizeY));
            
            // Vertical tile loop.
            ossim_int32 y = exandedRect.ul().y;
            while (y < exandedRect.lr().y)
            {
               // Horizontal tile loop.
               ossim_int32 x = exandedRect.ul().x;
               while (x < exandedRect.lr().x)
               {
                  if ( loadTileFromCache(x, y, clipRect) == false )
                  {
                     if ( loadTile(x, y) )
                     {
                        //---
                        // Note: Clip the cache tile to the image clipRect
                        // since there are j2k tiles that go beyond the image
                        // dimensions, i.e., edge tiles.
                        //---    
                        ossimIrect cr =
                           theCacheTile->getImageRectangle().
                           clipToRect(clipRect);
                        
                        theTile->loadTile(theCacheTile->getBuf(),
                                          theCacheTile->getImageRectangle(),
                                          cr,
                                          OSSIM_BSQ);
                     }
                     
                  }
                  
                  x += theTileSizeX; // Go to next tile.
               }
               
               y += theTileSizeY; // Go to next row of tiles.
            }

            // Set the tile status.
            theTile->validate();
            
         } // matches: if ( rect.intersects(theImageRect) )
         
      } // r0 block
      
   } // matches: if (theTile.valid())

   return theTile;
}

bool ossimKakaduJ2kReader::getOverviewTile(ossim_uint32 resLevel,
                                           ossimImageData* result)
{
   bool status = false;

   if ( (resLevel < getNumberOfDecimationLevels()) && result &&
        (result->getNumberOfBands() == getNumberOfOutputBands()) )
   {
      if (resLevel <= theMinDwtLevels)
      {
         // Internal overviews...
         try
         {
            status = ossim::copyRegionToTile(theCodestream,
                                             static_cast<int>(resLevel),
                                             theThreadEnv,
                                             theOpenTileThreadQueue,
                                             result);
         }
         catch(const ossimException& e)
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << __FILE__ << " " << __LINE__ << " caught exception\n"
               << e.what();
            status = false;
         }

      } // matches:  if (resLevel <= theMinDwtLevels)
      else
      {
         // External overviews...
         status = theOverview->getTile(result, resLevel - theMinDwtLevels);
      }
   }

   return status;
}

ossim_uint32 ossimKakaduJ2kReader::getNumberOfInputBands() const
{
   return theSizRecord.m_Csiz;
}

ossim_uint32 ossimKakaduJ2kReader::getNumberOfOutputBands()const
{
   return theSizRecord.m_Csiz;
}

ossim_uint32 ossimKakaduJ2kReader::getImageTileWidth() const
{
   ossim_uint32 result = 0;
   if ( (theSizRecord.m_XTsiz <= 1024) && (theSizRecord.m_YTsiz <= 1024) )
   {
      result = theSizRecord.m_XTsiz;
   }
   return result;
}

ossim_uint32 ossimKakaduJ2kReader::getImageTileHeight() const
{
   ossim_uint32 result = 0;
   if ( (theSizRecord.m_XTsiz <= 1024) && (theSizRecord.m_YTsiz <= 1024) )
   {
      result = theSizRecord.m_YTsiz;
   }
   return result;
}

ossimScalarType ossimKakaduJ2kReader::getOutputScalarType() const
{
   return theSizRecord.getScalarType();
}
 
void ossimKakaduJ2kReader::initializeTile()
{
   theTileSizeX = this->getImageTileWidth();
   theTileSizeY = this->getImageTileHeight();

   // Check for zero width, height and limit output tile sizes to 1024.
   if ( !theTileSizeX || !theTileSizeY ||
        ( theTileSizeX > 1024) || (theTileSizeY > 1024) )
   {
      ossimIpt tileSize;
      ossim::defaultTileSize(tileSize);

      theTileSizeX = tileSize.x;
      theTileSizeY = tileSize.y;
   }
   
   theTile = ossimImageDataFactory::instance()->
      create( this,
              this->getOutputScalarType(),
              this->getNumberOfOutputBands(),
              theTileSizeX,
              theTileSizeY);

   theTile->initialize();

   theCacheTile = ossimImageDataFactory::instance()->
      create( this,
              this->getOutputScalarType(),
              this->getNumberOfOutputBands(),
              theTileSizeX,
              theTileSizeY);
   
   theCacheTile->initialize();
}

bool ossimKakaduJ2kReader::loadTileFromCache(ossim_uint32 x, ossim_uint32 y,
                                             const ossimIrect& clipRect)
{
   bool result = false;

   ossimIpt origin(x, y);

   ossimRefPtr<ossimImageData> tempTile =
      ossimAppFixedTileCache::instance()->getTile(theCacheId, origin);

   if (tempTile.valid())
   {
      //---
      // Note: Clip the cache j2k tile to the image clipRect since
      // there are j2k tiles that go beyond the image dimensions, i.e.,
      // edge tiles.
      //---
      ossimIrect cr =
         tempTile->getImageRectangle().clipToRect(clipRect);
      
      theTile->loadTile(tempTile.get()->getBuf(),
                        tempTile->getImageRectangle(),
                        cr,
                        OSSIM_BSQ);
      result = true;
   }
   return result;
}

bool ossimKakaduJ2kReader::loadTile(ossim_uint32 x, ossim_uint32 y)
{
   bool result = true;
   
   ossimIpt ul(x, y);
   ossimIpt lr(ul.x + theTileSizeX - 1,
               ul.y + theTileSizeY - 1);

   // Set the cache rectangle to be an even j2k tile.
   theCacheTile->setImageRectangle(ossimIrect(ul, lr));

   //---
   // Let the getOverviewTile do the rest of the work.
   if ( getOverviewTile(0, theCacheTile.get()) )
   {
      // Add it to the cache for the next time.
      ossimAppFixedTileCache::instance()->addTile(theCacheId, theCacheTile);
   }
   else
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << __FILE__ << __LINE__
         << " ossimKakaduJ2kReader::loadBlock failed!"
         << std::endl;
      result = false;
   }
   
   return result;
}
