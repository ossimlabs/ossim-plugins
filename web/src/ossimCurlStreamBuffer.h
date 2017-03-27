//---
//
// License: MIT
//
// Author: Garrett Potts
//
// Description:
// 
// Curl HTTP stream definitions.
//
//---
// $Id$

#ifndef ossimCurlStreamBuffer_HEADER
#define ossimCurlStreamBuffer_HEADER 1

#include <ossim/base/ossimConstants.h>
#include <iostream>
#include "CurlStreamDefaults.h"
#include "ossimCurlHttpRequest.h"
#include <vector>
namespace ossim{
class  CurlStreamBuffer : public std::streambuf
{
public:
   // S3StreamBuffer(ossim_int64 blockSize=4096);
   CurlStreamBuffer(ossim_int64 blockSize=ossim::CurlStreamDefaults::m_readBlocksize);

   CurlStreamBuffer* open (const char* connectionString,  std::ios_base::openmode mode);
   CurlStreamBuffer* open (const std::string& connectionString, std::ios_base::openmode mode);
   
   bool is_open() const
   {
      return m_opened;
   }
   virtual ~CurlStreamBuffer()
   {
      
   }

   /**
    * @return Size of file in bytes.
    */
   ossim_int64 getFileSize() const;

   /**
    * @return Size of block buffer in bytes.
    */
   ossim_uint64 getBlockSize() const;
   
protected:
   //virtual int_type pbackfail(int_type __c  = traits_type::eof());
   virtual pos_type seekoff(off_type offset, std::ios_base::seekdir dir,
                            std::ios_base::openmode __mode = std::ios_base::in | std::ios_base::out);
   virtual pos_type seekpos(pos_type pos, 
                            std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out);
   virtual std::streamsize xsgetn(char_type* s, std::streamsize n);
   virtual int underflow();
   
   void clearAll();
   
   ossim_int64 getBlockIndex(ossim_int64 byteOffset)const;
   ossim_int64 getBlockOffset(ossim_int64 byteOffset)const;
   bool getBlockRangeInBytes(ossim_int64 blockIndex, 
                             ossim_int64& startRange, 
                             ossim_int64& endRange)const;
   
   bool loadBlock(ossim_int64 absolutePosition);
   
   //void adjustForSeekgPosition(ossim_int64 seekPosition);
   ossim_int64 getAbsoluteByteOffset()const;
   bool withinWindow()const;

  // Aws::S3::S3Client m_client;
   std::string m_bucket;
   std::string m_key;
   std::vector<char> m_buffer;
   ossim_int64 m_bufferActualDataSize;
   ossim_int64 m_currentBlockPosition;
   char* m_bufferPtr;
   ossim_int64 m_fileSize;
   bool m_opened;
   ossimCurlHttpRequest m_curlHttpRequest;
   //std::ios_base::openmode m_mode;
};

}

#endif
