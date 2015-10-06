//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description:  Place for common code used by both encoders and decoders
// using the openjpeg library.
//
// This code is namespaced with "ossim".
//
//----------------------------------------------------------------------------
// $Id$

#include <ossimOpjCommon.h>
#include <ossimOpjColor.h>
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/imaging/ossimImageData.h>

#include <openjpeg.h>

#include <cstring>
#include <istream>
#include <iostream>
#include <iomanip>

static ossimTrace traceDebug(ossimString("ossimOpjCommon:degug"));

/** To hold stream and offset. */
class opj_user_istream
{
public:
   opj_user_istream() : m_str(0), m_offset(0), m_length(0){}
   ~opj_user_istream(){ m_str = 0; } // We don't own stream.
   std::istream*   m_str;
   std::streamoff  m_offset;
   std::streamsize m_length;
};

/** Callback method for errors. */
void ossim::opj_error_callback(const char* msg, void* /* client_data */)
{
   ossimNotify(ossimNotifyLevel_WARN) << msg << std::endl;
}

/** Callback method for warnings. */
void ossim::opj_warning_callback(const char* msg, void* /* client_data */ )
{
   ossimNotify(ossimNotifyLevel_WARN) << msg << std::endl;
}

/** Callback method for info. */
void ossim::opj_info_callback(const char* /* msg */ , void* /* client_data */)
{
    //ossimNotify(ossimNotifyLevel_NOTICE) << msg << std::endl;
}

/** Callback function prototype for read function */ 
static OPJ_SIZE_T ossim_opj_istream_read( void * p_buffer,
                                          OPJ_SIZE_T p_nb_bytes,
                                          void * p_user_data )
{
   OPJ_SIZE_T count = 0;
   opj_user_istream* usrStr = static_cast<opj_user_istream*>(p_user_data);
   if ( usrStr )
   {
      if ( usrStr->m_str )
      {
         std::streamsize bytesToRead = ossim::min<std::streamsize>(
            (std::streamsize)p_nb_bytes,
            (std::streamsize)(usrStr->m_length - usrStr->m_str->tellg() ) );
         // std::streamsize left = usrStr->m_str->tellg();
         usrStr->m_str->read( (char*) p_buffer, bytesToRead );
         count = usrStr->m_str->gcount();
         
         if ( usrStr->m_str->eof() )
         {
            // Set back to front:
            usrStr->m_str->clear();
            usrStr->m_str->seekg(usrStr->m_offset, std::ios_base::beg);
         }
         else if ( !usrStr->m_str->good() )
         {
            usrStr->m_str->clear();
            usrStr->m_str->seekg(usrStr->m_offset, std::ios_base::beg);
            count = -1;
         }
      }
   }
   return count;
}

/** Callback function prototype for skip function */
static OPJ_OFF_T ossim_opj_istream_skip(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
   OPJ_OFF_T skipped = p_nb_bytes;
   opj_user_istream* usrStr = static_cast<opj_user_istream*>(p_user_data);
   if ( usrStr )
   {
      if ( usrStr->m_str )
      {
         std::streampos p1 = usrStr->m_str->tellg();
         usrStr->m_str->seekg( p_nb_bytes, std::ios_base::cur );
         std::streampos p2 = usrStr->m_str->tellg();
         if ( ( p2 - p1 ) !=  p_nb_bytes )
         {
            skipped = -1;
         }
      }
   }
   return skipped;
}

/** Callback function prototype for seek function */
static OPJ_BOOL ossim_opj_istream_seek(OPJ_OFF_T p_nb_bytes, void * p_user_data)
{
   opj_user_istream* usrStr = static_cast<opj_user_istream*>(p_user_data);
   if ( usrStr )
   {
      if ( usrStr->m_str )
      {
         usrStr->m_str->seekg( p_nb_bytes, std::ios_base::cur );
      }
   }
   return OPJ_TRUE;
}

static void ossim_opj_free_user_istream_data( void * p_user_data )
{
   opj_user_istream* usrStr = static_cast<opj_user_istream*>(p_user_data);
   if ( usrStr )
   {
      delete usrStr;
   }
   usrStr = 0;
}

bool ossim::opj_decode( std::ifstream* in,
                        const ossimIrect& rect,
                        ossim_uint32 resLevel,
                        ossim_int32 format, // OPJ_CODEC_FORMAT
                        std::streamoff fileOffset,
                        ossimImageData* tile)
{
   static const char MODULE[] = "ossimOpjDecoder::decode";

   bool status = false;

   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << "entered...\nrect: " << rect
         << "\nresLevel: " << resLevel << std::endl;
   }
   
   // Need to check for NAN in rect
   if ( in && tile && !rect.hasNans())
   {
      in->seekg( fileOffset, std::ios_base::beg );

      opj_dparameters_t param;
      opj_codec_t*      codec = 0;
      opj_image_t*      image = 0;;
      opj_stream_t*     stream = 0;

      opj_user_istream* userStream = new opj_user_istream();
      userStream->m_str = in;
      userStream->m_offset = fileOffset;

      /* Set the length to avoid an assert */
      in->seekg(0, std::ios_base::end);

      // Fix: length must be passed in for nift blocks.
      userStream->m_length = in->tellg();

      // Set back to front:
      in->clear();
      in->seekg(fileOffset, std::ios_base::beg);
      
      stream = opj_stream_default_create(OPJ_TRUE);
      if (!stream)
      {
         opj_stream_destroy(stream);
         std::string errMsg = MODULE;
         errMsg += " ERROR: opj_setup_decoder failed!";
         throw ossimException(errMsg);
      }
      
      opj_stream_set_read_function(stream, ossim_opj_istream_read);
      opj_stream_set_skip_function(stream, ossim_opj_istream_skip);
      opj_stream_set_seek_function(stream, ossim_opj_istream_seek);

      // Fix: length must be passed in for nift blocks.
      opj_stream_set_user_data_length(stream, userStream->m_length);
      
      opj_stream_set_user_data(stream, userStream,
                               ossim_opj_free_user_istream_data);

      opj_stream_set_user_data_length(stream, userStream->m_length);

      
      /* Set the default decoding parameters */
      opj_set_default_decoder_parameters(&param);

      param.decod_format = format;

      /** you may here add custom decoding parameters */
      /* do not use layer decoding limitations */
      param.cp_layer = 0;

      /* do not use resolutions reductions */
      param.cp_reduce = resLevel;

      codec = opj_create_decompress( (CODEC_FORMAT)format );

      // catch events using our callbacks and give a local context
      //opj_set_info_handler   (codec, ossim::opj_info_callback,   00);
      opj_set_info_handler   (codec, NULL,   00);
      opj_set_warning_handler(codec, ossim::opj_warning_callback,00);
      opj_set_error_handler  (codec, ossim::opj_error_callback,  00);

      // Setup the decoder decoding parameters using user parameters
      if ( opj_setup_decoder(codec, &param) == false )
      {
         opj_stream_destroy(stream);
         opj_destroy_codec(codec);
         std::string errMsg = MODULE;
         errMsg += " ERROR: opj_setup_decoder failed!";
         throw ossimException(errMsg);
      }

      // Read the main header of the codestream and if necessary the JP2 boxes.
      if ( opj_read_header(stream, codec, &image) == false )
      {
         opj_stream_destroy(stream);
         opj_destroy_codec(codec);
         opj_image_destroy(image);         
         std::string errMsg = MODULE;
         errMsg += " ERROR: opj_read_header failed!";
         throw ossimException(errMsg);
      }

      // tmp drb:
      // opj_stream_destroy(stream);
      // return;
      
      if ( opj_set_decoded_resolution_factor(codec, resLevel) == false)
      {
         opj_stream_destroy(stream);
         opj_destroy_codec(codec);
         opj_image_destroy(image);
         std::string errMsg = MODULE;
         errMsg += " ERROR:  opj_set_decoded_resolution_factor failed!";
         throw ossimException(errMsg);
      }

      //ossim_float32 res = resLevel;
      ossimIrect resRect = rect * (1 << resLevel);

      //std::cout << "resRect.ul(): " << resRect.ul()
      //          << "\nresRect.lr(): " << resRect.lr()
      //          << std::endl;

//      if ( opj_set_decode_area(codec, image, rect.ul().x, rect.ul().y,
//                               rect.lr().x+1, rect.lr().y+1) == false )
      if ( opj_set_decode_area(codec, image, resRect.ul().x, resRect.ul().y,
                               resRect.lr().x+1, resRect.lr().y+1) == false )
      {
         opj_stream_destroy(stream);
         opj_destroy_codec(codec);
         opj_image_destroy(image);
         std::string errMsg = MODULE;
         errMsg += " ERROR: opj_set_decode_area failed!";
         throw ossimException(errMsg);
      }

      // Get the decoded image:
      if ( opj_decode(codec, stream, image) == false )
      {
         opj_stream_destroy(stream);
         opj_destroy_codec(codec);
         opj_image_destroy(image);
         std::string errMsg = MODULE;
         errMsg += " ERROR: opj_decode failed!";
         throw ossimException(errMsg);
      }
      
      // ossim::print(std::cout, *image);
      if(image->icc_profile_buf) {
#if defined(OPJ_HAVE_LIBLCMS1) || defined(OPJ_HAVE_LIBLCMS2)
          color_apply_icc_profile(image); /* FIXME */
#endif
          free(image->icc_profile_buf);
          image->icc_profile_buf = NULL; image->icc_profile_len = 0;
      }

      status = ossim::copyOpjImage(image, tile);
      
#if 0
      ossim_uint32 tile_index      = 0;
      ossim_uint32 data_size       = 0;
      ossim_int32  current_tile_x0 = 0;
      ossim_int32  current_tile_y0 = 0;
      ossim_int32  current_tile_x1 = 0;
      ossim_int32  current_tile_y1 = 0;
      ossim_uint32 nb_comps        = 0;
      OPJ_BOOL go_on = 1;

      if ( opj_read_tile_header( codec,
                                 stream,
                                 &tile_index,
                                 &data_size,
                                 &current_tile_x0,
                                 &current_tile_y0,
                                 &current_tile_x1,
                                 &current_tile_y1,
                                 &nb_comps,
                                 &go_on) == false )
      {
         opj_stream_destroy(stream);
         opj_destroy_codec(codec);
         opj_image_destroy(image);
         std::string errMsg = MODULE;
         errMsg += " ERROR: opj_read_tile_header failed!";
         throw ossimException(errMsg);
      }
#endif

#if 0
      std::cout << "tile_index:      " << tile_index
                << "\ndata_size:       " << data_size
                << "\ncurrent_tile_x0: " << current_tile_x0
                << "\ncurrent_tile_y0: " << current_tile_y0
                << "\ncurrent_tile_x1: " << current_tile_x1
                << "\ncurrent_tile_y1: " << current_tile_y1
                << "\nnb_comps:        " << nb_comps
                << std::endl;
#endif

#if 0
      if ( opj_decode_tile_data(codec, tile_index,l_data,l_data_size,l_stream) == false)
      {
         opj_stream_destroy(l_stream);
         opj_destroy_codec(l_codec);
         opj_image_destroy(l_image);
         std::string errMsg = MODULE;
         errMsg += " ERROR: opj_read_tile_header failed!";
         throw ossimException(errMsg);
      }
#endif

#if 0
      
      if (opj_end_decompress(codec,stream) == false )
      {
         opj_stream_destroy(stream);
         opj_destroy_codec(codec);
         opj_image_destroy(image);
         std::string errMsg = MODULE;
         errMsg += " ERROR: opj_end_decompress failed!";
         throw ossimException(errMsg);
      } 

#endif
      
      /* Free memory */
      opj_stream_destroy(stream);
      opj_destroy_codec(codec);
      opj_image_destroy(image);

      // Tmp drb:
      if ( in->eof() )
      {
         in->clear();
      }
      in->seekg(fileOffset, std::ios_base::beg );
      
   } // Matches: if ( in && tile )

   return status;
   
} // End: ossim::opj_decode( ... )

bool ossim::copyOpjImage( opj_image* image, ossimImageData* tile )
{
   bool status = false;
   
   if ( image && tile )
   {
      if ( image->color_space == OPJ_CLRSPC_SRGB )
      {
         const ossimScalarType SCALAR = tile->getScalarType();
         if ( SCALAR == OSSIM_UINT8 )
         {
            status = ossim::copyOpjSrgbImage( ossim_uint8(0), image, tile );
         }
         else
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << "ossim::copyOpjImage WARNING!\nUnhandle scalar: "
               << SCALAR << "\n";
         }
      }
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "ossim::copyOpjImage WARNING!\nUnhandle color space: "
            << image->color_space << "\n";
      }
   }

   return status;
}

template <class T> bool ossim::copyOpjSrgbImage( T /* dummy */,
                                                 opj_image* image,
                                                 ossimImageData* tile )
{
   bool status = false;
   
   const ossim_uint32 BANDS = tile->getNumberOfBands();

   if ( image->numcomps == BANDS )
   {
      for ( ossim_uint32 band = 0; band < BANDS; ++band )
      {
         T* buf = (T*)tile->getBuf(band);
         const ossim_uint32 LINES = tile->getHeight();
         
         if ( image->comps[band].h == LINES )
         {
            ossim_uint32 offset = 0;
            for ( ossim_uint32 line = 0; line < LINES; ++line )
            {
               const ossim_uint32 SAMPS = tile->getWidth();
               if ( image->comps[band].w == SAMPS )
               {
                  for ( ossim_uint32 samp = 0; samp < SAMPS; ++samp )
                  {
                     buf[offset] = (T)(image->comps[band].data[offset]);
                     ++offset;
                  }

                  //---
                  // If we get here all the dimensions match and things sould
                  // be good.
                  //---
                  status = true;
               }
               else
               {
                  ossimNotify(ossimNotifyLevel_WARN)
                     << "ossim::copyOpjSrgbImage WARNING: sample mismatch!\n";
               }
            }
         }
         else
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << "ossim::copyOpjSrgbImage WARNING: line mismatch!\n";
         } 
      }
   }
   else
   {
      ossimNotify(ossimNotifyLevel_WARN)
               << "ossim::copyOpjSrgbImage WARNING: band mismatch!\n";
   }

   return status;
   
} // End: ossim::copyOpjSrgbImage( ... )

ossim_int32 ossim::getCodecFormat( std::istream* str )
{
   ossim_int32 result = OPJ_CODEC_UNKNOWN; // -1

   if ( str )
   {
      if ( str->good() )
      {
         const ossim_uint8 JP2_RFC3745_MAGIC[12] = 
            { 0x00,0x00,0x00,0x0c,0x6a,0x50,0x20,0x20,0x0d,0x0a,0x87,0x0a };
         const ossim_uint8 JP2_MAGIC[4] = { 0x0d,0x0a,0x87,0x0a };
         const ossim_uint8 J2K_CODESTREAM_MAGIC[4] = { 0xff,0x4f,0xff,0x51 };
         
         ossim_uint8 buf[12];
         
         // Read in the box.
         str->read((char*)buf, 12);
         
         if ( ( std::memcmp(buf, JP2_RFC3745_MAGIC, 12) == 0 ) ||
              ( std::memcmp(buf, JP2_MAGIC, 4) == 0 ) )
         {
            result = OPJ_CODEC_JP2;
         }
         else if ( std::memcmp(buf, J2K_CODESTREAM_MAGIC, 4) == 0 )
         {
            result = OPJ_CODEC_J2K;
         }
      }
   }
   
   return result;
}

std::ostream& ossim::print(std::ostream& out, const opj_codestream_info& info)
{
   // Capture stream flags since we are going to mess with them.
   std::ios_base::fmtflags f = out.flags();
   
   const int W = 20;
   
   out << std::setiosflags(std::ios_base::right) << std::setfill(' ')
       << "opj_codestream_info:\n\n"
       << std::setw(W) << "D_max: "
       << info.D_max << "\n"
       << std::setw(W) << "packno: "
       << info.packno << "\n"
       << std::setw(W) << "index_write: "
       <<info.index_write << "\n"
       << std::setw(W) << "image_w: "
       << info.image_w << "\n"
       << std::setw(W) << "image_h: "
       << info.image_h << "\n"
       << std::setw(W) << "proj: "
       << info.prog << "\n"
       << std::setw(W) << "tile_x: "
       << info.tile_x << "\n"
       << std::setw(W) << "tile_y: "
       << info.tile_y << "\n"
       << std::setw(W) << "tile_Ox: "
       << info.tile_Ox << "\n"
       << std::setw(W) << "tile_Oy: "
       << info.tile_Oy << "\n"
       << std::setw(W) << "tw: "
       << info.tw << "\n"
       << std::setw(W) << "th: "
       << info.th << "\n"
       << std::setw(W) << "numcomps: "
       << info.numcomps << "\n"
       << std::setw(W) << "numlayers: "
       << info.numlayers << "\n";

   for (int i = 0; i < info.numcomps; ++i)
   {
      std::string s = "numdecompos[";
      s += ossimString::toString(i).string();
      s += "]: ";
      out << std::setw(W) << s << info.numdecompos[i] << "\n";
   }

   out << std::setw(W) << "marknum: "
       << info.marknum << "\n"
      // << std::setw(W) << "marker: " << info.marker << "\n"
       << std::setw(W) << "maxmarknum: "
       << info.maxmarknum << "\n"
       << std::setw(W) << "main_head_start: "
       << info.main_head_start << "\n"
       << std::setw(W) << "main_head_end: "
       << info.main_head_end << "\n"
       << std::setw(W) << "codestream_size: "
       << info.codestream_size << "\n"
      // << "tile: " << info.tile
       << std::endl;

   // Set the flags back.
   out.flags(f);

   return out;
}

std::ostream& ossim::print(std::ostream& out, const opj_cparameters& param)
{
   // Capture stream flags since we are going to mess with them.
   std::ios_base::fmtflags f = out.flags();

   const int W = 24;
   int i;

   out << std::setiosflags(std::ios_base::left) << std::setfill(' ')
       << "opj_cparameters:\n\n"
      
       << std::setw(W) << "tile_size_on:"
       << param.tile_size_on << "\n"

       << std::setw(W) << "cp_tx0: "
       << param.cp_tx0 << "\n"

       << std::setw(W) << "cp_ty0: "
       << param.cp_ty0 << "\n"

       << std::setw(W) << "cp_tdx: "
       << param.cp_tdx << "\n"

       << std::setw(W) << "cp_tdy: "
       << param.cp_tdy << "\n"

       << std::setw(W) << "cp_disto_alloc: "
       << param.cp_disto_alloc << "\n"

       << std::setw(W) << "cp_fixed_alloc: "
       << param.cp_fixed_alloc << "\n"

       << std::setw(W) << "cp_fixed_quality: "
       << param.cp_fixed_quality << "\n"

       << "cp_matrice:\n"

       << "cp_comment:\n"

       << std::setw(W) << "csty:"
       << param.csty << "\n"
      
       << std::setw(W) << "prog_order: "
       << getProgressionOrderString( (ossim_int32)param.prog_order ) << "\n"

       << "POC:\n"
      
       << std::setw(W) << "numpocs:"
       << param.numpocs << "\n"
      
       << std::setw(W) << "tcp_numlayers:"
       << param.tcp_numlayers << "\n";

   for ( i = 0; i < param.tcp_numlayers; ++i )
   {
      out << "tcp_rate[" << i << std::setw(14)<< "]: "<< std::setw(4) << param.tcp_rates[i] << "\n"
          << "tcp_distoratio[" << i << std::setw(8) << "]: " << std::setw(4) << param.tcp_distoratio[i] << "\n";
   }

   out << std::setw(W) << "numresolution:"
       << param.numresolution << "\n"

       << std::setw(W) << "cblockw_init:"
       << param.cblockw_init << "\n"
      
       << std::setw(W) << "cblockh_init:"
       << param.cblockh_init << "\n"
      
       << std::setw(W) << "mode:"
       << param.mode << "\n"
      
       << std::setw(W) << "irreversible:"
       << param.irreversible << "\n"
      
       << std::setw(W) << "roi_compn:"
       << param.roi_compno << "\n"
      
       << std::setw(W) << "roi_shift:"
       << param.roi_shift << "\n"
      
       << std::setw(W) << "res_spec:"
       << param.res_spec << "\n"
      
       << "prcw_init:\n"
      // << param.prcw_init << "\n"
      
       << "prch_init:\n"
      // << param.prcw_init << "\n"

      // << std::setw(W) << "infile:"
      //  << param.infile << "\n"

       << std::setw(W) << "outfile:"
       << param.outfile << "\n"

      // << std::setw(W) << "index_on:"
      //  << param.index_on << "\n"

      // << std::setw(W) << "index:"
      //  << param.index << "\n"

       << std::setw(W) << "image_offset_x0:"
       << param.image_offset_x0 << "\n"

       << std::setw(W) << "image_offset_y0:"
       << param.image_offset_y0 << "\n"
      
       << std::setw(W) << "subsampling_dx:"
       << param.subsampling_dx << "\n"

       << std::setw(W) << "image_offset_dy:"
       << param.subsampling_dy << "\n"
      
       << std::setw(W) << "decod_format:"
       << param.decod_format << "\n"
      
       << std::setw(W) << "cod_format:"
       << param.cod_format << "\n"
      
       << std::setw(W) << "jpwl_epc_on:"
       << param.jpwl_epc_on << "\n"
      
       << std::setw(W) << "jpwl_hprot_MH:"
       << param.jpwl_hprot_MH << "\n"
      
       << "jpwl_hprot_TPH_tileno:\n"

       << "jpwl_pprot_TPH:\n"

       << "jpwl_pprot_tileno:\n"
      
       << "jpwl_pprot_packno:\n"

       << "jpwl_pprot:\n"
      
       << std::setw(W) << "jpwl_sens_size:"
       << param.jpwl_sens_size << "\n"

       << std::setw(W) << "jpwl_sens_addr:"
       << param.jpwl_sens_addr << "\n"
     
       << std::setw(W) << "jpwl_sens_range:"
       << param.jpwl_sens_range << "\n"
     
       << std::setw(W) << "jpwl_sens_MH:"
       << param.jpwl_sens_MH << "\n"
     
       << "jpwl_sens_TPH_tileno:\n"

       << "jpwl_sens_TPH:\n"

       << "cp_cinema:\n"

       << std::setw(W) << "max_comp_size:"
       << param.max_comp_size << "\n"

       << std::setw(W) << "cp_rsiz:"
       << param.cp_rsiz << "\n"

       << std::setw(W) << "tp_on:"
       << param.tp_on << "\n"

       << std::setw(W) << "tp_flag:"
       << param.tp_flag << "\n"

       << std::setw(W) << "tcp_mct:"
       << int(param.tcp_mct) << "\n"

       << std::setw(W) << "jpip_on:"
       << param.jpip_on << "\n"

       << "mct_data:\n"

       << std::setw(W) << "max_cs_size:"
       << param.max_cs_size << "\n"

       << std::setw(W) << "rsiz:"
       << param.rsiz << "\n"

       << std::endl;

   // Set the flags back.
   out.flags(f);

   return out;
}

std::ostream& ossim::print(std::ostream& out, const opj_dparameters& param)
{
   // Capture stream flags since we are going to mess with them.
   std::ios_base::fmtflags f = out.flags();

   const int W = 20;

   out << std::setiosflags(std::ios_base::right) << std::setfill(' ')
       << "opj_dparameters:\n\n"
       << std::setw(W) << "cp_reduce: "
       << param.cp_reduce << "\n"
       << std::setw(W) << "cp_layer: "
       << param.cp_layer << "\n"
       << std::setw(W) << "infile: "
       << param.infile << "\n"
       << std::setw(W) << "outfile: "
       << param.outfile << "\n"
       << std::setw(W) << "decod_format: "
       << param.decod_format << "\n"
       << std::setw(W) << "cod_format: "
       << param.cod_format << "\n"
       << std::setw(W) << "jpwl_correct: "
       << param.jpwl_correct << "\n"
       << std::setw(W) << "jpwl_exp_comps: "
       << param.jpwl_exp_comps << "\n"
       << std::setw(W) << "jpwl_max_tiles: "
       << param.jpwl_max_tiles << "\n"
       << std::setw(W) << "cp_limit_decoding: "
      // << param.cp_limit_decoding fix (drb)
       << std::endl;
   // Set the flags back.
   out.flags(f);

   return out;
}

std::ostream& ossim::print(std::ostream& out, const opj_image& image)
{
   // Capture stream flags since we are going to mess with them.
   std::ios_base::fmtflags f = out.flags();

   const int W = 20;

   out << std::setiosflags(std::ios_base::right) << std::setfill(' ')
       << "opj_image:\n\n"
       << std::setw(W) << "x0: "
       << image.x0 << "\n"
       << std::setw(W) << "y0: "
       << image.y0 << "\n"
       << std::setw(W) << "x1: "
       << image.x1 << "\n"
       << std::setw(W) << "y1: "
       << image.y1 << "\n"
       << std::setw(W) << "numcomps: "
       << image.numcomps << "\n"
       << std::setw(W) << "color_space: ";
   if ( image.color_space == OPJ_CLRSPC_UNKNOWN ) out << "UNKNOWN\n";
   else if (  image.color_space == OPJ_CLRSPC_UNSPECIFIED) out << "UNSPECIFIED\n";
   else if (  image.color_space == OPJ_CLRSPC_SRGB) out << "SRGB\n";
   else if (  image.color_space == OPJ_CLRSPC_GRAY) out << "GRAY\n";
   else if (  image.color_space == OPJ_CLRSPC_SYCC) out << "SYCC\n";
   else if (  image.color_space == OPJ_CLRSPC_EYCC) out << "EYCC\n";
   else if (  image.color_space == OPJ_CLRSPC_CMYK) out << "CMYK\n";

   for (ossim_uint32 i = 0; i < image.numcomps; ++i)
   {
      std::string s = "comps[";
      s += ossimString::toString(i).string();
      s += "]: ";
      out << s << std::endl;
         
      print(out, (image.comps[i]));
   }

   out << std::endl;
      
   // Set the flags back.
   out.flags(f);

   return out;
}

std::ostream& ossim::print(std::ostream& out, const opj_image_comp& comp)
{
   // Capture stream flags since we are going to mess with them.
   std::ios_base::fmtflags f = out.flags();

   const int W = 20;

   out << std::setiosflags(std::ios_base::right) << std::setfill(' ')
       << "opj_image_comp:\n\n"
       << std::setw(W) << "dx: "
       << comp.dx << "\n"
       << std::setw(W) << "dy: "
       << comp.dy << "\n"
       << std::setw(W) << "w: "
       << comp.w << "\n"
       << std::setw(W) << "h: "
       << comp.h << "\n"
       << std::setw(W) << "x0: "
       << comp.x0 << "\n"
       << std::setw(W) << "y0: "
       << comp.y0 << "\n"
       << std::setw(W) << "prec: "
       << comp.prec << "\n"
       << std::setw(W) << "bpp: "
       << comp.bpp << "\n" 
       << std::setw(W) << "sgnd: "
       << comp.sgnd << "\n"
       << std::setw(W) << "resno_decoded: "
       << comp.resno_decoded << "\n"
       << std::setw(W) << "factor: "
       << comp.factor
       << std::endl;
      
   // Set the flags back.
   out.flags(f);

   return out;
}

std::string ossim::getProgressionOrderString( int prog_order )
{
   std::string result = "";
   switch (prog_order)
   {
      case OPJ_LRCP:
      {
         result = "LRCP";
         break;
      }
      case OPJ_RLCP:
      {
         result = "RLCP";
         break;
      }
      case OPJ_RPCL:
      {
         result = "RPCL";
         break;
      }
      case OPJ_PCRL:
      {
         result = "PCRL";
         break;
      }
      case OPJ_CPRL:
      {
         result = "CPRL";
         break;
      }
      default:
      {
         result = "UNKOWN";
         break;
      }
   }
   
   return result;
}

