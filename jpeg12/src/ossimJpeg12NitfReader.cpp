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

#include "ossimJpeg12NitfReader.h"
#include "ossimJpegMemSrc12.h"
#include <ossim/base/ossimTrace.h>
#include <ossim/imaging/ossimJpegDefaultTable.h>
#include <ossim/support_data/ossimNitfImageHeader.h>
#include <fstream>
#include <vector>

/**
 * jpeg library includes missing dependent includes.
 */
#include <cstdio>                /** For FILE* in jpeg code.   */
#include <cstdlib>               /** For size_t in jpeg code.  */
#include <csetjmp>               /** For jmp_buf in jpeg code. */
#include <jpeg12/jpeglib.h>      /** For jpeg stuff. */
#include <jpeg12/jerror.h>

static ossimTrace traceDebug("ossimJpeg12NitrReader:debug");

RTTI_DEF1_INST(ossimJpeg12NitfReader,
               "ossimJpeg12NitfReader",
               ossimNitfTileSource)

/** @brief Extended error handler struct for jpeg code. */
struct OSSIM_DLL ossimJpegErrorMgr12
{
   struct jpeg12_error_mgr pub;	/* "public" fields */
   jmp_buf setjmp_buffer;	/* for return to caller */
};
typedef struct ossimJpegErrorMgr12* ossimJpegErrorPtr12;

void ossimJpegErrorExit12 (jpeg12_common_struct* cinfo)
{
   /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
   ossimJpegErrorPtr12 myerr = (ossimJpegErrorPtr12) cinfo->err;
   
   /* Always display the message. */
   /* We could postpone this until after returning, if we chose. */
   (*cinfo->err->output_message) (cinfo);
   
   /* Return control to the setjmp point */
   longjmp(myerr->setjmp_buffer, 1);
}

bool ossimJpeg12NitfReader::canUncompress(const ossimNitfImageHeader* hdr) const
{
   bool result = false;
   if (hdr)
   {
      // C3 = jpeg.  We only do 12 bit jpeg.
      if ( (hdr->getCompressionCode() == "C3") && (hdr->getBitsPerPixelPerBand() == 12) )
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

bool ossimJpeg12NitfReader::uncompressJpegBlock(ossim_uint32 x, ossim_uint32 y)
{
   ossim_uint32 blockNumber = getBlockNumber( ossimIpt(x,y) );

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimJpeg12NitrReader::uncompressJpegBlock DEBUG:"
         << "\nblockNumber:  " << blockNumber
         << std::endl;
   }

   //---
   // Logic to hold off on scanning for offsets until a block is actually needed
   // to speed up loads for things like ossim-info that don't actually read
   // pixel data.
   //---
   if ( m_jpegOffsetsDirty )
   {
      if ( scanForJpegBlockOffsets() )
      {
         m_jpegOffsetsDirty = false;
      }
      else
      {
         ossimNotify(ossimNotifyLevel_FATAL)
            << "ossimJpeg12NitrReader::uncompressJpegBlock scan for offsets error!"
            << "\nReturning error..." << std::endl;
         theErrorStatus = ossimErrorCodes::OSSIM_ERROR;
         return false;
      }
   }
   
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "\noffset to block: " << theNitfBlockOffset[blockNumber]
         << "\nblock size: " << theNitfBlockSize[blockNumber]
         << std::endl;
   }
   
   // Seek to the block.
   theFileStr->seekg(theNitfBlockOffset[blockNumber], std::ios::beg);
   
   // Read the block into memory.
   std::vector<ossim_uint8> compressedBuf(theNitfBlockSize[blockNumber]);
   if (!theFileStr->read((char*)&(compressedBuf.front()),
                        theNitfBlockSize[blockNumber]))
   {
      theFileStr->clear();
      ossimNotify(ossimNotifyLevel_FATAL)
         << "ossimJpeg12NitrReader::uncompressJpegBlock Read Error!"
         << "\nReturning error..." << std::endl;
      return false;
   }

   jpeg12_decompress_struct cinfo;

   ossimJpegErrorMgr12 jerr;

   cinfo.err = jpeg12_std_error(&jerr.pub);
   
   jerr.pub.error_exit = ossimJpegErrorExit12;

   /* Establish the setjmp return context for my_error_exit to use. */
   if (setjmp(jerr.setjmp_buffer))
   {
      jpeg12_destroy_decompress(&cinfo);
   
      return false;
   }

   jpeg12_CreateDecompress(&cinfo, JPEG12_LIB_VERSION, sizeof(cinfo));
  
   //---
   // Step 2: specify data source.  In this case we will uncompress from
   // memory so we will use "ossimJpegMemorySrc" in place of " jpeg_stdio_src".
   //---
   ossimJpegMemorySrc12( &cinfo,
                         &(compressedBuf.front()),
                         static_cast<size_t>(theReadBlockSizeInBytes) );

   /* Step 3: read file parameters with jpeg_read_header() */
  
   jpeg12_read_header(&cinfo, TRUE);
  
   
   // Check for Quantization tables.
   if (cinfo.quant_tbl_ptrs[0] == NULL)
   {
      // This will load table specified in COMRAT field.
      if (loadJpeg12QuantizationTables(cinfo) == false)
      {
         jpeg12_destroy_decompress(&cinfo);
         return false;
      }
   }

   // Check for huffman tables.
   if (cinfo.ac_huff_tbl_ptrs[0] == NULL)
   {
      // This will load default huffman tables into .
      if (loadJpeg12HuffmanTables(cinfo) == false)
      {
         jpeg12_destroy_decompress(&cinfo);
         return false;
      }
   }

   /* Step 4: set parameters for decompression */
   
   /* In this example, we don't need to change any of the defaults set by
    * jpeg_read_header(), so we do nothing here.
    */

   /* Step 5: Start decompressor */
   
   jpeg12_start_decompress(&cinfo);
  
   const ossim_uint32 SAMPLES = cinfo.output_width;

   //---
   // Note: Some nitf will be tagged with a given number of lines but the last
   // jpeg block may go beyond that to a complete block.  So it you clamp to
   // last line of the nitf you will get a libjpeg error:
   // 
   // "Application transferred too few scanlines"
   //
   // So here we will always read the full jpeg block even if it is beyond the
   // last line of the nitf.
   //---
   const ossim_uint32 LINES_TO_READ =
      std::min(static_cast<ossim_uint32>(theCacheSize.y), cinfo.output_height);

   /* JSAMPLEs per row in output buffer */
   const ossim_uint32 ROW_STRIDE = SAMPLES * cinfo.output_components;

#if 0 /* Please leave for debug. drb */
   static bool TRACED = false;
   if ( !TRACED )
   {
      std::cout << "theCacheTile:\n" << *(theCacheTile.get())
           << "\nSAMPLES:       " << SAMPLES
           << "\nLINES_TO_READ: " << LINES_TO_READ
           << "\nROW_STRIDE:    " << ROW_STRIDE
           << "\nJPEG12_BITS_IN_JSAMPLE: " << JPEG12_BITS_IN_JSAMPLE
           << "\nsizeof jsamp: " << sizeof(JSAMPLE)
           << "\ndata_precision: " << cinfo.data_precision
           << "\ntotal_iMCU_rows: " << cinfo.total_iMCU_rows
           << endl;

      TRACED = true;
   }
#endif

   if ( (SAMPLES < theCacheTile->getWidth() ) ||
        (LINES_TO_READ < theCacheTile->getHeight()) )
   {
      theCacheTile->makeBlank();
   }

   if ( (SAMPLES > theCacheTile->getWidth()) ||
        (LINES_TO_READ > theCacheTile->getHeight()) )
   {
      jpeg12_finish_decompress(&cinfo);
      jpeg12_destroy_decompress(&cinfo);

      return false;
   }
   
   // Get pointers to the cache tile buffers.
   std::vector<ossim_uint16*> destinationBuffer(theNumberOfInputBands);
   ossim_uint32 band = 0;
   for (band = 0; band < theNumberOfInputBands; ++band)
   {
      destinationBuffer[band] = theCacheTile->getUshortBuf(band);
   }

   std::vector<ossim_uint16> lineBuffer(ROW_STRIDE);
   // std::vector<ossim_uint8> lineBuffer(ROW_STRIDE);
   JSAMPROW jbuf[1];
   jbuf[0] = (JSAMPROW) &(lineBuffer.front());

   while (cinfo.output_scanline < LINES_TO_READ)
   {
      // Read a line from the jpeg file.
      jpeg12_read_scanlines(&cinfo, jbuf, 1);

      ossim_uint32 index = 0;
      for (ossim_uint32 sample = 0; sample < SAMPLES; ++sample)         
      {
         for (band = 0; band < theNumberOfInputBands; ++band)
         {
            destinationBuffer[band][sample] = lineBuffer[index];
            ++index;
         }
      }

      for (band = 0; band < theNumberOfInputBands; ++band)
      {
         destinationBuffer[band] += theCacheSize.x;         
      }
   }

   jpeg12_finish_decompress(&cinfo);
   jpeg12_destroy_decompress(&cinfo);
 
   return true;
}

bool ossimJpeg12NitfReader::loadJpeg12QuantizationTables(
   jpeg12_decompress_struct& cinfo) const
{
   const ossimNitfImageHeader* hdr = getCurrentImageHeader();
   if (!hdr)
   {
      return false;
   }

   ossimString comrat = hdr->getCompressionRateCode();
   ossim_uint32 tableIndex = 0;
   if (comrat.size() >= 4)
   {
      // COMRAT string like: "00.2" = use table 2. (between 1 and 5).
      ossimString s;
      s.push_back(comrat[static_cast<std::string::size_type>(3)]);
      ossim_int32 comTbl = s.toInt32();
      if ( (comTbl > 0) && (comTbl < 6) )
      {
         tableIndex = comTbl-1;
      }
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "ossimJpeg12NitrReader::loadJpegQuantizationTables WARNING\n"
            << "\nNo quantization tables specified!"
            << std::endl;
         return false;  
      }
   }

   cinfo.quant_tbl_ptrs[0] = jpeg12_alloc_quant_table((j12_common_ptr) &cinfo);

   JQUANT_TBL* quant_ptr = cinfo.quant_tbl_ptrs[0]; // quant_ptr is JQUANT_TBL*

   for (ossim_int32 i = 0; i < 64; ++i)
   {
      /* Qtable[] is desired quantization table, in natural array order */
      quant_ptr->quantval[i] = QTABLE_ARRAY[tableIndex][i];
   }
   return true;
}

bool ossimJpeg12NitfReader::loadJpeg12HuffmanTables(jpeg12_decompress_struct& cinfo) const
{
   if ( (cinfo.ac_huff_tbl_ptrs[0] != NULL) &&
        (cinfo.dc_huff_tbl_ptrs[0] != NULL) )
   {
      return false;
   }

   cinfo.ac_huff_tbl_ptrs[0] = jpeg12_alloc_huff_table((j12_common_ptr)&cinfo);
   cinfo.dc_huff_tbl_ptrs[0] = jpeg12_alloc_huff_table((j12_common_ptr)&cinfo);

   ossim_int32 i;
   JHUFF_TBL* huff_ptr;

   // Copy the ac tables.
   huff_ptr = cinfo.ac_huff_tbl_ptrs[0]; /* huff_ptr is JHUFF_TBL* */     
   for (i = 0; i < 16; ++i) 
   {
      // huff_ptr->bits is array of 17 bits[0] is unused; hence, the i+1
      huff_ptr->bits[i+1] = AC_BITS[i]; 
   }

   for (i = 0; i < 256; ++i)
   {
      huff_ptr->huffval[i] = AC_HUFFVAL[i];
   }

   // Copy the dc tables.
   huff_ptr = cinfo.dc_huff_tbl_ptrs[0]; /* huff_ptr is JHUFF_TBL* */
   for (i = 0; i < 16; ++i)
   {
      // huff_ptr->bits is array of 17 bits[0] is unused; hence, the i+1
      huff_ptr->bits[i+1] = DC_BITS[i];
   }

   for (i = 0; i < 256; i++)
   {
      /* symbols[] is the list of Huffman symbols, in code-length order */
      huff_ptr->huffval[i] = DC_HUFFVAL[i];
   }
   return true;
}
