//----------------------------------------------------------------------------
//
// License: MIT
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author: Refactor from Mingjie Su's ossimNitfTileSource_12.
//
// Description:
//
// Class declaration for reader of NITF images with 12 bit jpeg compressed
// blocks using libjpeg-turbo library compiled specifically with
// "--with-12bit" compiler option.
//
//----------------------------------------------------------------------------
// $Id

#ifndef ossimJpeg12NitfReader_HEADER
#define ossimJpeg12NitfReader_HEADER 1

#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/imaging/ossimNitfTileSource.h>

class ossimImageData;
class ossimNitfImageHeader;
struct jpeg12_decompress_struct;

/**
 * @brief ossimJpeg12NitfReader class for reading NITF images with 12 bit jpeg
 * compressed blocks using libjpeg-turbo library compiled specifically with
 * "--with-12bit" compiler option.
 */
class OSSIM_PLUGINS_DLL ossimJpeg12NitfReader : public ossimNitfTileSource
{
public:


protected:
   /**
    * @param hdr Pointer to image header.
    * @return true if reader can uncompress nitf.
    * */
   virtual bool canUncompress(const ossimNitfImageHeader* hdr) const;

      /**
    * @brief Uncompresses a jpeg block using the jpeg-6b library.
    * @param x sample location in image space.
    * @param y line location in image space.
    * @return true on success, false on error.
    */
   virtual bool uncompressJpegBlock(ossim_uint32 x, ossim_uint32 y);

   /**
    * @brief Loads one of the default tables based on COMRAT value.
    *
    * @return true if comrat had valid table value(1-5) and table was loaded,
    * false if COMRAT value did not contain a valid value.  Will also return
    * false if there is already a table loaded.
    * 
    * @note COMRAT is nitf compression rate code field:
    * -  "00.2" == default compression table 2
    * -  "00.5" == default compression table 5 and so on.
    */
   bool loadJpeg12QuantizationTables(jpeg12_decompress_struct& cinfo) const;

   /**
    * @brief Loads default huffman tables.
    *
    * @return true success, false on error.
    */
   bool loadJpeg12HuffmanTables(jpeg12_decompress_struct& cinfo) const;
   

TYPE_DATA   
};

#endif /* #ifndef ossimJpeg12NitfReader_HEADER */
