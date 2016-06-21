//----------------------------------------------------------------------------
// License:  See top level LICENSE.txt file.
//
// Author:  David Burken, original code from Thomas G. Lane
//
// Description:
//
// Code interfaces to use with jpeg-6b library to read a jpeg image from
// memory. Specialized for 12 bit compiled jpeg library.
//----------------------------------------------------------------------------
// $Id$
#ifndef ossimJpegMemSrc12_HEADER
#define ossimJpegMemSrc12_HEADER 1

#include <ossim/base/ossimConstants.h> /** for OSSIM_DLL export macro */
#include <cstdlib>                     /** For size_t.  */

// Forword declarations to avoid jpeg includes in the header.
struct jpeg12_decompress_struct;

extern "C"
{
/**
 * @brief Method which uses memory instead of a FILE* to read from.
 * @note Used in place of "jpeg_stdio_src(&cinfo, infile)".
 */
OSSIM_DLL void ossimJpegMemorySrc12 ( jpeg12_decompress_struct* cinfo,
                                      const ossim_uint8* buffer,
                                      std::size_t bufsize );
}

#endif /* #ifndef ossimJpegMemSrc12_HEADER */
