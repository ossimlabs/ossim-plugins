//---
//
// License: MIT
//
// Author:  David Burken
//
// Description:
//
// Class definition for reader of NITF images with JPEG2000 (J2K) compressed
// blocks using kakadu library for decompression.  The image data segment
// can be raw J2K or have a JP2 wrapper.
//
//---
// $Id$

#include "ossimKakaduNitfReader.h"
#include "ossimKakaduCommon.h"
#include "ossimKakaduMessaging.h"
 
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimPreferences.h>
#include <ossim/base/ossimScalarTypeLut.h>
#include <ossim/base/ossimTrace.h>
 
#include <ossim/imaging/ossimTiffTileSource.h>
#include <ossim/imaging/ossimImageDataFactory.h>
 
#include <ossim/support_data/ossimNitfImageHeader.h>
#include <ossim/support_data/ossimJ2kCodRecord.h>
#include <ossim/support_data/ossimJ2kSizRecord.h>
#include <ossim/support_data/ossimJ2kSotRecord.h>
#include <ossim/support_data/ossimJ2kTlmRecord.h>

#include <jp2.h>
#include <kdu_sample_processing.h>
#include <kdu_region_decompressor.h>
 
#include <iostream>

using namespace std;

using namespace kdu_core;
using namespace kdu_supp;

#ifdef OSSIM_ID_ENABLED
static const char OSSIM_ID[] = "$Id";
#endif
 
static const ossimTrace traceDebug( ossimString("ossimKakaduNitfReader:debug") );
static const ossimTrace traceDump( ossimString("ossimKakaduNitfReader:dump") );
 
RTTI_DEF1_INST(ossimKakaduNitfReader,
               "ossimKakaduNitfReader",
               ossimNitfTileSource)

ossimKakaduNitfReader::ossimKakaduNitfReader()
   : ossimNitfTileSource(),
     m_startOfCodestreamOffset(0),
     m_jp2FamilySrc(0),
     m_jp2Source(0),
     m_channels(0),
     m_codestream(),
     m_threadEnv(0),
     m_openTileThreadQueue(0),
     m_sourcePrecisionBits(0),
     m_minDwtLevels(0),
     m_imageDims(0),
     m_minSampleValue(ossim::nan()),
     m_maxSampleValue(ossim::nan()),
     m_nullSampleValue(ossim::nan())
{
   kdu_customize_warnings(&pretty_cout); // Deliver warnings to stdout.
   kdu_customize_errors(&pretty_cerr); // Deliver errors to stderr + throw exc
}

ossimKakaduNitfReader::~ossimKakaduNitfReader()
{
   // Kakadu kdu_thread_entity::terminate throws exceptions...
   try
   {
      // Cleanup processing environment
      if ( m_threadEnv )
      {
         m_threadEnv->terminate(m_openTileThreadQueue, true);

         // Terminates background codestream processing.
         m_threadEnv->cs_terminate(m_codestream);

         m_threadEnv->destroy();
         delete m_threadEnv;
         m_threadEnv = 0; 
      }
   
      if ( m_codestream.exists() )
      {
         m_codestream.destroy();
      }
      
      if ( m_openTileThreadQueue )
      {
         m_openTileThreadQueue = 0;
      }
      if ( m_jp2Source )
      {
         delete m_jp2Source;
         m_jp2Source = 0;
      }
      if ( m_jp2FamilySrc )
      {
         delete m_jp2FamilySrc;
         m_jp2FamilySrc = 0;
      }
      if ( m_channels )
      {
         m_channels->clear();
         delete m_channels;
         m_channels = 0;
      }
   }
   catch ( kdu_core::kdu_exception exc )
   {
      // kdu_exception is an int typedef.
      if ( m_threadEnv != 0 )
      {
         m_threadEnv->handle_exception(exc);
      }
      ostringstream e;
      e << "ossimKakaduNitfReader::~ossimKakaduNitfReader\n"
        << "Caught exception from kdu_region_decompressor: " << exc << "\n";
      ossimNotify(ossimNotifyLevel_WARN) << e.str() << std::endl;
   }
   catch ( std::bad_alloc& )
   {
      if ( m_threadEnv != 0 )
      {
         m_threadEnv->handle_exception(KDU_MEMORY_EXCEPTION);
      }
      std::string e =
         "Caught exception from kdu_region_decompressor: std::bad_alloc";
      ossimNotify(ossimNotifyLevel_WARN) << e << std::endl;
   }
   catch( ... )
   {
      std::string e =
         "Caught unhandled exception from kdu_region_decompressor";
      ossimNotify(ossimNotifyLevel_WARN) << e << std::endl;
   }
   
   ossimNitfTileSource::close();
}

ossimString ossimKakaduNitfReader::getShortName()const
{
   return ossimString("ossim_kakadu_nitf_reader");
}

ossimString ossimKakaduNitfReader::getLongName()const
{
   return ossimString("ossim kakadu nitf reader");
}

ossim_uint32 ossimKakaduNitfReader::getNumberOfDecimationLevels()const
{
   ossim_uint32 result = 1; // Add r0
   if ( isEntryJ2k() )
   {
      if (m_minDwtLevels)
      {
         //---
         // Add internal overviews.
         //---
         result += m_minDwtLevels;
      }

      if (theOverview.valid())
      {
         result += theOverview->getNumberOfDecimationLevels();
      }
   }
   else
   {
      result = ossimNitfTileSource::getNumberOfDecimationLevels();
   }
   return result;
}

ossim_uint32 ossimKakaduNitfReader::getNumberOfLines(
   ossim_uint32 resLevel) const
{
   ossim_uint32 result = 0;
   if ( isEntryJ2k() )
   {
      if ( isValidRLevel(resLevel) )
      {
         ossim_uint32 level = resLevel;
         
         if (theStartingResLevel) // This is an overview.
         {
            //---
            // Adjust the level to be relative to the reader using this as
            // overview.
            //---
            level = resLevel - theStartingResLevel;
         }
         
         if (level <= m_minDwtLevels)
         {
            result = m_imageDims[level].height();
         }
         else if ( theOverview.valid() )
         {
            // Note the non-adjusted resLevel is passed to this by design.
            result = theOverview->getNumberOfLines(resLevel);
         }
      }
   }
   else // Not our entry.
   {
      result = ossimNitfTileSource::getNumberOfLines(resLevel);
   }
   return result;
}

ossim_uint32 ossimKakaduNitfReader::getNumberOfSamples(
   ossim_uint32 resLevel) const
{
   ossim_uint32 result = 0;
   if ( isEntryJ2k() )
   {
      if ( isValidRLevel(resLevel) )
      {
         ossim_uint32 level = resLevel;
         
         if (theStartingResLevel) // This is an overview.
         {
            //---
            // Adjust the level to be relative to the reader using this as
            // overview.
            //---
            level = resLevel - theStartingResLevel;
         }

         if (level <= m_minDwtLevels)
         {
            result = m_imageDims[level].width();
         }
         else if ( theOverview.valid() )
         {
            // Note the non-adjusted resLevel is passed to this by design.
            result = theOverview->getNumberOfSamples(resLevel);
         }
      }
   }
   else // Not our entry.
   {
      result = ossimNitfTileSource::getNumberOfSamples(resLevel);
   }
   return result;
}

void ossimKakaduNitfReader::setMinPixelValue(ossim_uint32 band,
                                             const ossim_float64& pix)
{
   //---
   // Store and call base ossimImageHandler::setMinPixelValue in case someone
   // does a save state on the ossimImageHandler::theMetaData.
   //---
   m_minSampleValue = pix;
   ossimImageHandler::setMinPixelValue(band, pix);
   if ( theTile.valid() )
   {
      theTile->setMinPix(pix, band);
   }
   if ( theCacheTile.valid() )
   {
      theCacheTile->setMinPix(pix, band);
   }
}

void ossimKakaduNitfReader::setMaxPixelValue(ossim_uint32 band,
                                             const ossim_float64& pix)
{
   //---
   // Store and call base ossimImageHandler::setMaxPixelValue in case someone
   // does a save state on the ossimImageHandler::theMetaData.
   //---
   m_maxSampleValue  = pix;
   ossimImageHandler::setMaxPixelValue(band, pix);
   if ( theTile.valid() )
   {
      theTile->setMaxPix(pix, band);
   }
   if ( theCacheTile.valid() )
   {
      theCacheTile->setMaxPix(pix, band);
   }   
}

void ossimKakaduNitfReader::setNullPixelValue(ossim_uint32 band,
                                              const ossim_float64& pix)
{
   //---
   // NOTES:
   //
   // 1) If the null pixel value has changed a makeBlank must be performed or the validate
   // method will not work correctly.
   // 
   // 2) Must set the tile status to unknown prior to the makeBlank or it won't
   // do anything.
   //
   // 3) It would be nice to do this after all the calls to setNullPixelValue are finished
   // with a dirty flag or something.  In reality though the makeBlank will only be called
   // once as this only happens(usually) with elevation data which is one band.
   //---
   
   //---
   // Store and call base ossimImageHandler::setNullPixelValue in case someone
   // does a save state on the ossimImageHandler::theMetaData.
   //---
   m_nullSampleValue = pix;
   ossimImageHandler::setNullPixelValue(band, pix);
   if ( theTile.valid() )
   {
      if ( theTile->getNullPix(band) != pix )
      {
         theTile->setNullPix(pix, band);
         theTile->setDataObjectStatus(OSSIM_STATUS_UNKNOWN);
         theTile->makeBlank();
      }
   }
   if ( theCacheTile.valid() )
   {
      if ( theCacheTile->getNullPix(band) != pix )
      {
         theCacheTile->setNullPix(pix, band);
         theCacheTile->setDataObjectStatus(OSSIM_STATUS_UNKNOWN);
         theCacheTile->makeBlank();
      }
   }   
}

ossim_float64 ossimKakaduNitfReader::getMinPixelValue(ossim_uint32 band)const
{
   if ( ossim::isnan(m_minSampleValue) )
   {
      return ossimImageHandler::getMinPixelValue(band);
   }
   return m_minSampleValue;
}

ossim_float64 ossimKakaduNitfReader::getMaxPixelValue(ossim_uint32 band)const
{
   if ( ossim::isnan(m_maxSampleValue) )
   {
      return ossimImageHandler::getMaxPixelValue(band);
   }
   return m_maxSampleValue;
}

ossim_float64 ossimKakaduNitfReader::getNullPixelValue(ossim_uint32 band)const
{
   if ( ossim::isnan(m_nullSampleValue) )
   {
      return ossimImageHandler::getNullPixelValue(band);
   }
   return m_nullSampleValue;
}

bool ossimKakaduNitfReader::getOverviewTile(ossim_uint32 resLevel,
                                            ossimImageData* result)
{
   bool status = false;

   // static bool traced = false;
   
   // Must be j2k entry, not past number of levels, bands must match.
   if ( isEntryJ2k() )
   {
      if ( isValidRLevel(resLevel) && result &&
           (result->getNumberOfBands() == getNumberOfOutputBands()) )
      {
         ossim_uint32 level = resLevel - theStartingResLevel;

#if 0 /* please leave for debug */
         cout << "ovr get tile res level: " << resLevel
              << " start level: " << theStartingResLevel
              << "\nlevel: " << level
              << "\nrect3: " << result->getImageRectangle() << endl;
#endif

         if (level <= m_minDwtLevels)
         {
            // Internal overviews...
            try
            {
               if ( m_channels )
               {
                  status = ossim::copyRegionToTile(m_channels,
                                                   m_codestream,
                                                   static_cast<int>(level),
                                                   m_threadEnv,
                                                   m_openTileThreadQueue,
                                                   result);
               }
               else
               {
                  status = ossim::copyRegionToTile(m_codestream,
                                                   static_cast<int>(level),
                                                   m_threadEnv,
                                                   m_openTileThreadQueue,
                                                   result);
               }
            }
            catch(const ossimException& e)
            {
               ossimNotify(ossimNotifyLevel_WARN)
                  << __FILE__ << ":" << __LINE__ << " caught exception\n"
                  << e.what() << std::endl;
               status = false;
            }

         } // matches:  if (resLevel <= m_minDwtLevels)
         else if ( theOverview.valid() )
         {
            //---
            // Note: Passing non-adjusted "resLevel" to get tile by design.
            //---
            status = theOverview->getTile(result, resLevel);
         }
      }
      else // Failed range check.
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << __FILE__ << " " << __LINE__
            << "Range error." << std::endl;
         status = false;
      }
         
   } // matches:  if ( isEntryJ2k() )
   else
   {
      // Not our entry, call the base.
      status = ossimNitfTileSource::getOverviewTile(resLevel, result);
   }

   return status;
   
} // End:  bool ossimKakaduNitfReader::getOverviewTile( ... ) 

bool ossimKakaduNitfReader::loadBlock(ossim_uint32 x, ossim_uint32 y)
{
   
   bool result = true;

   if ( isEntryJ2k() )
   {
      ossimIpt ul(x, y);
      
      ossimIpt lr(ul.x + theCacheSize.x - 1,
                  ul.y + theCacheSize.y - 1);
      
      // Set the cache rectangle to be an even j2k tile.
      theCacheTile->setImageRectangle(ossimIrect(ul, lr));
      
      // Let the getOverviewTile do the rest of the work.
      if ( getOverviewTile(0, theCacheTile.get()) )
      {
         // Add it to the cache for the next time.
         ossimAppFixedTileCache::instance()->addTile(theCacheId, theCacheTile);
      }
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << __FILE__ << ":" << __LINE__
            << "\nossimKakaduNitfReader::loadBlock failed!"
            << std::endl;
         result = false;
      }
   }
   else
   {
      result = ossimNitfTileSource::loadBlock(x, y);
   }
   
   return result;
}

bool ossimKakaduNitfReader::parseFile()
{
   static const char MODULE[] = "ossimKakaduNitfReader::parseFile";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   bool result = ossimNitfTileSource::parseFile();

   if (result)
   {
      //---
      // Look through the entries to see if any are j2k; if not, set result to
      // false so the ossimNitfTileSource will pick this file up instead of
      // ossimKakaduNitfReader.
      //---

      result = false; // Must prove we have a j2k entry.
      
      for (ossim_uint32 i = 0; i < theNumberOfImages; ++i)
      {
         if (theNitfImageHeader[i]->getCompressionCode() == "C8") // j2k
         {
            result = true;
            break;
         }
      }
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " exit status = " << (result?"true\n":"false\n");
   }

   return result;
}

bool ossimKakaduNitfReader::allocate()
{
   static const char MODULE[] = "ossimKakaduNitfReader::allocate";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
      
   }

   bool result = ossimNitfTileSource::allocate();

   if ( isEntryJ2k() && result )
   {
      // This only finds "Start Of Codestream" (SOC) so it's fast.
      result = scanForJpegBlockOffsets();

      if (traceDebug())
      {
         const ossimNitfImageHeader* hdr = getCurrentImageHeader();
         if( hdr )
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "start_of_data: " << hdr->getDataLocation() << "\n";
         }
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "start_of_codestream: " << m_startOfCodestreamOffset << "\n";
      }
      
      if ( result )
      {
         // Kakadu throws exceptions so wrap in try block.
         try
         {
            // Position to start of code stream prior to create call.
            theFileStr->seekg(m_startOfCodestreamOffset, ios_base::beg);
            
            //---
            // Initialize the codestream.  The class ossimKakaduNitfReader is a
            // kdu_compressed source so we feed ourself to the codestream.
            //---
            
            // Construct multi-threaded processing environment if required.
            if ( m_threadEnv )
            {
               m_threadEnv->terminate(NULL, true);
               m_threadEnv->cs_terminate(m_codestream);
               m_threadEnv->destroy();
            }
            
            if( m_codestream.exists() )
            {
               m_codestream.destroy();
            }
            
            if ( !m_threadEnv )
            {
               m_threadEnv = new kdu_thread_env();
            }
            
            m_threadEnv->create(); // Creates the single "owner" thread
            
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
                     if ( !m_threadEnv->add_thread() )
                     {
                        if (traceDebug())
                        {
                           ossimNotify(ossimNotifyLevel_WARN) << "Unable to create thread!\n";
                        }
                     }
                  }
               }
            }
            
            m_openTileThreadQueue = m_threadEnv->add_queue(NULL,NULL,"open_tile_q");
            
            if ( checkJp2Signature() )
            {
               // Note we must be at the JP2 signature block at this point for the open call.
               m_jp2FamilySrc = new jp2_family_src();
               m_jp2FamilySrc->open(this);
               
               if (m_jp2FamilySrc->exists())
               {
                  m_jp2Source = new kdu_supp::jp2_source();
                  
                  m_jp2Source->open(m_jp2FamilySrc);
                  m_jp2Source->read_header();

                  if (traceDebug())
                  {
                     jp2_colour colour = m_jp2Source->access_colour();
                     if ( colour.exists() )
                     {
                        ossimNotify(ossimNotifyLevel_DEBUG)
                           << "jp2 color space: " << colour.get_space() << endl;
                     }
                  }
               }
               m_codestream.create(m_jp2Source, m_threadEnv);
            }
            else
            {
               m_codestream.create(this, m_threadEnv);
            }

            if ( m_codestream.exists() )
            {
               // Setting "resilient" mode fixes loadBlock errors on some data.
               m_codestream.set_resilient(true);

               //---
               // We have to store things here in this non-const method because
               // NONE of the kakadu methods are const.
               //---
               m_minDwtLevels = m_codestream.get_min_dwt_levels();

               //---
               // NOTE:  ossimNitfTileSource::allocate() calls open overview
               // before the number of levels is known.
               // Set the starting res level for the overview now that the levels
               // are known.
               //--- 
               if ( theOverview.valid() )
               {
                  theOverview->setStartingResLevel(m_minDwtLevels+1);
               }
         
               // ASSUMPTION:  All bands same bit depth.
               m_sourcePrecisionBits = m_codestream.get_bit_depth(0, true);
         
               m_codestream.set_persistent(); // ????
               m_codestream.enable_restart(); // ????

               // Get the image and tile dimensions.
               std::vector<ossimIrect> tileDims;
               if ( ossim::getCodestreamDimensions(m_codestream,
                                                   m_imageDims,
                                                   tileDims) == false )
               {
                  ossimNotify(ossimNotifyLevel_WARN)
                     << __FILE__ << " " << __LINE__ << " " << MODULE
                     << "Could not ascertain dimensions!" << std::endl;
                  result = false;
               }

               kdu_dims region_of_interest;
               region_of_interest.pos.x = 0;
               region_of_interest.pos.y = 0;
               region_of_interest.size.x = m_imageDims[0].width();
               region_of_interest.size.y = m_imageDims[0].height();
         
               m_codestream.apply_input_restrictions(
                  0, // first_component
                  0, // max_components (0 = all remaining will appear)
                  0, // highest resolution level
                  0, // max_layers (0 = all layers retained)
                  &region_of_interest, // expanded out to block boundary.
                  KDU_WANT_OUTPUT_COMPONENTS);

               // Configure the kdu_channel_mapping if it makes sense.
               configureChannelMapping();

               if (traceDebug())
               {
                  ossimNotify(ossimNotifyLevel_DEBUG)
                     << "m_codestream.get_num_components(false): "
                     << m_codestream.get_num_components(false)
                     << "\nm_codestream.get_num_components(true): "
                     << m_codestream.get_num_components(true)
                     << "\nm_codestream.get_bit_depth(0, true): "
                     << m_codestream.get_bit_depth(0, true)
                     << "\nm_codestream.get_signed(0, true): "
                     << m_codestream.get_signed(0, true)
                     << "\nm_codestream.get_min_dwt_levels(): "
                     << m_codestream.get_min_dwt_levels()
                     << "\ntheNumberOfInputBands: "
                     << theNumberOfInputBands
                     << "\ntheNumberOfOutputBands: "
                     << theNumberOfOutputBands
                     << "\nthreads: " << threads
                     << "\n";
                  for ( std::vector<ossimIrect>::size_type i = 0; i < m_imageDims.size(); ++i )
                  {
                     ossimNotify(ossimNotifyLevel_DEBUG)
                        << "m_imageDims[" << i << "]: " << m_imageDims[i] << "\n";
                  }
               }
            }
            else // if ( m_codestream.exists() )
            {
               result = false;
            }

         } // End: try block
         catch( const ossimException& e )
         {
            result = false;         
            if (traceDebug())
            {
               ossimNotify(ossimNotifyLevel_WARN)
                  << MODULE << " Caught exception: " << e.what() << std::endl;
            }
         }
         catch( ... )
         {
            result = false;
            if (traceDebug())
            {
               ossimNotify(ossimNotifyLevel_WARN)
                  << MODULE << " Caught unknown exception!" << std::endl;
            }
         }
      
      } // Matches: if ( result )

   } // Matches:    if ( isEntryJ2k() && result )

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " exit status = " << (result?"true":"false\n")
         << std::endl;
   }

   return result;
   
} // End: ossimKakaduNitfReader::allocate()

bool ossimKakaduNitfReader::allocateBuffers()
{
   bool result = ossimNitfTileSource::allocateBuffers();
   if ( result )
   {
      //---
      // The ossimImageHandler::openOverview can set our min, max and null before we
      // have our tiles allocated.
      // 
      // Set the min, max, and null in case we're being used as an overview handler
      // and it is out of sync with the base image.  Example is dted null is -32767
      // not default -32768.
      //---
      const ossim_uint32 BANDS = getNumberOfOutputBands();
      for (ossim_uint32 band = 0; band < BANDS; ++band)
      {
         if ( theTile.valid() )
         {
            if ( !ossim::isnan(m_maxSampleValue) )
            {
               theTile->setMaxPix(m_maxSampleValue,   band);
            }
            if ( !ossim::isnan(m_minSampleValue) )
            {
               theTile->setMinPix(m_minSampleValue,   band);
            }
            if ( !ossim::isnan(m_nullSampleValue) )
            {
               theTile->setNullPix(m_nullSampleValue, band);
            }
         }
         if ( theCacheTile.valid() )
         {
            if ( !ossim::isnan(m_maxSampleValue) )
            {
               theCacheTile->setMaxPix(m_maxSampleValue,   band);
            }
            if ( !ossim::isnan(m_minSampleValue) )
            {
               theCacheTile->setMinPix(m_minSampleValue,   band);
            }
            if ( !ossim::isnan(m_nullSampleValue) )
            {
               theCacheTile->setNullPix(m_nullSampleValue, band);
            }
         } 
      }
   }
   return result;
   
} // End: ossimKakaduNitfReader::allocateBuffer()
 
bool ossimKakaduNitfReader::canUncompress(const ossimNitfImageHeader* hdr) const
{
   bool result = false;
   if (hdr)
   {
      if (hdr->getCompressionCode() == "C8") // j2k
      {
         result = true;
      }
      else
      {
         result = ossimNitfTileSource::canUncompress(hdr);
      }
   }
   return result;
}
 
void ossimKakaduNitfReader::initializeReadMode()
{
   theReadMode = READ_MODE_UNKNOWN;
   const ossimNitfImageHeader* hdr = getCurrentImageHeader();
   if (hdr)
   {
      if (hdr->getCompressionCode() == "C8") // j2k
      {
         if ( (hdr->getIMode() == "B") && (hdr->getCompressionCode()== "C8") )
         {
            theReadMode = READ_JPEG_BLOCK; 
         }
      }
      else
      {
         ossimNitfTileSource::initializeReadMode();
      }
   }
}
 
bool ossimKakaduNitfReader::scanForJpegBlockOffsets()
{
   bool result = false;

   const ossimNitfImageHeader* hdr = getCurrentImageHeader();
   if( hdr )
   {  
      // Capture the start of data.  This is start of j2k main header.
      m_startOfCodestreamOffset = hdr->getDataLocation();

      if ( checkJp2Signature() )
      {
         result = true;
      }
      else
      {
         //---
         // Kakadu library finds j2k tiles.  We only find the "Start Of Codestream"(SOC)
         // which should be at getDataLocation() but not in all cases.
         //---
         
         // Seek to the first block.
         theFileStr->seekg(m_startOfCodestreamOffset, ios_base::beg);
         if ( theFileStr->good() )
         {
            //---
            // Read the first two bytes and test for SOC (Start Of Codestream)
            // marker.
            //---
            ossim_uint8 markerField[2];
            theFileStr->read( (char*)markerField, 2);
         
            if ( (markerField[0] == 0xff) && (markerField[1] == 0x4f) )
            {
               result = true;
            }
            else
            {
               //---
               // Scan the file from the beginning for SOC.  This handles cases
               // where the data does not fall on "hdr->getDataLocation()".
               //
               // Changed for multi-enty nitf where first entry is j2k, second is
               // uncompressed. Need to test at site. 02 August 2013 (drb)
               //---
               // theFileStr->seekg(0, ios_base::beg);
               theFileStr->seekg(m_startOfCodestreamOffset, ios_base::beg);
               char c;
               while ( theFileStr->get(c) )
               {
                  if (static_cast<ossim_uint8>(c) == 0xff)
                  {
                     if (  theFileStr->get(c) )
                     {
                        if (static_cast<ossim_uint8>(c) == 0x4f)
                        {
                           m_startOfCodestreamOffset = theFileStr->tellg();
                           m_startOfCodestreamOffset -= 2;
                           result = true;
                           break;
                        }
                     }
                  }
               }
            }
            
            if ( (result == true) && traceDump() )
            {
               dumpTiles(ossimNotify(ossimNotifyLevel_DEBUG));
            }
            
         } // matches: if (theFileStr->good())
         
      }  // if ( isJp2() ) ... else {

   } // matches: if (hdr)

   return result;
   
} // End: bool ossimKakaduNitfReader::scanForJpegBlockOffsets()
 
void ossimKakaduNitfReader::initializeSwapBytesFlag()
{
   if ( isEntryJ2k() )
   {
      // Kakadu library handles.
      theSwapBytesFlag = false;
   }
   else
   {
      ossimNitfTileSource::initializeSwapBytesFlag();
   }
}

void ossimKakaduNitfReader::initializeCacheTileInterLeaveType()
{
   if ( isEntryJ2k() )
   {
      // Always band separate here.
      theCacheTileInterLeaveType = OSSIM_BSQ;
   }
   else
   {
      ossimNitfTileSource::initializeCacheTileInterLeaveType();
   }
}

bool ossimKakaduNitfReader::isEntryJ2k() const
{
   bool result = false;
   const ossimNitfImageHeader* hdr = getCurrentImageHeader();
   if (hdr)
   {
      if (hdr->getCompressionCode() == "C8") // j2k
      {
         result = true;
      }
   }
   return result;
}

bool ossimKakaduNitfReader::checkJp2Signature()
{
   bool result = false;
   const ossimNitfImageHeader* hdr = getCurrentImageHeader();
   if( hdr )
   {
      std::streamoff startOfDataPos = hdr->getDataLocation();

      // Seek to the start of data.
      theFileStr->seekg(startOfDataPos, ios_base::beg);
      if ( theFileStr->good() )
      {
         const ossim_uint8 J2K_SIGNATURE_BOX[SIGNATURE_BOX_SIZE] = 
            {0x00,0x00,0x00,0x0c,0x6a,0x50,0x20,0x20,0x0d,0x0a,0x87,0x0a};
         
         ossim_uint8 box[SIGNATURE_BOX_SIZE];
         
         // Read in the box.
         theFileStr->read((char*)box, SIGNATURE_BOX_SIZE);
         result = true;
         for (ossim_uint32 i = 0; i < SIGNATURE_BOX_SIZE; ++i)
         {
            if (box[i] != J2K_SIGNATURE_BOX[i])
            {
               result = false;
               break;
            }
         }
      }
      
      // Seek back to the start of data.
      theFileStr->seekg(startOfDataPos, ios_base::beg);
   }
   
   return result;
}

void ossimKakaduNitfReader::configureChannelMapping()
{
   bool result = false;
   
   if ( m_channels )
   {
      m_channels->clear();
   }
   else
   {
      m_channels = new kdu_channel_mapping;
   }
            
   if ( m_jp2Source )
   {
      // Currently ignoring alpha:
      result = m_channels->configure(m_jp2Source, false);
   }
   else
   {
      result = m_channels->configure(m_codestream);
   }
   
   if ( result )
   {
      // If we the mapping doesn't have all our bands we don't use it.
      if ( m_channels->get_num_colour_channels() != static_cast<int>(theNumberOfOutputBands) )
      {
         result = false;
      }
   }

   if ( !result )
   {
      m_channels->clear();
      delete m_channels;
      m_channels = 0;
   }
}

std::ostream& ossimKakaduNitfReader::dumpTiles(std::ostream& out)
{
   //---
   // NOTE:
   // SOC = 0xff4f Start of Codestream
   // SOT = 0xff90 Start of tile
   // SOD = 0xff93 Last marker in each tile
   // EOC = 0xffd9 End of Codestream
   //---
   
   const ossimNitfImageHeader* hdr = getCurrentImageHeader();
   if(hdr)
   {
      // Capture the starting position.
      std::streampos currentPos = theFileStr->tellg();
      
      // Seek to the first block.
      theFileStr->seekg(m_startOfCodestreamOffset, ios_base::beg);
      if (theFileStr->good())
      {
         out << "offset to codestream: " << m_startOfCodestreamOffset << "\n";
         
         //---
         // Read the first two bytes and test for SOC (Start Of Codestream)
         // marker.
         //---
         ossim_uint8 markerField[2];
         theFileStr->read( (char*)markerField, 2);

         bool foundSot = false;
         if ( (markerField[0] == 0xff) && (markerField[1] == 0x4f) )
         {
            // Get the SIZ marker and dump it.
            theFileStr->read( (char*)markerField, 2);
            if ( (markerField[0] == 0xff) && (markerField[1] == 0x51) )
            {
               ossimJ2kSizRecord siz;
               siz.parseStream( *theFileStr );
               siz.print(out);
            }
            
            // Find the firt tile marker.
            char c;
            while ( theFileStr->get(c) )
            {
               if (static_cast<ossim_uint8>(c) == 0xff)
               {
                  if ( theFileStr->get(c) )
                  {
                     out << "marker: 0xff" << hex << (ossim_uint16)c << dec << endl;
                     
                     if (static_cast<ossim_uint8>(c) == 0x52)
                     {
                        out << "\nFound COD...\n\n" << endl;
                        ossimJ2kCodRecord cod;
                        cod.parseStream( *theFileStr );
                        cod.print(out);

                     }
                     else if (static_cast<ossim_uint8>(c) == 0x55)
                     {
                        out << "\nFound TLM...\n\n" << endl;
                        ossimJ2kTlmRecord tlm;
                        tlm.parseStream( *theFileStr );
                        tlm.print(out);
                     }
                     else if (static_cast<ossim_uint8>(c) == 0x90)
                     {
                        foundSot = true;
                        break;
                     }
                  }
               }
            }
         }

         if (foundSot) // At SOT marker...
         {
            const ossim_uint32 BLOCKS =
               hdr->getNumberOfBlocksPerRow() * hdr->getNumberOfBlocksPerCol();
            for (ossim_uint32 i = 0; i < BLOCKS; ++i)
            {
               std::streamoff pos = theFileStr->tellg();
               out << "sot pos: " << pos << endl;
               ossimJ2kSotRecord sotRecord;
               sotRecord.parseStream( *theFileStr );
               pos += sotRecord.thePsot;
               sotRecord.print(out);
               theFileStr->seekg(pos, ios_base::beg);
            }
         }
      }

      // If the last byte is read, the eofbit must be reset. 
      if ( theFileStr->eof() )
      {
         theFileStr->clear();
      }
      
      // Put the stream back to the where it was.
      theFileStr->seekg(currentPos);
   }

   return out;
}
