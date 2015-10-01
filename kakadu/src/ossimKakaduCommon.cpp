//----------------------------------------------------------------------------
//
// License:  LGPL
//
// See LICENSE.txt file in the top level directory for more details.
//
// Description: Common code for this plugin.
//
//----------------------------------------------------------------------------
// $Id: ossimKakaduCommon.cpp 23121 2015-01-30 01:24:56Z dburken $

#include "ossimKakaduCommon.h"
#include "ossimKakaduCompressedTarget.h"

#include <ossim/base/ossimException.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimPreferences.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/imaging/ossimImageData.h>
#include <ossim/imaging/ossimOverviewBuilderBase.h>
#include <ossim/imaging/ossimTiffWriter.h>

#include <jp2.h>
#include <kdu_region_decompressor.h>
#include <kdu_compressed.h>
#include <kdu_threads.h>

#include <ostream>
#include <sstream>

static const ossimTrace traceDebug( ossimString("ossimKakaduCommon:debug") );

void ossim::getDims(const ossimIrect& rect, kdu_core::kdu_dims& dims)
{
   dims.pos.x = rect.ul().x;
   dims.pos.y = rect.ul().y;
   dims.size.x = static_cast<int>(rect.width());
   dims.size.y = static_cast<int>(rect.height());
}

void ossim::getRect(const kdu_core::kdu_dims& dims, ossimIrect& rect)
{
   rect = ossimIrect(dims.pos.x,
                     dims.pos.y,
                     dims.pos.x + dims.size.x - 1,
                     dims.pos.y + dims.size.y - 1);
}

bool ossim::clipRegionToImage(kdu_core::kdu_codestream& codestream,
                              kdu_core::kdu_dims& region,
                              int discard_levels,
                              kdu_core::kdu_dims& clipRegion)
{
   // Clip the region to the image dimensions.

   bool result = false;

   if ( codestream.exists() )
   {
      codestream.apply_input_restrictions(
         0, 0, discard_levels, 0, NULL, kdu_core::KDU_WANT_OUTPUT_COMPONENTS);
      
      kdu_core::kdu_dims dims;
      codestream.get_dims(0, dims);
      if ( region.intersects(dims) )
      {
         clipRegion = region.intersection(dims);
         result = true;
      }
   }

   return result;
}

bool ossim::getCodestreamDimensions(kdu_core::kdu_codestream& codestream,
                                    std::vector<ossimIrect>& imageDims,
                                    std::vector<ossimIrect>& tileDims)
{
   bool result = true;
   
   imageDims.clear();
   tileDims.clear();

   if ( codestream.exists() )
   {
      kdu_core::kdu_coords tileIdx(0, 0);
      
      ossim_uint32 levels = codestream.get_min_dwt_levels();

      for (ossim_uint32 level=0; level <= levels; ++level)
      {
         // Get the image dimensions.

         codestream.apply_input_restrictions(
            0,     // first_component
            0,     // max_components (0 = all remaining will appear)
            level, // highest resolution level
            0,     // max_layers (0 = all layers retained)
            NULL,  // expanded out to block boundary.
            kdu_core::KDU_WANT_OUTPUT_COMPONENTS);
         
         kdu_core::kdu_dims dims;
         codestream.get_dims(0, dims);
         
         // Make the imageRect upper left relative to any sub image offset.
         ossimIrect imageRect;
         getRect(dims, imageRect);
         
         imageDims.push_back(imageRect);

         // Get the tile dimensions.
         
         kdu_core::kdu_dims mappedRegion;
         codestream.map_region(0, dims, mappedRegion, true);
         
         kdu_core::kdu_tile tile = codestream.open_tile(tileIdx);
         if ( tile.exists() )
         {
            codestream.get_tile_dims( tile.get_tile_idx(), 0, dims );
            
            // Make the tile rect zero based.
            ossimIrect tileRect(0,
                                0,
                                dims.size.x-1,
                                dims.size.y-1);
            
            tileDims.push_back(tileRect);

            // Every open has a close.
            tile.close();
         }
         else
         {
            result = false;
         }
         
      } // matches: for (ossim_uint32 level=0; level <= levels; ++level)

      // Set things back to level 0.
      codestream.apply_input_restrictions(
         0,     // first_component
         0,     // max_components (0 = all remaining will appear)
         0, // highest resolution level
         0,     // max_layers (0 = all layers retained)
         NULL,  // expanded out to block boundary.
         kdu_core::KDU_WANT_OUTPUT_COMPONENTS);

      // Should be the same sizes as levels.
      if ( (imageDims.size() != tileDims.size()) ||
           (tileDims.size() != levels+1) )
      {
         result = false;
      }
   } 
   else // codestream.exists() == false
   {
      result = false;
   }

   return result;
}

// Takes a channel map and decompresses all bands at once.
bool ossim::copyRegionToTile(kdu_supp::kdu_channel_mapping* channelMapping,
                             kdu_core::kdu_codestream& codestream,
                             int discard_levels,
                             kdu_core::kdu_thread_env* threadEnv,
                             kdu_core::kdu_thread_queue* threadQueue,
                             ossimImageData* destTile)
{
   bool result = true;

   if ( channelMapping && destTile && codestream.exists())// && threadEnv && threadQueue )
   {
      try // Kakadu throws exceptions...
      {
         kdu_core::kdu_dims region;
         getDims(destTile->getImageRectangle(), region);
      
         kdu_core::kdu_dims clipRegion;
         if ( clipRegionToImage(codestream,
                                region,
                                discard_levels,
                                clipRegion) )
         {
            if (region != clipRegion)
            {
               // Not filling whole tile.
               destTile->makeBlank();
            }
         
            const ossim_uint32 BANDS = destTile->getNumberOfBands();
            const ossimScalarType SCALAR = destTile->getScalarType();
         
            int max_layers = INT_MAX;
            kdu_core::kdu_coords expand_numerator(1,1);
            kdu_core::kdu_coords expand_denominator(1,1);
            bool precise = true;
            kdu_core::kdu_component_access_mode access_mode =
               kdu_core::KDU_WANT_OUTPUT_COMPONENTS;
            bool fastest = false;

            // Start the kdu_region_decompressor.
            kdu_supp::kdu_region_decompressor krd;
            if ( krd.start( codestream,
                            channelMapping,
                            -1,
                            discard_levels,
                            max_layers,
                            clipRegion,
                            expand_numerator,
                            expand_denominator,
                            precise,
                            access_mode,
                            fastest,
                            threadEnv,
                            threadQueue )
                 == false)
            {
               std::string e = "kdu_region_decompressor::start error!";
               throw(ossimException(e));
            }

            bool expand_monochrome = false;
            int pixel_gap = 1;
            kdu_core::kdu_coords buffer_origin;
            buffer_origin.x = destTile->getImageRectangle().ul().x;
            buffer_origin.y = destTile->getImageRectangle().ul().y;
            int row_gap = region.size.x;
            int suggested_increment = static_cast<int>(destTile->getSize());
            int max_region_pixels = suggested_increment;
            kdu_core::kdu_dims incomplete_region = clipRegion;
            kdu_core::kdu_dims new_region;
            bool measure_row_gap_in_pixels = true;
         
            // For signed int set precision bit to 0 for kakadu.
            int precision_bits = (SCALAR != OSSIM_SINT16) ? codestream.get_bit_depth(0, true) : 0;

            switch (SCALAR)
            {
               case OSSIM_UINT8:
               {
                  // Get pointers to the tile buffers.
                  std::vector<kdu_core::kdu_byte*> channel_bufs(BANDS);
                  for ( ossim_uint32 band = 0; band < BANDS; ++band )
                  {
                     channel_bufs[band] = destTile->getUcharBuf(band);
                  }

                  while ( !incomplete_region.is_empty() )
                  {
                     if ( krd.process( &channel_bufs.front(),
                                       expand_monochrome,
                                       pixel_gap,
                                       buffer_origin,
                                       row_gap, 
                                       suggested_increment,
                                       max_region_pixels,
                                       incomplete_region,
                                       new_region,
                                       precision_bits,
                                       measure_row_gap_in_pixels ) == false )
                     {
                        break;
                     }
                  }

                  // Wait for things to finish.
                  if( threadEnv && threadQueue )
                  {
                     //---
                     // Wait for all queues descended from `root_queue' to identify themselves
                     // as "finished" via the `kdu_thread_queue::all_done' function.
                     //---
                     threadEnv->join(threadQueue, true);
                  }
                  
                  // Validate the tile.
                  destTile->validate();

                  break;
               }
               case OSSIM_USHORT11:
               case OSSIM_UINT16:
               case OSSIM_SINT16:
               {
                  // Get pointers to the tile buffers.
                  std::vector<kdu_core::kdu_uint16*> channel_bufs(BANDS);
                  for ( ossim_uint32 band = 0; band < BANDS; ++band )
                  {
                     channel_bufs[band] = static_cast<kdu_core::kdu_uint16*>(
                        destTile->getBuf(band));
                  }

                  while ( !incomplete_region.is_empty() )
                  {
                     //---
                     // Note: precision_bits set to 0 to indicate "signed" data
                     // for the region decompressor.
                     //---
                     if ( krd.process( &channel_bufs.front(),
                                       expand_monochrome,
                                       pixel_gap,
                                       buffer_origin,
                                       row_gap, 
                                       suggested_increment,
                                       max_region_pixels,
                                       incomplete_region,
                                       new_region,
                                       precision_bits,
                                       measure_row_gap_in_pixels ) == false )
                     {
                        break;
                     }
                  }

                  // Wait for things to finish.
                  if( threadEnv && threadQueue )
                  {
                     //---
                     // Wait for all queues descended from `root_queue' to identify themselves
                     // as "finished" via the `kdu_thread_queue::all_done' function.
                     //---
                     threadEnv->join(threadQueue, true);
                  }
                  
                  // Validate the tile.
                  destTile->validate();

                  break;
               }
               case OSSIM_SINT32:
               case OSSIM_FLOAT32:
               {
                  //---
                  // NOTES:
                  // 1) Signed 32 bit integer data gets normalized when compressed
                  //    so use the same code path as float data.
                  // 2) Cannot call "ossimImageData::getFloatBuf" as it will return a
                  //    null pointer if the destination tile is OSSIM_SINT32 scalar type.
                  //---

                  // Get pointers to the tile buffers.
                  std::vector<ossim_float32*> channel_bufs(BANDS);
                  for ( ossim_uint32 band = 0; band < BANDS; ++band )
                  {
                     channel_bufs[band] = static_cast<ossim_float32*>(destTile->getBuf(band));
                  }

                  while ( !incomplete_region.is_empty() )
                  {
                     //---
                     // Note: precision_bits set to 0 to indicate "signed" data
                     // for the region decompressor.
                     //---
                     if ( krd.process( &channel_bufs.front(),
                                       expand_monochrome,
                                       pixel_gap,
                                       buffer_origin,
                                       row_gap,
                                       suggested_increment,
                                       max_region_pixels,
                                       incomplete_region,
                                       new_region,
                                       true, // normalize
                                       measure_row_gap_in_pixels ) )
                     {
                        break;
                     }
                  }
                  
                  if( threadEnv && threadQueue )
                  {
                     //---
                     // Wait for all queues descended from `root_queue' to identify themselves
                     // as "finished" via the `kdu_thread_queue::all_done' function.
                     //---
                     threadEnv->join(threadQueue, true);
                  }
                  
                  // Un-normalize.
                  ossim::unNormalizeTile(destTile);
                  
                  // Validate the tile.
                  destTile->validate();

                  break;
               }
               default:
               {
                  ossimNotify(ossimNotifyLevel_WARN)
                     << __FILE__ << " " << __LINE__ << " Unhandle scalar: "
                     << destTile->getScalarType() << "\n";
                  result = false;
                  break;
               }
               
            } // End of:  switch (theScalarType)
            
            // Every call to kdu_region_decompressor::start has a finish.
            if ( krd.finish() == false )
            {
               ossimNotify(ossimNotifyLevel_WARN)
                  << __FILE__ << " " << __LINE__
                  << "kdu_region_decompressor::proces error!\n";
            }
         }
         else // No region intersect with image.
         {
            destTile->makeBlank();
         }
         
      } // Matches: try{ ...
      
      // Catch and rethrow exceptions.
      catch( const ossimException& /* e */ )
      {
         throw;
      }
      catch ( kdu_core::kdu_exception exc )
      {
         // kdu_exception is an int typedef.
         if ( threadEnv != 0 )
         {
            threadEnv->handle_exception(exc);
         }
         ostringstream e;
         e << "Caught exception from kdu_region_decompressor: " << exc << "\n";
         throw ossimException( e.str() );
      }
      catch ( std::bad_alloc& )
      {
         if ( threadEnv != 0 )
         {
            threadEnv->handle_exception(KDU_MEMORY_EXCEPTION);
         }
         std::string e =
            "Caught exception from kdu_region_decompressor: std::bad_alloc";
         throw ossimException( e );
      }
      catch( ... )
      {
         std::string e =
            "Caught unhandled exception from kdu_region_decompressor";
         throw ossimException( e );
      }
   }
   else  // no codestream
   {
      result = false;
   }

#if 0  /* Please leave for serious debug. (drb) */
   if (destTile)
   {
      static int tileNumber = 0;
      if (destTile)
      {
         ossimFilename f = "tile-dump";
         f += ossimString::toString(tileNumber);
         f += ".ras";
         if (destTile->write(f))
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "wrote: " << f << std::endl;
            ++tileNumber;
         }
      }
   }
#endif
   
   return result;

} // End: ossim::copyRegionToTile

// Takes a codestream and decompresses band at a time.
bool ossim::copyRegionToTile(kdu_core::kdu_codestream& codestream,
                             int discard_levels,
                             kdu_core::kdu_thread_env* threadEnv,
                             kdu_core::kdu_thread_queue* threadQueue,
                             ossimImageData* destTile)
{
   bool result = true;

   if ( destTile && codestream.exists())// && threadEnv && threadQueue )
   {
      try // Kakadu throws exceptions...
      {
         kdu_core::kdu_dims region;
         getDims(destTile->getImageRectangle(), region);

         kdu_core::kdu_dims clipRegion;
         if ( clipRegionToImage(codestream,
                                region,
                                discard_levels,
                                clipRegion) )
         {
            if (region != clipRegion)
            {
               // Not filling whole tile.
               destTile->makeBlank();
            }
            
            const ossimScalarType SCALAR = destTile->getScalarType();
            const ossim_uint32 BANDS = destTile->getNumberOfBands();
            
            kdu_supp::kdu_channel_mapping* mapping = 0;
            int max_layers = INT_MAX;
            kdu_core::kdu_coords expand_numerator(1,1);
            kdu_core::kdu_coords expand_denominator(1,1);
            bool precise = true;
            kdu_core::kdu_component_access_mode access_mode =
               kdu_core::KDU_WANT_OUTPUT_COMPONENTS;
            bool fastest = false;
            
            //---
            // band loop:
            // Note: At some point we may want to be a band selector;
            // in which case, we would loop through the band list.
            // For now just go through all bands and let the ossimBandSelector
            // weed them out.
            //---
            for (ossim_uint32 band = 0; band < BANDS; ++band)
            {
               int single_component = band;
            
               // Start the kdu_region_decompressor.
               kdu_supp::kdu_region_decompressor krd;
               
               if ( krd.start( codestream,
                               mapping,
                               single_component,
                               discard_levels,
                               max_layers,
                               clipRegion,
                               expand_numerator,
                               expand_denominator,
                               precise,
                               access_mode,
                               fastest,
                               threadEnv,
                               threadQueue )
                    == false)
               {
                  std::string e = "kdu_region_decompressor::start error!";
                  throw(ossimException(e));
               }
               
               vector<int> channel_offsets(1);
               channel_offsets[0] = 0;
               int pixel_gap = 1;
               kdu_core::kdu_coords buffer_origin;
               buffer_origin.x = destTile->getImageRectangle().ul().x;
               buffer_origin.y = destTile->getImageRectangle().ul().y;
               int row_gap = region.size.x;
               int suggested_increment = static_cast<int>(destTile->getSize());
               int max_region_pixels = suggested_increment;
               kdu_core::kdu_dims incomplete_region = clipRegion;
               kdu_core::kdu_dims new_region;
               bool measure_row_gap_in_pixels = true;
               
               // For signed int set precision bit to 0 for kakadu.
               int precision_bits = (SCALAR != OSSIM_SINT16) ? codestream.get_bit_depth(0, true) : 0;
               
               switch (SCALAR)
               {
                  case OSSIM_UINT8:
                  {
                     // Get pointer to the tile buffer.
                     kdu_core::kdu_byte* buffer = destTile->getUcharBuf(band);
                     
                     while ( !incomplete_region.is_empty() )
                     {
                        if ( krd.process( buffer,
                                          &channel_offsets.front(),
                                          pixel_gap,
                                          buffer_origin,
                                          row_gap,
                                          suggested_increment,
                                          max_region_pixels,
                                          incomplete_region,
                                          new_region,
                                          precision_bits,
                                          measure_row_gap_in_pixels ) )
                        {
                           break;
                        }
                     }

                     if( threadEnv && threadQueue)
                     {
                        //---
                        // Wait for all queues descended from `root_queue' to identify themselves
                        // as "finished" via the `kdu_thread_queue::all_done' function.
                        //---
                        threadEnv->join(threadQueue, true);
                     }
                     
                     // Validate the tile.
                     destTile->validate();

                     break;
                  }
                  case OSSIM_USHORT11:
                  case OSSIM_UINT16:
                  case OSSIM_SINT16:   
                  {
                     // Get pointer to the tile buffer.
                     kdu_core::kdu_uint16* buffer =
                        static_cast<kdu_core::kdu_uint16*>(destTile->getBuf(band));
                     
                     while ( !incomplete_region.is_empty() )
                     {
                        //---
                        // Note: precision_bits set to 0 to indicate "signed" data
                        // for the region decompressor.
                        //---
                        if ( krd.process( buffer,
                                          &channel_offsets.front(),
                                          pixel_gap,
                                          buffer_origin,
                                          row_gap,
                                          suggested_increment,
                                          max_region_pixels,
                                          incomplete_region,
                                          new_region,
                                          precision_bits,
                                          measure_row_gap_in_pixels ) == false )
                        {
                           break;
                        }
                     }
                     
                     //---
                     // Wait for all queues descended from `root_queue' to identify themselves
                     // as "finished" via the `kdu_thread_queue::all_done' function.
                     //---
                     if( threadEnv && threadQueue)
                     {
                        threadEnv->join(threadQueue, true);
                     }
                     
                     // Validate the tile.
                     destTile->validate();
                  
                     break;
                  }
                  case OSSIM_SINT32:
                  case OSSIM_FLOAT32:
                  {
                     //---
                     // NOTES:
                     // 1) Signed 32 bit integer data gets normalized when compressed
                     //    so use the same code path as float data.
                     // 2) Cannot call "ossimImageData::getFloatBuf" as it will return a
                     //    null pointer if the destination tile is OSSIM_SINT32 scalar type.
                     //---
                     
                     // Get pointer to the tile buffer.
                     ossim_float32* buffer = static_cast<ossim_float32*>(destTile->getBuf(band));
                     
                     while ( !incomplete_region.is_empty() )
                     {
                        //---
                        // Note: precision_bits set to 0 to indicate "signed" data
                        // for the region decompressor.
                        //---
                        if ( krd.process( buffer,
                                          &channel_offsets.front(),
                                          pixel_gap,
                                          buffer_origin,
                                          row_gap,
                                          suggested_increment,
                                          max_region_pixels,
                                          incomplete_region,
                                          new_region,
                                          true, // normalize
                                          measure_row_gap_in_pixels ) == false )
                        {
                           break;
                        }
                     }
               
                     if( threadEnv && threadQueue)
                     {
                        //---
                        // Wait for all queues descended from `root_queue' to identify themselves
                        // as "finished" via the `kdu_thread_queue::all_done' function.
                        //---
                        threadEnv->join(threadQueue, true);
                     }
               
                     // Un-normalize.
                     ossim::unNormalizeTile(destTile);
                     
                     // Validate the tile.
                     destTile->validate();
                     
                     break;
                  }
                  
                  default:
                  {
                     ossimNotify(ossimNotifyLevel_WARN)
                        << __FILE__ << " " << __LINE__ << " Unhandle scalar: "
                        << destTile->getScalarType() << "\n";
                     result = false;
                     break;
                  }
         
               } // End of:  switch (theScalarType)
            
               // Every call to kdu_region_decompressor::start has a finish.
               if ( krd.finish() == false )
               {
                  result = false;
                  ossimNotify(ossimNotifyLevel_WARN)
                     << __FILE__ << " " << __LINE__
                     << " kdu_region_decompressor::proces error!" << std::endl;
               }
               
         
            } // End of band loop.
         }
         else // No region intersect with image.
         {
            destTile->makeBlank();
         }
         
      } // Matches: try{ ...

      // Catch and rethrow exceptions.
      catch( const ossimException& /* e */ )
      {
         throw;
      }
      catch ( kdu_core::kdu_exception exc )
      {
         // kdu_exception is an int typedef.
         if ( threadEnv != 0 )
         {
            threadEnv->handle_exception(exc);
         }
         ostringstream e;
         e << "Caught exception from kdu_region_decompressor: " << exc << "\n";
         throw ossimException( e.str() );
         
      }
      catch ( std::bad_alloc& )
      {
         if ( threadEnv != 0 )
         {
            threadEnv->handle_exception(KDU_MEMORY_EXCEPTION);
         }
         std::string e =
            "Caught exception from kdu_region_decompressor: std::bad_alloc";
         throw ossimException( e );
      }
      catch( ... )
      {
         std::string e =
            "Caught unhandled exception from kdu_region_decompressor";
         throw ossimException(e);
      }
   }
   else  // no codestream
   {
      result = false;
   }

#if 0  /* Please leave for serious debug. (drb) */
   if (destTile)
   {
      static int tileNumber = 0;
      if (destTile)
      {
         ossimFilename f = "tile-dump";
         f += ossimString::toString(tileNumber);
         f += ".ras";
         if (destTile->write(f))
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "wrote: " << f << std::endl;
            ++tileNumber;
         }
      }
   }
#endif
   
   return result;

} // End: ossim::copyRegionToTile

void ossim::unNormalizeTile(ossimImageData* result)
{
   if (result)
   {
      const ossim_uint32  SIZE  = result->getSize();
      const ossim_float64 MINP  = result->getMinPix(0);
      const ossim_float64 MAXP  = result->getMaxPix(0);
      const ossim_float64 RANGE = MAXP - MINP;
      
      if ( result->getScalarType() == OSSIM_FLOAT32 )
      {
         const ossim_float32 NULLP = static_cast<ossim_float32>(result->getNullPix(0));         
         ossim_float32* buf = result->getFloatBuf();
         for(ossim_uint32 idx = 0; idx < SIZE; ++idx)
         {
            ossim_float64 p = buf[idx];
            if(p > 0.0)
            {
               p = MINP + RANGE * p;
               p = (p < MAXP ? (p > MINP ? p : MINP) : MAXP);
               buf[idx] = static_cast<ossim_float32>(p);
            }
            else
            {
               buf[idx] = NULLP;
            }
         }
      }
      else if ( result->getScalarType() == OSSIM_SINT32 )
      {
         const ossim_sint32 NULLP = static_cast<ossim_sint32>(result->getNullPix(0));
         ossim_float32* inBuf = static_cast<ossim_float32*>(result->getBuf());
         ossim_sint32* outBuf = static_cast<ossim_sint32*>(result->getBuf());
         for(ossim_uint32 idx = 0; idx < SIZE; ++idx)
         {
            ossim_float64 p = inBuf[idx];
            if(p > 0.0)
            {
               p = MINP + RANGE * p;
               p = (p < MAXP ? (p > MINP ? p : MINP) : MAXP);
               outBuf[idx] = static_cast<ossim_sint32>(p);
            }
            else
            {
               outBuf[idx] = NULLP;
            }
         }         
      }
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << __FILE__ << " " << __LINE__ << " Unhandle scalar: "
            << result->getScalarType() << "\n";
      }
   }

} // End: ossim::unNormalizeTile

std::ostream& ossim::print(std::ostream& out, kdu_core::kdu_codestream& cs)
{
   out << "codestream debug:"
       << "exists: " << (cs.exists()?"true":"false");
   if (cs.exists())
   {
      const int BANDS = cs.get_num_components(true);
      out << "\ncomponents: " << BANDS;
      for (int i = 0; i < BANDS; ++i)
      {
         kdu_core::kdu_dims dims;
         cs.get_dims(i, dims, true);
         out << "\nbit_depth[" << i << "]: " << cs.get_bit_depth(i, true)
             << "\nsigned[" << i << "]: " << cs.get_signed(i, true)
             << "\ndims[" << i << "]: ";
         ossim::print(out, dims);
      }
      out << "\nlevels: " << cs.get_min_dwt_levels();
   }
   return out;
}

std::ostream& ossim::print(std::ostream& out, const kdu_core::kdu_dims& dims)
{
   out << "pos: ";
   print(out, dims.pos);
   out << ", size: ";
   print(out, dims.size);
   out << "\n";
   return out;
}

std::ostream& ossim::print(std::ostream& out, const kdu_core::kdu_coords& coords)
{
   out << "(" << coords.x << ", " << coords.y << ")";
   return out;
}
