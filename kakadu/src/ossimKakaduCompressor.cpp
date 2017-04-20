//---
//
// License: MIT
//
// Author:  David Burken
//
// Description: Wrapper class to compress whole tiles using kdu_analysis
// object.
//
//---
// $Id$

#include "ossimKakaduCompressor.h"
#include "ossimKakaduCommon.h"
#include "ossimKakaduCompressedTarget.h"
#include "ossimKakaduKeywords.h"

#include <ossim/base/ossimBooleanProperty.h>
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimNumericProperty.h>
#include <ossim/base/ossimPreferences.h>
#include <ossim/base/ossimStringProperty.h>
#include <ossim/base/ossimTrace.h>

#include <ossim/imaging/ossimImageData.h>
#include <ossim/imaging/ossimImageGeometry.h>

#include <ossim/support_data/ossimGeoTiff.h>
#include <ossim/support_data/ossimGmlSupportData.h>
#include <ossim/support_data/ossimNitfJ2klraTag.h>

#include <jp2.h>
#include <cmath> /* ceil */

RTTI_DEF1_INST(ossimKakaduCompressor, "ossimKakaduCompressor", ossimObject)


//---
// For trace debugging (to enable at runtime do:
// your_app -T "ossimKakaduCompressor:debug" your_app_args
//---
static ossimTrace traceDebug("ossimKakaduCompressor:debug");

static const ossim_int32 DEFAULT_LEVELS = 5;

//---
// Matches ossimKakaduCompressionQuality enumeration:
// EPJE = Exploitation Preferred J2K Encoding
//---
static const ossimString COMPRESSION_QUALITY[] = { "unknown",
                                                   "user_defined",
                                                   "numerically_lossless",
                                                   "visually_lossless",
                                                   "lossy",
                                                   "lossy2",
                                                   "lossy3",
                                                   "epje" };

static void transfer_bytes(
   kdu_core::kdu_line_buf &dest, kdu_core::kdu_byte *src,
   int num_samples, int sample_gap, int src_bits, int original_bits)
{
   if (dest.get_buf16() != 0)
   {
      kdu_core::kdu_sample16 *dp = dest.get_buf16();
      kdu_core::kdu_int16 off = ((kdu_core::kdu_int16)(1<<src_bits))>>1;
      kdu_core::kdu_int16 mask = ~((kdu_core::kdu_int16)((-1)<<src_bits));
      if (!dest.is_absolute())
      {
         int shift = KDU_FIX_POINT - src_bits; assert(shift >= 0);
         for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            dp->ival = ((((kdu_core::kdu_int16) *src) & mask) - off) << shift;
      }
      else if (src_bits < original_bits)
      { // Reversible processing; source buffer has too few bits
         int shift = original_bits - src_bits;
         for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            dp->ival = ((((kdu_core::kdu_int16) *src) & mask) - off) << shift;
      }
      else if (src_bits > original_bits)
      { // Reversible processing; source buffer has too many bits
         int shift = src_bits - original_bits;
         off -= (1<<shift)>>1; // For rounded down-shifting
         for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            dp->ival = ((((kdu_core::kdu_int16) *src) & mask) - off) >> shift;
      }
      else
      { // Reversible processing, `src_bits'=`original_bits'
         for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            dp->ival = (((kdu_core::kdu_int16) *src) & mask) - off;
      }
   }
   else
   {
      kdu_core::kdu_sample32 *dp = dest.get_buf32();
      kdu_core::kdu_int32 off = ((kdu_core::kdu_int32)(1<<src_bits))>>1;
      kdu_core::kdu_int32 mask = ~((kdu_core::kdu_int32)((-1)<<src_bits));
      if (!dest.is_absolute())
      {
         float scale = 1.0F / (float)(1<<src_bits);
         for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            dp->fval = scale * (float)((((kdu_core::kdu_int32) *src) & mask) - off);
      }
      else if (src_bits < original_bits)
      { // Reversible processing; source buffer has too few bits
         int shift = original_bits - src_bits;
         for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            dp->ival = ((((kdu_core::kdu_int32) *src) & mask) - off) << shift;
      }
      else if (src_bits > original_bits)
      { // Reversible processing; source buffer has too many bits
         int shift = src_bits - original_bits;
         off -= (1<<shift)>>1; // For rounded down-shifting
         for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            dp->ival = ((((kdu_core::kdu_int32) *src) & mask) - off) >> shift;
      }
      else
      { // Reversible processing, `src_bits'=`original_bits'
         for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            dp->ival = (((kdu_core::kdu_int32) *src) & mask) - off;
      }
   }
}

static void transfer_words(
   kdu_core::kdu_line_buf &dest, kdu_core::kdu_int16 *src, int num_samples,
   int sample_gap, int src_bits, int original_bits,
   bool is_signed)
{
   if (dest.get_buf16() != 0)
   {
      kdu_core::kdu_sample16 *dp = dest.get_buf16();
      int upshift = 16-src_bits; assert(upshift >= 0);
      if (!dest.is_absolute())
      {
         if (is_signed)
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
               dp->ival = ((*src) << upshift) >> (16-KDU_FIX_POINT);
         else
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
               dp->ival = (((*src) << upshift) - 0x8000) >> (16-KDU_FIX_POINT);
      }
      else
      {
         // Reversible processing
         int downshift = 16-original_bits; assert(downshift >= 0);
         if (is_signed)
         {
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            {
               dp->ival = ((*src) << upshift) >> downshift;
            }
         }
         else
         {
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            {
               dp->ival = (((*src) << upshift) - 0x8000) >> downshift;
            }
         }
      }
   }
   else
   {
      kdu_core::kdu_sample32 *dp = dest.get_buf32();
      int upshift = 32-src_bits; assert(upshift >= 0);

      if (!dest.is_absolute())
      {
         float scale = 1.0F / (((float)(1<<16)) * ((float)(1<<16)));
         if (is_signed)
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
               dp->fval = scale * (float)(((kdu_core::kdu_int32) *src)<<upshift);
         else
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
               dp->fval = scale * (float)((((kdu_core::kdu_int32) *src)<<upshift)-(1<<31));
      }
      else
      {
         int downshift = 32-original_bits; assert(downshift >= 0);
         
         if (is_signed)
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
            {
               dp->ival = (((kdu_core::kdu_int32) *src)<<upshift) >> downshift;
            }
         else
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
               dp->ival = ((((kdu_core::kdu_int32) *src)<<upshift)-(1<<31)) >> downshift;
      }
   }
}

void transfer_dwords(kdu_core::kdu_line_buf &dest, kdu_core::kdu_int32 *src,
                     int num_samples, int sample_gap, int src_bits, int original_bits,
                     bool is_signed)
{
   if (dest.get_buf16() != NULL)
   {
      kdu_core::kdu_sample16 *dp = dest.get_buf16();
      int upshift = 32-src_bits; assert(upshift >= 0);
      if (!dest.is_absolute())
      {
          if (is_signed)
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
               dp->ival = (kdu_core::kdu_int16)
                  (((*src) << upshift) >> (32-KDU_FIX_POINT));
         else
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
               dp->ival = (kdu_core::kdu_int16)
                  ((((*src) << upshift)-0x80000000) >> (32-KDU_FIX_POINT));
      }
      else
      { // Reversible processing
         int downshift = 32-original_bits; assert(downshift >= 0);
         if (is_signed)
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
               dp->ival = (kdu_core::kdu_int16)
                  (((*src) << upshift) >> downshift);
         else
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
               dp->ival = (kdu_core::kdu_int16)
                  ((((*src) << upshift) - 0x80000000) >> downshift);
      }
   }
   else
   {
      kdu_core::kdu_sample32 *dp = dest.get_buf32();
      int upshift = 32-src_bits; assert(upshift >= 0);
      if (!dest.is_absolute())
      {
         float scale = 1.0F / (((float)(1<<16)) * ((float)(1<<16)));
         if (is_signed)
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
               dp->fval = scale * (float)((*src)<<upshift);
         else
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
               dp->fval = scale * (float)(((*src)<<upshift)-(1<<31));
      }
      else
      {
         int downshift = 32-original_bits; assert(downshift >= 0);
         if (is_signed)
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
               dp->ival = ((*src)<<upshift) >> downshift;
         else
            for (; num_samples > 0; num_samples--, src+=sample_gap, dp++)
               dp->ival = (((*src)<<upshift)-(1<<31)) >> downshift;
      }
   }
}



ossimKakaduCompressor::ossimKakaduCompressor()
   :
   ossimObject(),
   m_target(0),
   m_jp2FamTgt(0),
   m_jp2Target(0),
   m_codestream(),
   m_threadEnv(0),
   m_threadQueue(0),
   m_layerSpecCount(0),
   m_layerByteSizes(0),
   m_imageRect(),
   m_reversible(true),
   m_alpha(false),
   m_levels(0),
   m_threads(1),
   m_options(),
   m_qualityType(ossimKakaduCompressor::OKP_NUMERICALLY_LOSSLESS),
   m_normTile(0)
{
}

ossimKakaduCompressor::~ossimKakaduCompressor()
{
   finish();
}

ossimString ossimKakaduCompressor::getLongName() const
{
   return ossimString("ossim kakadu compressor");
}

ossimString ossimKakaduCompressor::getClassName() const
{
   return ossimString("ossimKakaduCompressor");
}


void ossimKakaduCompressor::create(ossim::ostream* os,
                                   ossimScalarType scalar,
                                   ossim_uint32 bands,
                                   const ossimIrect& imageRect,
                                   const ossimIpt& tileSize,
                                   ossim_uint32 tilesToWrite,
                                   bool jp2)
{
   static const char MODULE[] = "ossimKakaduCompressor::create";
   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_WARN) << MODULE << " entered...\n";
   }
   
#if 0 /* Please leave for debug. (drb) */
   cout << "levels:      " << m_levels
        << "\nreversible:  " << m_reversible
        << "\nthreads:     " << m_threads
        << "\nscalar:    " << scalar
        << "\nbands:     " << bands
        << "\nimageRect: " << imageRect
        << "\ntileSize:  " << tileSize
        << "\njp2:       " << jp2
        << endl;
#endif

   // In case we were reused.
   finish();

   if ( !os )
   {
      std::string errMsg = MODULE;
      errMsg += " ERROR: Null stream passed to method!";
      throw ossimException(errMsg);
   }
   
   if ( !os->good() )
   {
      std::string errMsg = MODULE;
      errMsg += " ERROR: Stream state has error!";
      throw ossimException(errMsg);
   }
   
   if ( ossim::getActualBitsPerPixel(scalar) > 31 )
   {
      // Data is not reversible.
      if ( m_reversible )
      {
         std::string errMsg = MODULE;
         errMsg += " ERROR: Reversible processing not possible with 32 bit data!";
         throw ossimException(errMsg);
      }

      // Create a tile for normalization.
      m_normTile = new ossimImageData(0,
                                      OSSIM_NORMALIZED_FLOAT,
                                      bands,
                                      static_cast<ossim_uint32>(tileSize.x),
                                      static_cast<ossim_uint32>(tileSize.y));
      m_normTile->initialize();
   }
   
   // Store for tile clip.
   m_imageRect = imageRect;
   
   m_target =  new ossimKakaduCompressedTarget();
   m_target->setStream(os);
   
   if (jp2)
   {
      //---
      // Note the jp2_family_tgt and the jp2_target classes merely store
      // the target and do not delete on close or destroy.
      //---
      m_jp2FamTgt = new kdu_supp::jp2_family_tgt();
      m_jp2FamTgt->open(m_target);
      m_jp2Target = new kdu_supp::jp2_target();
      m_jp2Target->open(m_jp2FamTgt);
   }

   if (m_alpha)
   {
      if ( (bands != 1) && (bands != 3) )
      {
         m_alpha = false;
         // if ( traceDebug() )
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << "Alpha channel being unset! Can only be used with "
               << "one or three band data.\n"
               << "Source image bands: " << bands << "\n";
         }
      }
   }
   
   kdu_core::siz_params siz;
   
   // Set the number of components adding in alpha if set.
   siz.set(Scomponents, 0, 0, static_cast<ossim_int32>( (m_alpha?bands+1:bands) ) );
   
   // Set the verical size.
   siz.set(Ssize, 0, 0, static_cast<ossim_int32>(imageRect.height()));
   siz.set(Sdims, 0, 0, static_cast<ossim_int32>(imageRect.height()));
   
   // Set the horizontal size.
   siz.set(Ssize, 0, 1, static_cast<ossim_int32>(imageRect.width()));
   siz.set(Sdims, 0, 1, static_cast<ossim_int32>(imageRect.width()));
   
   // Set the tile verical size.
   siz.set(Stiles, 0, 0, tileSize.y);
   
   // Set the tile horizontal size.
   siz.set(Stiles, 0, 1, tileSize.x);
   
   // Set the signed bit.
   siz.set(Ssigned, 0, 0, ossim::isSigned(scalar));
   
   // Set the precision bits.
   siz.set(Sprecision, 0, 0,
           static_cast<ossim_int32>(ossim::getActualBitsPerPixel(scalar)) );
   
   siz.finalize_all();

   // Set up threads:
   m_threadEnv = new kdu_core::kdu_thread_env();
   m_threadEnv->create();
   if ( m_threads == 1 )
   {
      // Look in prefs for threads:
      const char* lookup = ossimPreferences::instance()->findPreference("kakadu_threads");
      if ( lookup )
      {
         m_threads = ossimString::toUInt32(lookup);
      }
   }
   if ( m_threads > 1 )
   {
      for (int nt=1; nt < m_threads; ++nt)
      {
         if ( !m_threadEnv->add_thread() )
         {
            if (traceDebug())
            {
               ossimNotify(ossimNotifyLevel_WARN)
                  << "Unable to create thread!\n";
            }
         }
      }
   }
   
   m_threadQueue = m_threadEnv->add_queue(0, 0, "tile-compressor-root");
   
   kdu_supp::jp2_dimensions dims;
   
   if (jp2)
   {
      //---
      // Configure jp2 header attributes jp2_dimensions', `
      // jp2_colour', `jp2_channels', etc.) as appropriate.
      //---
      
      //---
      // Set dimensional information
      // (all redundant with the SIZ marker segment)
      //---
      dims = m_jp2Target->access_dimensions();
      dims.init(&siz);
      
      //---
      // Set colour space information (mandatory)
      // Since JP2 can only describe one and three band data if not three
      // band we will use the first channel only.
      //----
      kdu_supp::jp2_colour colour = m_jp2Target->access_colour();
      if (bands == 3)
      {
         colour.init( kdu_supp::JP2_sRGB_SPACE );
      }
      else
      {
         colour.init( kdu_supp::JP2_sLUM_SPACE );
      }
      
      //---
      // Set the channel mapping.  See note on colour space.
      //---
      int num_colours = static_cast<int>( (bands==3)?bands:1 );
      kdu_supp::jp2_channels channels = m_jp2Target->access_channels();
      channels.init(num_colours);
      for (int c=0; c < num_colours; ++c)
      {
         channels.set_colour_mapping(c,c);
      }
      
      if (m_alpha)
      {
         if (bands == 1)
         {
            channels.set_opacity_mapping(0,1);
         }
         else if (bands == 3)
         {
            channels.set_opacity_mapping(0,3);
            channels.set_opacity_mapping(1,3);
            channels.set_opacity_mapping(2,3);               
         }
      }
      
      m_codestream.create(&siz, m_jp2Target, 0, 0, 0, m_threadEnv);
   }
   else // Not a jp2
   {
      m_codestream.create(&siz, m_target, 0, 0, 0, m_threadEnv);
   }
   
   // Requests the insertion of TLM (tile-part-length) marker.
   setTlmTileCount(tilesToWrite);
   
   //---
   // Set up coding defaults.
   //---
   kdu_core::kdu_params* cod = m_codestream.access_siz()->access_cluster(COD_params);
   if (cod)
   {
      initializeCodingParams(cod, imageRect);
   }

   
   // Set options if any.
   std::vector<ossimString>::const_iterator optionIter = m_options.begin();
   while ( optionIter != m_options.end() )
   {
      m_codestream.access_siz()->parse_string( (*optionIter).c_str() );
      ++optionIter;
   }
   
   // Finalize preparation for compression
   m_codestream.access_siz()->finalize_all();
   
   if (jp2)
   {
      // Call `write_header' to write the JP2 header.
      m_jp2Target->write_header();
      m_jp2Target->close();
      
      // Write out the geotiff_box:
      // writeGeotffBox(m_jp2Target);
      
      //---
      // Optionally write additional boxes, opening them using the base
      // object's
      // `jp2_output_box::open_next' function, writing their contents (or
      // sub-boxes) and closing them using `jp2_output_box::close'.
      //---
   }

   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_WARN) << MODULE << " exiting...\n";
   }
}

void ossimKakaduCompressor::openJp2Codestream()
{
   if (m_jp2Target)

   {
      //---
      // Call `open_codestream' prior to any call to
      // `kdu_codestream::flush'.
      //---
      m_jp2Target->open_codestream(true);
   }
}

bool ossimKakaduCompressor::writeTile(ossimImageData& srcTile)
{
   bool result = true;

   if (srcTile.getDataObjectStatus() != OSSIM_NULL)
   {
      // tile samples:
      const ossim_int32 TILE_SAMPS =
         static_cast<ossim_int32>(srcTile.getWidth());

      // Samples to copy clipping to image width:
      const ossim_int32 SAMPS =
         ossim::min(TILE_SAMPS, m_imageRect.lr().x-srcTile.getOrigin().x+1);
      
      // tile lines:
      const ossim_int32 TILE_LINES =
         static_cast<ossim_int32>(srcTile.getHeight());

      // Lines to copy:
      const ossim_int32 LINES =
         ossim::min(TILE_LINES, m_imageRect.lr().y-srcTile.getOrigin().y+1);
      
      // Get the tile index.
      kdu_core::kdu_coords tileIndex;
      tileIndex.x = (srcTile.getOrigin().x - m_imageRect.ul().x) / TILE_SAMPS;
      tileIndex.y = (srcTile.getOrigin().y - m_imageRect.ul().y) / TILE_LINES;

      kdu_core::kdu_tile tile = m_codestream.open_tile(tileIndex);

      kdu_core::kdu_dims tile_dims;
      m_codestream.get_tile_dims(tileIndex, 0, tile_dims);

      if ( tile.exists() )
      {  
         // Bands:
         const ossim_int32 BANDS =
            static_cast<ossim_int32>(m_alpha?srcTile.getNumberOfBands()+1:
                                     srcTile.getNumberOfBands());
         tile.set_components_of_interest(BANDS);
         
         // Scalar:
         const ossimScalarType SCALAR = srcTile.getScalarType();
         
         // Signed:
         const bool SIGNED = ossim::isSigned(SCALAR);
         
         // Set up common things to both scalars.
         std::vector<kdu_core::kdu_push_ifc> engine(BANDS);
         std::vector<kdu_core::kdu_line_buf> lineBuf(BANDS);

         // Precision:
         ossim_int32 src_bits = ossim::getActualBitsPerPixel(SCALAR);

         std::vector<ossim_int32> original_bits(BANDS);

         // Initialize tile-components 
         kdu_core::kdu_tile_comp tc;
         kdu_core::kdu_resolution res;
         bool reversible;
         bool use_shorts;
         kdu_core::kdu_sample_allocator allocator;

         ossim_int32 band;
         for (band = 0; band < BANDS; ++band)
         {
            original_bits[band] = m_codestream.get_bit_depth(band,true);

            tc = tile.access_component(band);
            res = tc.access_resolution();
            if ( ossim::getActualBitsPerPixel(SCALAR) > 31 )
            {
               // Data is not reversible.
               reversible = false;
               use_shorts = false;
            }
            else
            {
               reversible = tc.get_reversible();
               use_shorts = (tc.get_bit_depth(true) <= 16);
            }
            
            res.get_dims(tile_dims);

            engine[band] = kdu_core::kdu_analysis(res,
                                        &allocator,
                                        use_shorts,
                                        1.0F,
                                        0,
                                        m_threadEnv,
                                        m_threadQueue);

            lineBuf[band].pre_create(&allocator,
                                     SAMPS,
                                     reversible, // tmp drb
                                     use_shorts,
                                     0,          // extend_left
                                     0);         // extend_right
         }
         
         // Complete sample buffer allocation
         allocator.finalize( m_codestream );

         for (band = 0; band < BANDS; ++band)
         {
            lineBuf[band].create();
         }

         switch (SCALAR)
         {
            case OSSIM_UINT8:
            {
               std::vector<kdu_core::kdu_byte*> srcBuf(BANDS);
               for (band = 0; band < BANDS; ++band)
               {
                  void* p = const_cast<void*>(srcTile.getBuf(band));
                  srcBuf[band] = static_cast<kdu_core::kdu_byte*>(p);
               }
               if (m_alpha)
               {
                  // Ugly casting...
                  const void* cp =
                     static_cast<const void*>(srcTile.getAlphaBuf());
                  void* p = const_cast<void*>(cp);
                  srcBuf[BANDS-1] = static_cast<kdu_core::kdu_byte*>(p);
               }
               for (ossim_int32 line = 0; line < LINES; ++line)
               {
                  for (band = 0; band < BANDS; ++band)
                  {
                     transfer_bytes(lineBuf[band],
                                    srcBuf[band],
                                    SAMPS,
                                    1,
                                    src_bits,
                                    original_bits[band]);
                     
                     engine[band].push(lineBuf[band], m_threadEnv);

                     // Increment the line buffers.
                     srcBuf[band] = srcBuf[band]+TILE_SAMPS;
                  }
               }
               break;
            }
            case OSSIM_UINT11:
            case OSSIM_UINT12:
            case OSSIM_UINT13:
            case OSSIM_UINT14:                  
            case OSSIM_UINT15:
            case OSSIM_UINT16:
            {
               if (!m_alpha)
               {
                  std::vector<kdu_core::kdu_int16*> srcBuf(BANDS);
                  for (band = 0; band < BANDS; ++band)
                  {
                     void* p = const_cast<void*>(srcTile.getBuf(band));
                     srcBuf[band] = static_cast<kdu_core::kdu_int16*>(p);
                  }
                  
                  for (ossim_int32 line = 0; line < LINES; ++line)
                  {
                     for (band = 0; band < BANDS; ++band)
                     {
                        transfer_words(lineBuf[band],
                                       srcBuf[band],
                                       SAMPS,
                                       1,
                                       src_bits,
                                       original_bits[band],
                                       SIGNED);
                     
                        engine[band].push(lineBuf[band], m_threadEnv);
                     
                        // Increment the line buffers.
                        srcBuf[band] = srcBuf[band]+TILE_SAMPS;
                     }
                  }
               }
               else // Need to write an alpha channel.
               {
                  //---
                  // Alpha currently stored a eight bit so we must move 255 to
                  // 2047 (11 bit) or 255 to 65535 for 16 bit.
                  //---
                  const ossim_float64 SCALAR_MAX = ossim::defaultMax( SCALAR );
                  ossim_float64 d = SCALAR_MAX / 255.0;

                  ossim_int32 dataBands = BANDS-1;
                  std::vector<kdu_core::kdu_int16*> srcBuf(dataBands);
                  for (band = 0; band < dataBands; ++band)
                  {
                     void* p = const_cast<void*>(srcTile.getBuf(band));
                     srcBuf[band] = static_cast<kdu_core::kdu_int16*>(p);
                  }
                  
                  const ossim_uint8* alphaPtr = srcTile.getAlphaBuf();;
                  std::vector<kdu_core::kdu_int16> alphaLine(SAMPS);
                  
                  for (ossim_int32 line = 0; line < LINES; ++line)
                  {
                     for (band = 0; band < dataBands; ++band)
                     {
                        transfer_words(lineBuf[band],
                                       srcBuf[band],
                                       SAMPS,
                                       1,
                                       src_bits,
                                       original_bits[band],
                                       SIGNED);
                        
                        engine[band].push(lineBuf[band], m_threadEnv);
                        
                        // Increment the line buffers.
                        srcBuf[band] = srcBuf[band]+TILE_SAMPS;
                     }
                     
                     // Transfer alpha channel:
                     for (ossim_int32 samp = 0; samp < SAMPS; ++samp)
                     {
                        alphaLine[samp] = static_cast<kdu_core::kdu_int16>(alphaPtr[samp]*d);
                     }

                     transfer_words(lineBuf[band],
                                    &alphaLine.front(),
                                    SAMPS,
                                    1,
                                    src_bits,
                                    original_bits[band],
                                    SIGNED);
                     
                     engine[band].push(lineBuf[band], m_threadEnv);
                     
                     alphaPtr = alphaPtr+TILE_SAMPS;
                  }
               } // End of alpha section.
               break;
            }
            case OSSIM_SINT16:
            {
               std::vector<ossim_sint16*> srcBuf(BANDS);
               for (band = 0; band < BANDS; ++band)
               {
                  void* p = const_cast<void*>(srcTile.getBuf(band));
                  srcBuf[band] = static_cast<ossim_sint16*>(p);
               }

               for (ossim_int32 line = 0; line < LINES; ++line)
               {
                  for (band = 0; band < BANDS; ++band)
                  {
                     transfer_words(lineBuf[band],
                                    srcBuf[band],
                                    SAMPS,
                                    1,
                                    src_bits,
                                    original_bits[band],
                                    SIGNED);
                     
                     engine[band].push(lineBuf[band], m_threadEnv);

                     // Increment the line buffers.
                     srcBuf[band] = srcBuf[band]+TILE_SAMPS;
                  }
               }
               break;
            }

            //---
            // ??? This should probably take the same path as OSSIM_SINT32 data.
            // Need test case.
            //---
            case OSSIM_UINT32:
            {
               std::vector<kdu_core::kdu_int32*> srcBuf(BANDS);
               for (band = 0; band < BANDS; ++band)
               {
                  void* p = const_cast<void*>(srcTile.getBuf(band));
                  srcBuf[band] = static_cast<kdu_core::kdu_int32*>(p);
               }
               
               for (ossim_int32 line = 0; line < LINES; ++line)
               {
                  for (band = 0; band < BANDS; ++band)
                  {
                     transfer_dwords(lineBuf[band],
                                     srcBuf[band],
                                     SAMPS,
                                     1,
                                     src_bits,
                                     original_bits[band],
                                     SIGNED);
                     
                     engine[band].push(lineBuf[band], m_threadEnv);
                     
                     // Increment the line buffers.
                     srcBuf[band] = srcBuf[band]+TILE_SAMPS;
                  }
               }
               break;  
            }

            case OSSIM_SINT32:
            case OSSIM_FLOAT32:
            {
               //---
               // Kakadu wants float data normalized between -0.5 and 0.5:
               // 1) Normalize between 0.0 and 1.0 using ossim code.
               // 2) Copy applying -0.5 offset.
               //---
               srcTile.copyTileToNormalizedBuffer(m_normTile->getFloatBuf());
               
               std::vector<ossim_float32*> srcBuf(BANDS);
               for (band = 0; band < BANDS; ++band)
               {
                  srcBuf[band] = m_normTile->getFloatBuf(band);
               }

               for (ossim_int32 line = 0; line < LINES; ++line)
               {
                  for (band = 0; band < BANDS; ++band)
                  {
                     kdu_core::kdu_sample32* dp = lineBuf[band].get_buf32();
                     for (ossim_int32 samp = 0; samp < SAMPS; ++samp)
                     {
                        dp[samp].fval = srcBuf[band][samp] - 0.5; // -.5 for kakadu.
                     }
                     engine[band].push(lineBuf[band], m_threadEnv);

                     // Increment the line buffers.
                     srcBuf[band] = srcBuf[band]+TILE_SAMPS;
                  }
               }
               break;
            }
            default:
            {
               ossimNotify(ossimNotifyLevel_WARN)
                  << __FILE__ << " " << __LINE__ << " Unhandle scalar!\n";
               result = false;
               break;
            }
            
         }  // End:  switch(scalar)

         if (m_threadEnv)
         {
            // m_threadEnv->synchronize(m_threadQueue);

            //---
            // Snip from kdu_threads.h:
            // If `descendants_only' is true, the function waits for all queues
            // descended from `root_queue' to identify themselves as "finished"
            // via the `kdu_thread_queue::all_done' function.
            //---
            m_threadEnv->join(m_threadQueue,
                              true); // descendants_only flag
         }
         
         for (band = 0; band < BANDS; ++band)
         {
            engine[band].destroy();
            lineBuf[band].destroy();
         }
         tile.close();
         allocator.restart();

         // Done with tile flush it...
         m_codestream.flush( &(m_layerByteSizes.front()),  // layerbytes,
                             m_layerSpecCount,        // num_layer_specs
                             0, // layer_thresholds
                             true, // trim_to_rate
                             true, // record_in_comseg
                             0.0, // tolerence,
                             m_threadEnv); // env
         
      } // if (tile.exists())
      else
      {
         result = false;
      }
   }
   else // srcTile has null status...
   {
      result = false;
   }

   return result;
   
} // End: ossimKakaduCompressor::writeTile

void ossimKakaduCompressor::finish()
{
   // Kakadu kdu_thread_entity::terminate throws exceptions...
   try
   {
      // Cleanup processing environment
      if ( m_threadEnv )
      {
         m_threadEnv->join(NULL,true); // Wait until all internal processing is complete.
         m_threadEnv->terminate(m_threadQueue, true);      
         m_threadEnv->cs_terminate(m_codestream);   // Terminates background codestream processing.

         // kdu_codestream::destroy causing "double free or corruption" exception.
         // m_codestream.destroy();

         m_threadEnv->destroy();
         delete m_threadEnv;
         m_threadEnv = 0;
      }
      
      m_normTile = 0;
      
      if (m_threadQueue)
      {
         m_threadQueue = 0;
      }
      
      if (m_jp2FamTgt)
      {
         delete m_jp2FamTgt;
         m_jp2FamTgt = 0;
      }
      
      if (m_jp2Target)
      {
         delete m_jp2Target;
         m_jp2Target = 0;
      }
      
      if (m_target)
      {
         delete m_target;
         m_target = 0;
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
   
   m_layerByteSizes.clear();
}

void ossimKakaduCompressor::setQualityType(ossimKakaduCompressionQuality type)
{
   m_qualityType = type;

   //---
   // Set the reversible flag for appropriate type.
   // Not sure what to set for unknown and user defined but visually lossless
   // and lossy need to set the reversible flag to false.
   //---
   if ( (type == ossimKakaduCompressor::OKP_VISUALLY_LOSSLESS) ||
        (type == ossimKakaduCompressor::OKP_LOSSY) ||
        (type == ossimKakaduCompressor::OKP_EPJE) )
   {
      setReversibleFlag(false);
   }
   else
   {
      setReversibleFlag(true);
   }
}

ossimKakaduCompressor::ossimKakaduCompressionQuality ossimKakaduCompressor::getQualityType() const
{
   return m_qualityType;
   
}

void ossimKakaduCompressor::setReversibleFlag(bool reversible)
{
   m_reversible = reversible;
}

bool ossimKakaduCompressor::getReversibleFlag() const
{
   return m_reversible;
}

void ossimKakaduCompressor::setAlphaChannelFlag(bool flag)
{
   m_alpha = flag;
}

bool ossimKakaduCompressor::getAlphaChannelFlag() const
{
   return m_alpha;
}

void ossimKakaduCompressor::setLevels(ossim_int32 levels)
{
   if (levels)
   {
      m_levels = levels;
   }
}

ossim_int32 ossimKakaduCompressor::getLevels() const
{
   return m_levels;
}

void ossimKakaduCompressor::setThreads(ossim_int32 threads)
{
   if (threads)
   {
      m_threads = threads;
   }
}

ossim_int32 ossimKakaduCompressor::getThreads() const
{
   return m_threads;
}

void ossimKakaduCompressor::setOptions(const std::vector<ossimString>& options)
{
   std::vector<ossimString>::const_iterator i = options.begin();
   while ( i != options.end() )
   {
      m_options.push_back( (*i) );
      ++i;
   }
}

bool ossimKakaduCompressor::setProperty(ossimRefPtr<ossimProperty> property)
{
   bool consumed = false;
   
   if ( property.valid() )
   {
      ossimString key = property->getName();

      if ( traceDebug() )
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "ossimKakaduCompressor::setProperty DEBUG:"
            << "\nkey: " << key
            << " values: " << property->valueToString() << std::endl;
      }
      
      if (key == ossimKeywordNames::COMPRESSION_QUALITY_KW)
      {
         setQualityTypeString(property->valueToString());
         consumed = true;
      }
      else if ( (key == LEVELS_KW) || (key ==  "Clevels") )
      {
         m_levels = property->valueToString().toInt32();
         consumed = true;
      }
      else if( (key == REVERSIBLE_KW) || (key == "Creversible") )
      {
         setReversibleFlag(property->valueToString().toBool());
         consumed = true;
      }
      else if (key == ADD_ALPHA_CHANNEL_KW)
      {
         m_alpha = property->valueToString().toBool();
         consumed = true;
      }
      else if ( key == THREADS_KW)
      {
         m_threads = property->valueToString().toInt32();
         consumed = true;
      }
      else if ( (key == "Clayers") ||
                (key == "Cprecincts") )
      {
         // Generic options passed through kdu_params::parse_string.

         // Make in the form of "key=value" for kdu_params::parse_string.
         ossimString option = key;
         option += "=";
         option += property->valueToString();
         
         // Add to list.
         m_options.push_back(option);
         
         consumed = true;
      } 
   }

   return consumed;
}

ossimRefPtr<ossimProperty> ossimKakaduCompressor::getProperty(
   const ossimString& name)const
{
   ossimRefPtr<ossimProperty> p = 0;

   if (name == ossimKeywordNames::COMPRESSION_QUALITY_KW)
   {
      // property value
      ossimString value = getQualityTypeString();

      if ( (value == COMPRESSION_QUALITY[ossimKakaduCompressor::OKP_USER_DEFINED] ) ||
           (value == COMPRESSION_QUALITY[ossimKakaduCompressor::OKP_UNKNOWN])
          )
      {
         value = COMPRESSION_QUALITY[ossimKakaduCompressor::OKP_NUMERICALLY_LOSSLESS];
      }
         
      // constraint list
      vector<ossimString> constraintList;
      constraintList.push_back(
         COMPRESSION_QUALITY[ossimKakaduCompressor::OKP_NUMERICALLY_LOSSLESS]);
      constraintList.push_back(
         COMPRESSION_QUALITY[ossimKakaduCompressor::OKP_VISUALLY_LOSSLESS]);
      constraintList.push_back(
         COMPRESSION_QUALITY[ossimKakaduCompressor::OKP_LOSSY]);
      constraintList.push_back(
         COMPRESSION_QUALITY[ossimKakaduCompressor::OKP_LOSSY2]);
      constraintList.push_back(
         COMPRESSION_QUALITY[ossimKakaduCompressor::OKP_LOSSY3]);
      constraintList.push_back(
         COMPRESSION_QUALITY[ossimKakaduCompressor::OKP_EPJE]);
      
      p = new ossimStringProperty(name,
                                  value,
                                  false, // not editable
                                  constraintList);
   }
   else if (name == LEVELS_KW)
   {
      p = new ossimNumericProperty(name, ossimString::toString(m_levels));
   }
   else if (name == REVERSIBLE_KW)
   {
      p = new ossimBooleanProperty(name, m_reversible);
   }
   else if (name == ADD_ALPHA_CHANNEL_KW)
   {
      p = new ossimBooleanProperty(name, m_alpha);
   }
   else if (name == THREADS_KW)
   {
      p = new ossimNumericProperty(name, ossimString::toString(m_threads));
   }   
   
   return p;
}

void ossimKakaduCompressor::getPropertyNames(
   std::vector<ossimString>& propertyNames)const
{
   propertyNames.push_back(ossimKeywordNames::COMPRESSION_QUALITY_KW);
   propertyNames.push_back(LEVELS_KW);
   propertyNames.push_back(REVERSIBLE_KW);
   propertyNames.push_back(THREADS_KW);
}

bool ossimKakaduCompressor::saveState(ossimKeywordlist& kwl,
                                      const char* prefix)const
{
   kwl.add( prefix,
            ossimKeywordNames::COMPRESSION_QUALITY_KW,
            getQualityTypeString().c_str(),
            true );
   
   kwl.add( prefix,
            LEVELS_KW,
            ossimString::toString(m_levels),
            true );
   
   kwl.add( prefix,
            REVERSIBLE_KW,
            ossimString::toString(m_reversible),
            true );

   kwl.add( prefix,
            ADD_ALPHA_CHANNEL_KW,
            ossimString::toString(m_alpha),
            true ); 
   
   kwl.add( prefix,
            THREADS_KW,
            ossimString::toString(m_threads),
            true );

   std::vector<ossimString>::size_type size = m_options.size();
   for (ossim_uint32 i = 0; i < size; ++i)
   {
      ossimString key = "option";
      key += ossimString::toString(i);
      
      kwl.add( prefix,
               key.c_str(),
               m_options[i].c_str(),
               true );
   }
   
   return true;
}

bool ossimKakaduCompressor::loadState(const ossimKeywordlist& kwl,
                                      const char* prefix)
{
   const char* value = 0;

   value = kwl.find(prefix, ossimKeywordNames::COMPRESSION_QUALITY_KW);
   if(value)
   {
      setQualityTypeString( ossimString(value) );
   }
   
   value = kwl.find(prefix, LEVELS_KW);
   if(value)
   {
      m_levels = ossimString(value).toInt32();
   }
   
   value = kwl.find(prefix, REVERSIBLE_KW);
   if(value)
   {
      setReversibleFlag(ossimString(value).toBool());
   }

   value = kwl.find(prefix, ADD_ALPHA_CHANNEL_KW);
   if(value)
   {
      m_alpha = ossimString(value).toBool();
   }
   
   value = kwl.find(prefix, THREADS_KW);
   if(value)
   {
      m_threads = ossimString(value).toInt32();
   }

   ossimString searchKey;
   if (prefix)
   {
      searchKey = prefix;
   }
   searchKey += "option";
   ossim_uint32 nOptions = kwl.numberOf(searchKey);
   for (ossim_uint32 i = 0; i < nOptions; ++i)
   {
      ossimString key = searchKey;
      key += ossimString::toString(i);
      
      const char* lookup = kwl.find(key.c_str());
      if (lookup)
      {
         m_options.push_back(ossimString(lookup));
      }
   }
   
   return true;
}

bool ossimKakaduCompressor::writeGeotiffBox(const ossimImageGeometry* geom,
                                            const ossimIrect& rect,
                                            const ossimFilename& tmpFile,
                                            ossimPixelType pixelType)
{
   bool result = false;
   
   if ( geom && m_jp2Target )
   {
      ossimRefPtr<const ossimImageGeometry> imgGeom = geom;
      ossimRefPtr<const ossimProjection> proj = imgGeom->getProjection();
      if ( proj.valid() )
      {
         //---
         // Make a temp file.  No means currently write a tiff straight to
         // memory.
         //---
         
         // Buffer to hold the tiff box.
         std::vector<ossim_uint8> buf;

         // Write to buffer.
         if ( ossimGeoTiff::writeJp2GeotiffBox(tmpFile,
                                               rect,
                                               proj.get(),
                                               buf,
                                               pixelType) )
         {
            //---
            // JP2 box type uuid in ascii expressed as an int.
            // "u(75), u(75), i(69), d(64"
            //---
            const ossim_uint32 UUID_TYPE = 0x75756964;
            
            // Write to a box on the JP2 file.
            m_jp2Target->open_next( UUID_TYPE );
            m_jp2Target->write(
               static_cast<kdu_core::kdu_byte*>(&buf.front()), static_cast<int>(buf.size()));
            m_jp2Target->close();
            result = true;
         }
      }
   }

   return result;
   
} // End: ossimKakaduCompressor::writeGeotiffBox

bool ossimKakaduCompressor::writeGmlBox( const ossimImageGeometry* geom,
                                         const ossimIrect& rect )
{
   bool result = false;

   if ( geom && m_jp2Target )
   {
      ossimGmlSupportData* gml = new ossimGmlSupportData();
      if ( gml->initialize( geom, rect ) )
      {
         // Write the xml to a stream.
         ostringstream xmlStr;
         if ( gml->write( xmlStr ) )
         {
            const ossim_uint8 ASOC_BOX_ID[4] = 
            {
               0x61, 0x73, 0x6F, 0x63
            };
            
            const ossim_uint8 LBL_BOX_ID[4] = 
            {
               0x6C, 0x62, 0x6C, 0x20
            };

            const ossim_uint8 XML_BOX_ID[4] = 
            {
               0x78, 0x6D, 0x6C, 0x20
            };
            
            ossim_uint32 xmlDataSize = xmlStr.str().size();
            
            // Set the 1st asoc box size and type
            ossim_uint32 boxSize = xmlDataSize + 17 + 8 + 8 + 8 + 8 + 8 + 8;
            if (ossim::byteOrder() == OSSIM_LITTLE_ENDIAN)
            {
               ossimEndian endian;
               endian.swap( boxSize );
            }

            const ossim_uint32 ASOC_BOX = 0x61736f63;

            m_jp2Target->open_next( ASOC_BOX );
            
            //m_jp2Target->close();
            //m_jp2Target->write( (const kdu_core::kdu_byte*)&boxSize, 4); // 1st asoc size
            // m_jp2Target->write( (const kdu_core::kdu_byte*)ASOC_BOX_ID, 4); // 1st asoc type

            // Set the 1st lbl box size, type, and data
            boxSize = 8 + 8;
            if (ossim::byteOrder() == OSSIM_LITTLE_ENDIAN)
            {
                ossimEndian endian;
                endian.swap( boxSize );
            }

            m_jp2Target->write((kdu_core::kdu_byte*)&boxSize, 4); // 1st lbl size
            m_jp2Target->write((const kdu_core::kdu_byte*)LBL_BOX_ID, 4); // 1st lbl type
            m_jp2Target->write((const kdu_core::kdu_byte*)"gml.data", 8); // 1st lbl data

            // Set the 2nd asoc box size and type
            boxSize = xmlDataSize + 17 + 8 + 8 + 8;
            if (ossim::byteOrder() == OSSIM_LITTLE_ENDIAN)
            {
                ossimEndian endian;
                endian.swap( boxSize );
            }
            m_jp2Target->write((kdu_core::kdu_byte*)&boxSize, 4); // 2nd asoc size
            m_jp2Target->write((const kdu_core::kdu_byte*)ASOC_BOX_ID, 4); // 2nd asoc type

            // Set the 2nd lbl box size, type, and data
            boxSize = 17 + 8;
            if (ossim::byteOrder() == OSSIM_LITTLE_ENDIAN)
            {
                ossimEndian endian;
                endian.swap( boxSize );
            }
            m_jp2Target->write((kdu_core::kdu_byte*)&boxSize, 4); // 2nd lbl size
            m_jp2Target->write((const kdu_core::kdu_byte*)LBL_BOX_ID, 4); // 2nd lbl type
            m_jp2Target->write((const kdu_core::kdu_byte*)"gml.root-instance", 17); // 2nd lbl data

            // Set the xml box size, type, and data
            boxSize = xmlDataSize + 8;
            if (ossim::byteOrder() == OSSIM_LITTLE_ENDIAN)
            {
                ossimEndian endian;
                endian.swap( boxSize );
            }
            m_jp2Target->write((kdu_core::kdu_byte*)&boxSize, 4); // xml size
            m_jp2Target->write((const kdu_core::kdu_byte*)XML_BOX_ID, 4); // xml type
            m_jp2Target->write((const kdu_core::kdu_byte*)xmlStr.str().data(),
                               xmlDataSize); // xml data

            m_jp2Target->close();
            result = true;
         }
      }
      
      // cleanup:
      delete gml;
      gml = 0;
      
   } // if ( geom && m_jp2Target )

   return result;
   
} // End: ossimKakaduCompressor::writeGmlBox

void ossimKakaduCompressor::initialize( ossimNitfJ2klraTag* j2klraTag,
                                        ossim_uint32 actualBitsPerPixel ) const
{
   if ( j2klraTag )
   {
      j2klraTag->setOrigin(0);
      j2klraTag->setLevelsO( (ossim_uint32)m_levels );
      // bands set in writer.
      j2klraTag->setLayersO( (ossim_uint32)m_layerSpecCount );

      const ossim_float64 TP = m_imageRect.area();
      if ( TP )
      {
         for (ossim_uint32 id = 0; id < (ossim_uint32)m_layerSpecCount; ++id)
         {
            j2klraTag->setLayerId( id, id);

            if ( m_layerByteSizes[id] != KDU_LONG_MAX )
            {
               j2klraTag->setLayerBitRate(
                  id, ( m_layerByteSizes[id] / (TP * 0.125) ) );
            }
            else
            {
               //---
               // KDU_LONG_MAX indicates that the final quality layer should
               // include all compressed bits. (abpp=actuall bits per pixel)
               //---
               j2klraTag->setLayerBitRate( id, actualBitsPerPixel );
            }
         }
      }
   }
}

void ossimKakaduCompressor::initializeCodingParams(kdu_core::kdu_params* cod,
                                                   const ossimIrect& imageRect)
{
   static const char MODULE[] = "ossimKakaduCompressor::initializeCodingParams";
   
   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }
   
   if (cod)
   {
      // No ycc.
      cod->set(Cycc, 0, 0, false);

      // Set the number of levels
      setLevels(cod, imageRect, m_levels);

      // Set the block size.  Note 64x64 is the current kakadu default.
      setCodeBlockSize(cod, 64, 64);

      //---
      // Set the compression order.  Note LRCP is the current kakadu default.
      // L=layer; R=resolution C=component; P=position
      //---
      if ( m_qualityType != OKP_EPJE )
      {
         setProgressionOrder(cod, Corder_LRCP);
      }
      else
      {
         setProgressionOrder(cod, Corder_RLCP);
      }
      
      // total pixels
      const ossim_float64 TP = imageRect.area();

      if ( traceDebug() )
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "quality type: " << getQualityTypeString() << endl;
      }

      //---
      // Rate is a ratio of desired bytes / total bytes.  So if you
      // want 4 bits per pixels it's total_pixels * 4 / 8 or
      // total_pixels * 4 * 0.125.
      //---
      
      switch (m_qualityType)
      {
         case ossimKakaduCompressor::OKP_NUMERICALLY_LOSSLESS:
         case ossimKakaduCompressor::OKP_EPJE:            
         {
            setReversibleFlag(true);

            setWaveletKernel(cod, Ckernels_W5X3);
            
            m_layerSpecCount = 20;
            m_layerByteSizes.resize(m_layerSpecCount);

            m_layerByteSizes[0] =
               static_cast<kdu_core::kdu_long>(std::ceil( TP * 0.03125 * 0.125 ));
            m_layerByteSizes[1] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.0625* 0.125 ));
            m_layerByteSizes[2] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.125* 0.125 ));
            m_layerByteSizes[3] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.25* 0.125 ));
            m_layerByteSizes[4] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.5* 0.125 ));
            m_layerByteSizes[5] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.6* 0.125 ));
            m_layerByteSizes[6] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.7* 0.125 ));
            m_layerByteSizes[7] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.8* 0.125 ));
            m_layerByteSizes[8] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.9* 0.125 ));
            m_layerByteSizes[9] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 1.0* 0.125 ));
            m_layerByteSizes[10] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 1.1* 0.125 ));
            m_layerByteSizes[11] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 1.2* 0.125 ));
            m_layerByteSizes[12] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 1.3* 0.125 ));
            m_layerByteSizes[13] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 1.5* 0.125 ));
            m_layerByteSizes[14] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 1.7* 0.125 ));
            m_layerByteSizes[15] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 2.0* 0.125 ));
            m_layerByteSizes[16] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 2.3* 0.125 ));
            m_layerByteSizes[17] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 2.8* 0.125 ));
            m_layerByteSizes[18] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 3.5* 0.125 ));

            //---
            // Indicate that the final quality layer should include all
            // compressed bits.
            //---
            m_layerByteSizes[19] = KDU_LONG_MAX;

            break;
         }
         case ossimKakaduCompressor::OKP_VISUALLY_LOSSLESS:
         {
            setReversibleFlag(false);
            
            setWaveletKernel(cod, Ckernels_W9X7);
            
            m_layerSpecCount = 19;
            m_layerByteSizes.resize(m_layerSpecCount);
            m_layerByteSizes[0] =
               static_cast<kdu_core::kdu_long>(std::ceil( TP * 0.03125 * 0.125 ));
            m_layerByteSizes[1] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.0625* 0.125 ));
            m_layerByteSizes[2] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.125* 0.125 ));
            m_layerByteSizes[3] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.25* 0.125 ));
            m_layerByteSizes[4] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.5* 0.125 ));
            m_layerByteSizes[5] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.6* 0.125 ));
            m_layerByteSizes[6] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.7* 0.125 ));
            m_layerByteSizes[7] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.8* 0.125 ));
            m_layerByteSizes[8] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.9* 0.125 ));
            m_layerByteSizes[9] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 1.0* 0.125 ));
            m_layerByteSizes[10] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 1.1* 0.125 ));
             m_layerByteSizes[11] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 1.2* 0.125 ));
            m_layerByteSizes[12] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 1.3* 0.125 ));
            m_layerByteSizes[13] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 1.5* 0.125 ));
            m_layerByteSizes[14] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 1.7* 0.125 ));
            m_layerByteSizes[15] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 2.0* 0.125 ));
            m_layerByteSizes[16] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 2.3* 0.125 ));
            m_layerByteSizes[17] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 2.8* 0.125 ));
            m_layerByteSizes[18] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 3.5* 0.125 ));
            break;
         }
#if 0
         case ossimKakaduCompressor::OKP_EPJE:
         {
            //---
            // Exploitation Preferred J2K Encoding(EPJE):
            // This is currently the same as VISUALLY_LOSSLESS but making
            // separate code block as I anticipate tweaking this.
            // drb - 16 Mar. 2016
            //---
            setReversibleFlag(false);
            
            setWaveletKernel(cod, Ckernels_W9X7);
            
            m_layerSpecCount = 19;
            m_layerByteSizes.resize(m_layerSpecCount);
            m_layerByteSizes[0] =
               static_cast<kdu_core::kdu_long>(std::ceil( TP * 0.03125 * 0.125 ));
            m_layerByteSizes[1] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.0625* 0.125 ));
            m_layerByteSizes[2] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.125* 0.125 ));
            m_layerByteSizes[3] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.25* 0.125 ));
            m_layerByteSizes[4] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.5* 0.125 ));
            m_layerByteSizes[5] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.6* 0.125 ));
            m_layerByteSizes[6] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.7* 0.125 ));
            m_layerByteSizes[7] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.8* 0.125 ));
            m_layerByteSizes[8] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.9* 0.125 ));
            m_layerByteSizes[9] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 1.0* 0.125 ));
            m_layerByteSizes[10] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 1.1* 0.125 ));
             m_layerByteSizes[11] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 1.2* 0.125 ));
            m_layerByteSizes[12] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 1.3* 0.125 ));
            m_layerByteSizes[13] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 1.5* 0.125 ));
            m_layerByteSizes[14] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 1.7* 0.125 ));
            m_layerByteSizes[15] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 2.0* 0.125 ));
            m_layerByteSizes[16] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 2.3* 0.125 ));
            m_layerByteSizes[17] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 2.8* 0.125 ));
            m_layerByteSizes[18] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 3.5* 0.125 ));
            break;
         }
#endif
         case ossimKakaduCompressor::OKP_LOSSY:
         {
            setReversibleFlag(false);
            
            setWaveletKernel(cod, Ckernels_W9X7);
            
            m_layerSpecCount = 10;
            m_layerByteSizes.resize(m_layerSpecCount);

            m_layerByteSizes[0] =
               static_cast<kdu_core::kdu_long>(std::ceil( TP * 0.03125 * 0.125 ));
            m_layerByteSizes[1] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.0625* 0.125 ));
            m_layerByteSizes[2] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.125* 0.125 ));
            m_layerByteSizes[3] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.25* 0.125 ));
            m_layerByteSizes[4] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.5* 0.125 ));
            m_layerByteSizes[5] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.6* 0.125 ));
            m_layerByteSizes[6] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.7* 0.125 ));
            m_layerByteSizes[7] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.8* 0.125 ));
            m_layerByteSizes[8] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.9* 0.125 ));
            m_layerByteSizes[9] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 1.0* 0.125 ));
            break;
         }

         case ossimKakaduCompressor::OKP_LOSSY2:
         {
            setReversibleFlag(false);
            
            setWaveletKernel(cod, Ckernels_W9X7);
            
            m_layerSpecCount = 5;
            m_layerByteSizes.resize(m_layerSpecCount);

            m_layerByteSizes[0] =
               static_cast<kdu_core::kdu_long>(std::ceil( TP * 0.03125 * 0.125 ));
            m_layerByteSizes[1] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.0625 * 0.125 ));
            m_layerByteSizes[2] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.125 * 0.125 ));
            m_layerByteSizes[3] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.25 * 0.125 ));
            m_layerByteSizes[4] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.5 * 0.125 ));
            break;
         }

         case ossimKakaduCompressor::OKP_LOSSY3:
         {
            setReversibleFlag(false);
            
            setWaveletKernel(cod, Ckernels_W9X7);
            
            m_layerSpecCount = 4;
            m_layerByteSizes.resize(m_layerSpecCount);

            m_layerByteSizes[0] =
               static_cast<kdu_core::kdu_long>(std::ceil( TP * 0.03125 * 0.125 ));
            m_layerByteSizes[1] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.0625 * 0.125 ));
            m_layerByteSizes[2] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.125 * 0.125 ));
            m_layerByteSizes[3] =
               static_cast<kdu_core::kdu_long>(std::ceil(TP * 0.25 * 0.125 ));
            break;
         }

         default:
         {
            m_layerSpecCount = 1;
            m_layerByteSizes.resize(m_layerSpecCount);
            m_layerByteSizes[0] = 0;

            ossimNotify(ossimNotifyLevel_WARN)
               << MODULE << "unhandled compression_quality type! Valid types:\n";
            printCompressionQualityTypes( ossimNotify(ossimNotifyLevel_WARN) );
         }
         
      } // matches: switch (m_qualityType)

      //---
      // Set reversible flag, note, this controls the kernel.
      // W5X3(default) if reversible = true, W9X7 if reversible is false.
      //---
      cod->set(Creversible, 0, 0, m_reversible);

      // Set the quality layers.
      setQualityLayers(cod, m_layerSpecCount);

      if ( traceDebug() )
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "reversible: " << m_reversible
            << "\nLevels: " << m_levels
            << "\nLayers: " << m_layerSpecCount;
         for (int n = 0; n < m_layerSpecCount; ++n)
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "\nsize:  " << m_layerByteSizes[n];
         }
         ossimNotify(ossimNotifyLevel_DEBUG) << std::endl;
      }
      

   } // matches: if (cod)
      
   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " exited...\n";
   }
}

int ossimKakaduCompressor::getNumberOfLayers() const
{
   return m_layerSpecCount;
}

ossimString ossimKakaduCompressor::getQualityTypeString() const
{
   return COMPRESSION_QUALITY[m_qualityType];
}

void ossimKakaduCompressor::setQualityTypeString(const ossimString& s)
{
   ossimString type = s;
   type.downcase();
   
   if ( type == COMPRESSION_QUALITY[ossimKakaduCompressor::OKP_UNKNOWN] )
   {
      setQualityType(ossimKakaduCompressor::OKP_UNKNOWN);
   }
   else if ( type == COMPRESSION_QUALITY[ossimKakaduCompressor::OKP_USER_DEFINED] )
   {
      setQualityType(ossimKakaduCompressor::OKP_USER_DEFINED);
   }
   else if ( type == COMPRESSION_QUALITY[ossimKakaduCompressor::OKP_NUMERICALLY_LOSSLESS] )
   {
      setQualityType(ossimKakaduCompressor::OKP_NUMERICALLY_LOSSLESS);
   }
   else if ( type == COMPRESSION_QUALITY[ossimKakaduCompressor::OKP_VISUALLY_LOSSLESS] )
   {
      setQualityType(ossimKakaduCompressor::OKP_VISUALLY_LOSSLESS);
   }
   else if (type == "lossy")
   {
      setQualityType(ossimKakaduCompressor::OKP_LOSSY);
   }
   else if (type == "lossy2")
   {
      setQualityType(ossimKakaduCompressor::OKP_LOSSY2);
   }
   else if (type == "lossy3")
   {
      setQualityType(ossimKakaduCompressor::OKP_LOSSY3);
   }
   else if (type == "epje")
   {
      setQualityType(ossimKakaduCompressor::OKP_EPJE);
   }
   else
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << "ossimKakaduCompressor::setQualityTypeString WARNING"
         << "\nUnhandled compression_quality type: " << type
         << "\n";
      printCompressionQualityTypes( ossimNotify(ossimNotifyLevel_WARN) );
   }
}

void ossimKakaduCompressor::setLevels(kdu_core::kdu_params* cod,
                                      const ossimIrect& imageRect,
                                      ossim_int32 levels)
{
   //---
   // Number of wavelet decomposition levels, or stages.  May not exceed 32.
   // Default is 5
   //---
   if (cod)
   {
      if (levels == 0)
      {
         levels = (ossim_int32)ossim::computeLevels(imageRect);
         if (levels == 0)
         {
            levels = 1; // Must have at least one.
         }
      }
      if ( (levels < 1) || (levels > 32) )
      {
         levels = 5;
      }
      cod->set(Clevels, 0, 0, levels);

      // Set the class attribute:
      setLevels(levels);
   }
}

void ossimKakaduCompressor::setCodeBlockSize(kdu_core::kdu_params* cod,
                                             ossim_int32 xSize,
                                             ossim_int32 ySize)
{
   //---
   // Nominal code-block dimensions (must be powers of 2 no less than 4 and
   // no greater than 1024).
   // Default block dimensions are {64,64}
   //---
   if (cod)
   {
      cod->set(Cblk,0,0,ySize);
      cod->set(Cblk,0,1,xSize);
   }
}

void ossimKakaduCompressor::setProgressionOrder(kdu_core::kdu_params* cod,
                                                ossim_int32 corder)
{
   //---
   // Default progression order (may be overridden by Porder).
   // The four character identifiers have the following interpretation:
   // L=layer; R=resolution; C=component; P=position.
   // The first character in the identifier refers to the index which
   // progresses most slowly, while the last refers to the index which
   // progresses most quickly.  [Default is LRCP]
   // Enumerations:  (LRCP=0,RLCP=1,RPCL=2,PCRL=3,CPRL=4)
   //---
   if (cod)
   {
      if ( (corder < 0) || (corder > 5) )
      {
         corder = Corder_LRCP;
      }
      cod->set(Corder,0,0,corder);
   }
}
 
void ossimKakaduCompressor::setWaveletKernel(kdu_core::kdu_params* cod,
                                             ossim_int32 kernel)
{
   //---
   // Wavelet kernels to use.  The special value, `ATK' means that an ATK
   // (Arbitrary Transform Kernel) marker segment is used to store the DWT
   // kernel.  In this case, the `Catk' attribute must be non-zero.
   // [Default is W5X3 if `Creversible' is true, W9X7 if `Creversible' is
   // false, and ATK if `Catk' is non-zero.
   // Enumerations: (W9X7=0,W5X3=1,ATK=-1)
   //---
   if (cod)
   {
      if ( (kernel < -1) || (kernel > 1) )
      {
         if ( m_reversible )
         {
            kernel = Ckernels_W5X3;
         }
         else
         {
            kernel = Ckernels_W9X7; 
         }
      }
      cod->set(Ckernels,0,0,kernel);
   }
}

void ossimKakaduCompressor::setQualityLayers(kdu_core::kdu_params* cod,
                                             ossim_int32 layers)
{
   //---
   // Number of quality layers. May not exceed 16384. Kakadu default is 1.
   //---
   if (cod)
   {
      if ( (layers < 1) || (layers > 16384) )
      {
         layers = 1;
      }
      cod->set(Clayers,0,0,layers);
   }
}

void ossimKakaduCompressor::setTlmTileCount(ossim_uint32 /* tilesToWrite */)
{
   //---
   // Identifies the maximum number of tile-parts which will be written to the
   // code-stream for each tile.
   //
   // See Kakadu kdu_params.h "Macro = `ORGgen_tlm'" for more.
   //---
   ossimString s = "ORGgen_tlm=1";
   m_codestream.access_siz()->parse_string( s.c_str() );
}

void ossimKakaduCompressor::printCompressionQualityTypes( std::ostream& out ) const
{
   out << "compression_quality="
       << COMPRESSION_QUALITY[ossimKakaduCompressor::OKP_NUMERICALLY_LOSSLESS]
       << "\ncompression_quality="
       << COMPRESSION_QUALITY[ossimKakaduCompressor::OKP_VISUALLY_LOSSLESS]
       << "\ncompression_quality="
       << COMPRESSION_QUALITY[ossimKakaduCompressor::OKP_LOSSY]
       << "\ncompression_quality="
       << COMPRESSION_QUALITY[ossimKakaduCompressor::OKP_LOSSY2]
       << "\ncompression_quality="
       << COMPRESSION_QUALITY[ossimKakaduCompressor::OKP_LOSSY3]
       << "\ncompression_quality="
       << COMPRESSION_QUALITY[ossimKakaduCompressor::OKP_EPJE]
       << "\n";
}
