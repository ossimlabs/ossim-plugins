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

#ifndef ossimOpjCommon_HEADER
#define ossimOpjCommon_HEADER 1

#include <ossim/base/ossimConstants.h>
#include <iosfwd>
#include <string>

// Forward declarations:
class ossimImageData;
class ossimIrect;
struct opj_codestream_info;
struct opj_cparameters;
struct opj_dparameters;
struct opj_image;
struct opj_image_comp;

namespace ossim
{
   /** @brief Callback method for errors. */
   void opj_error_callback(const char* msg, void* /* client_data */);

   /** @brief Callback method for warnings. */
   void opj_warning_callback(const char* msg, void* /* client_data */ );

   /** @brief Callback method for info. */
   void opj_info_callback(const char* msg, void* /* client_data */);

   bool opj_decode( std::ifstream* in,
                    const ossimIrect& rect,
                    ossim_uint32 resLevel,
                    ossim_int32 format, // OPJ_CODEC_FORMAT
                    std::streamoff fileOffset, // for nitf
                    ossimImageData* tile
                    );
   
   bool copyOpjImage( opj_image* image, ossimImageData* tile );
   
   template <class T> bool copyOpjSrgbImage( T dummy,
                                             opj_image* image,
                                             ossimImageData* tile );
   
   /**
    * Gets codec format from magic number.
    * @return OPJ_CODEC_J2K = 0, OPJ_CODEC_JP2 = 2, OPJ_CODEC_UNKNOWN = -1
    */
   ossim_int32 getCodecFormat( std::istream* str );

   /**
    * Prints codestream info from openjpeg struct opj_codestream_info.
    * 
    * @return std::ostream&
    */
   std::ostream& print(std::ostream& out, const opj_codestream_info& info);
   
   /**
    * Prints compression parameters from openjpeg struct opj_cparameters.
    * 
    * @return std::ostream&
    */
   std::ostream& print(std::ostream& out, const opj_cparameters& param);

   /**
    * Prints decode parameters from openjpeg struct opj_dparameters.
    * 
    * @return std::ostream&
    */
   std::ostream& print(std::ostream& out, const opj_dparameters& param);

   /**
    * Prints opj_image structure.
    * 
    * @return std::ostream&
    */
   std::ostream& print(std::ostream& out, const opj_image& image);

   /**
    * Prints opj_image_comp structure.
    * 
    * @return std::ostream&
    */
   std::ostream& print(std::ostream& out, const opj_image_comp& comp);

   std::string getProgressionOrderString( ossim_int32 prog_order );

} // End of namespace ossim

#endif /* ossimOpjCommon_HEADER */
