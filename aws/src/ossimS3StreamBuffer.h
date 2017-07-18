//---
//
// License: MIT
//
// Author: Garrett Potts
//
// Description:
// 
// OSSIM Amazon Web Services (AWS) S3 streambuf declaration.
//
//---
// $Id$

#ifndef ossimS3StreamBuffer_HEADER
#define ossimS3StreamBuffer_HEADER 1

#include <ossim/base/ossimConstants.h>
#include <aws/s3/S3Client.h>

#include "S3StreamDefaults.h"
#include <iostream>
#include <vector>

// namespace Aws
// {
//    namespace S3
//    {
//       class S3Client;
//    }
// }

class ossimKeywordlist;

namespace ossim{
class  S3StreamBuffer : public std::streambuf
{
public:
    class blockInfo
    {
      public:
        blockInfo()
        :m_startByte(0),
        m_currentByte(0),
        m_endByte(0)
        {

        }

        void setBytes(ossim_int64 startByte, ossim_int64 currentByte, ossim_int64 endByte)
        {
          m_startByte   = startByte;
          m_currentByte = currentByte;
          m_endByte     = endByte;
        }
        void setCurrentByte(ossim_int64 currentByte){m_currentByte = currentByte;}
        ossim_int64 getOffsetFromStart()const{return (m_currentByte-m_startByte);}
        ossim_int64 getOffsetFromEnd()const{return (m_endByte-m_currentByte);}
        const ossim_int64& getStartByte()const{return m_startByte;}
        const ossim_int64& getEndByte()const{return m_endByte;}
        const ossim_int64& getCurrentByte()const{return m_currentByte;}
        bool withinWindow()const{return ((m_currentByte>=m_startByte) && (m_currentByte < m_endByte));}


        ossim_int64 m_startByte;
        ossim_int64 m_endByte;
        ossim_int64 m_currentByte;
    };
   // S3StreamBuffer(ossim_int64 blockSize=4096);
   S3StreamBuffer(ossim_int64 blockSize=ossim::S3StreamDefaults::m_readBlocksize);

   S3StreamBuffer* open (const char* connectionString,                         
                         const ossimKeywordlist& options,
                         std::ios_base::openmode mode);
   S3StreamBuffer* open (const std::string& connectionString, 
                         const ossimKeywordlist& options,
                         std::ios_base::openmode mode);
   
   bool is_open() const
   {
      return m_opened;
   }

   virtual ~S3StreamBuffer();

   /**
    * @return Size of file in bytes.
    */
   ossim_uint64 getFileSize() const;

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

   //Aws::S3::S3Client* m_client;
   mutable std::shared_ptr<Aws::S3::S3Client> m_client;
   std::string m_bucket;
   std::string m_key;
   std::vector<char> m_buffer;
   ossim_int64 m_bufferActualDataSize;
   ossim_int64 m_currentBlockPosition;
   char* m_bufferPtr;
   ossim_int64 m_fileSize;
   bool m_opened;
   blockInfo m_blockInfo;
   //std::ios_base::openmode m_mode;
};

}

#endif
