//---
//
// License: MIT
//
// Author: Garrett Potts
//
// Description:
//
// OSSIM Amazon Web Services (AWS) S3 streambuf definition.
//
//---
// $Id$

#include "ossimS3StreamBuffer.h"
#include "ossimAwsStreamFactory.h"
#include "S3HeaderCache.h"

#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimPreferences.h>
#include <ossim/base/ossimUrl.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimTimer.h>

#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/GetObjectResult.h>
#include <aws/s3/model/HeadObjectRequest.h>
#include <aws/core/Aws.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/http/HttpRequest.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <cstdio>  /* for EOF */
#include <cstring> /* for memcpy */
#include <ios>
#include <iostream>
#include <streambuf>
#include <sstream>
#include <ctime>
#include <vector>

//static const char* KEY = "test-file.txt";
//static const char* BUCKET = "ossimlabs";

using namespace Aws::S3;
using namespace Aws::S3::Model;
static ossimTrace traceDebug("ossimS3StreamBuffer:debug");

ossim::S3StreamBuffer::S3StreamBuffer(ossim_int64 blockSize)
    : m_bucket(""),
      m_key(""),
      m_buffer(blockSize),
      m_bufferActualDataSize(0),
      m_currentBlockPosition(-1),
      m_bufferPtr(0),
      m_fileSize(0),
      m_opened(false)
{
   Aws::Client::ClientConfiguration config;

   // Look for AWS S3 regionn override:
   std::string region = ossimPreferences::instance()->preferencesKWL().findKey(std::string("ossim.plugins.aws.s3.region"));
   if (region.size())
   {
      config.region = region.c_str();
   }
   m_client = ossim::AwsStreamFactory::instance()->getSharedS3Client();
   setg(m_bufferPtr, m_bufferPtr, m_bufferPtr);
}

ossim::S3StreamBuffer::~S3StreamBuffer()
{
}

ossim_int64 ossim::S3StreamBuffer::getBlockIndex(ossim_int64 byteOffset) const
{
   ossim_int64 blockNumber = -1;

   if (byteOffset < (ossim_int64)m_fileSize)
   {
      if (m_buffer.size() > 0)
      {
         blockNumber = byteOffset / m_buffer.size();
      }
   }

   return blockNumber;
}

ossim_int64 ossim::S3StreamBuffer::getBlockOffset(ossim_int64 byteOffset) const
{
   ossim_int64 blockOffset = -1;

   if (m_buffer.size() > 0)
   {
      blockOffset = byteOffset % m_buffer.size();
   }

   return blockOffset;
}

bool ossim::S3StreamBuffer::getBlockRangeInBytes(ossim_int64 blockIndex,
                                                 ossim_int64 &startRange,
                                                 ossim_int64 &endRange) const
{
   bool result = false;

   if (blockIndex >= 0)
   {
      startRange = blockIndex * m_buffer.size();
      endRange = startRange + m_buffer.size() - 1;

      result = true;
   }

   return result;
}

bool ossim::S3StreamBuffer::loadBlock(ossim_int64 absolutePosition)
{
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
          << "ossim::S3StreamBuffer::loadBlock DEBUG: entered with absolute position: " << absolutePosition << "\n";
   }
   bool result = false;
   m_bufferPtr = 0;
   GetObjectRequest getObjectRequest;
   std::stringstream stringStream;
   ossim_int64 startRange, endRange;
   ossim_int64 blockIndex = getBlockIndex(absolutePosition);
   if ((absolutePosition < 0) || (absolutePosition > (ossim_int64)m_fileSize))
      return false;
   //std::cout << "CURRENT BYTE LOCATION = " << absoluteLocation << std::endl;
   if (getBlockRangeInBytes(blockIndex, startRange, endRange))
   {
      stringStream << "bytes=" << startRange << "-" << endRange;
      getObjectRequest.WithBucket(m_bucket.c_str())
          .WithKey(m_key.c_str())
          .WithRange(stringStream.str().c_str());
      auto getObjectOutcome = m_client->GetObject(getObjectRequest);

      if (getObjectOutcome.IsSuccess())
      {
         //        std::cout << "GOOD CALL!!!!!!!!!!!!\n";
         Aws::IOStream &bodyStream = getObjectOutcome.GetResult().GetBody();
         ossim_int64 bufSize = getObjectOutcome.GetResult().GetContentLength();
         //        std::cout << "SIZE OF RESULT ======== " << bufSize << std::endl;
         m_bufferActualDataSize = bufSize;
         bodyStream.read(&m_buffer.front(), bufSize);
         m_bufferPtr = &m_buffer.front();

         ossim_int64 delta = absolutePosition - startRange;
         setg(m_bufferPtr, m_bufferPtr + delta, m_bufferPtr + m_bufferActualDataSize);
         //         m_blockInfo.setBytes(startRange, startRange + delta, startRange + m_bufferActualDataSize);
         m_blockInfo.setBytes(startRange, startRange + m_bufferActualDataSize);
         // std::cout << "LOADING BLOCK: " << m_blockInfo.getStartByte() << ", " << m_blockInfo.getCurrentByte() << ", " << m_blockInfo.getEndByte() << "\n";
         m_currentBlockPosition = startRange;
         result = true;
         //std::cout << "Successfully retrieved object from s3 with value: " << std::endl;
         //std::cout << getObjectOutcome.GetResult().GetBody().rdbuf() << std::endl << std::endl;;
      }
      else
      {
         m_bufferActualDataSize = 0;
      }
   }
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
          << "ossim::S3StreamBuffer::loadBlock DEBUG: leaving with absolutePosition " << absolutePosition << "\n";
   }

   return result;
}

ossim::S3StreamBuffer *ossim::S3StreamBuffer::open(const char *connectionString,
                                                   const ossimKeywordlist &options,
                                                   std::ios_base::openmode m)
{
   std::string temp(connectionString);
   return open(temp, options, m);
}

ossim::S3StreamBuffer *ossim::S3StreamBuffer::open(const std::string &connectionString,
                                                   const ossimKeywordlist & /* options */,
                                                   std::ios_base::openmode /* mode */)
{
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
          << "ossim::S3StreamBuffer::open DEBUG: entered..... with connection " << connectionString << std::endl;
   }
   // bool result = false;
   ossimUrl url(connectionString);
   clearAll();
   // m_mode = mode;
   ossimTimer::Timer_t startTimer = ossimTimer::instance()->tick();

   // AWS server is case insensitive:
   if ((url.getProtocol() == "s3") || (url.getProtocol() == "S3"))
   {
      ossim_int64 filesize;
      m_bucket = url.getIp().c_str();
      m_key = url.getPath().c_str();
      if (ossim::S3HeaderCache::instance()->getCachedFilesize(connectionString, filesize))
      {
         m_fileSize = filesize;
         if (m_fileSize >= 0)
         {
            m_opened = true;
            m_currentBlockPosition = 0;
         }
      }
      else
      {
         if (!m_bucket.empty() && !m_key.empty())
         {
            HeadObjectRequest headObjectRequest;
            headObjectRequest.WithBucket(m_bucket.c_str())
                .WithKey(m_key.c_str());
            auto headObject = m_client->HeadObject(headObjectRequest);
            if (headObject.IsSuccess())
            {
               m_fileSize = headObject.GetResult().GetContentLength();
               m_opened = true;
               m_currentBlockPosition = 0;
               ossim::S3HeaderCache::Node_t nodePtr = std::make_shared<ossim::S3HeaderCacheNode>(m_fileSize);
               ossim::S3HeaderCache::instance()->addHeader(connectionString, nodePtr);
            }
            else
            {
               m_opened = false;
               m_fileSize = -1;
               m_currentBlockPosition = 0;
               ossim::S3HeaderCache::Node_t nodePtr = std::make_shared<ossim::S3HeaderCacheNode>(m_fileSize);
               ossim::S3HeaderCache::instance()->addHeader(connectionString, nodePtr);
            }
         }
      }
   }
   ossimTimer::Timer_t endTimer = ossimTimer::instance()->tick();

   if (traceDebug())
   {
      ossim_float64 delta = ossimTimer::instance()->delta_s(startTimer, endTimer);

      ossimNotify(ossimNotifyLevel_DEBUG)
          << "ossim::S3StreamBuffer::open DEBUG: Took " << delta << " seconds to open" << std::endl;
      ossimNotify(ossimNotifyLevel_DEBUG)
          << "ossim::S3StreamBuffer::open DEBUG: leaving....." << std::endl;
   }
   if (m_opened)
      return this;

   return 0;
}

void ossim::S3StreamBuffer::clearAll()
{
   m_bucket = "";
   m_key = "";
   m_fileSize = 0;
   m_opened = false;
   m_currentBlockPosition = 0;
   m_blockInfo.setBytes(0, 0);
}

int ossim::S3StreamBuffer::underflow()
{
   if (!is_open())
   {
      return EOF;
   }
   else if (!withinWindow())
   {
      ossim_int64 absolutePosition = getAbsoluteByteOffset();
      if (absolutePosition < 0)
      {
         return EOF;
      }

      if (!loadBlock(absolutePosition))
      {
         return EOF;
      }
   }

   // std::cout << "GPTR CHARACTER ========== "
   // << (int)static_cast<ossim_uint8>(*gptr()) << std::endl;

   //---
   // Double cast to get non-negative values so as to not send an inadvertent
   // EOF(-1) to caller.
   //---
   return (int)static_cast<ossim_uint8>(*gptr());
}

ossim::S3StreamBuffer::pos_type ossim::S3StreamBuffer::seekoff(off_type offset,
                                                               std::ios_base::seekdir dir,
                                                               std::ios_base::openmode mode)
{
   //    std::cout <<"ossim::S3StreamBuffer::seekoff type size === " << sizeof(off_type) << std::endl;
   //    std::cout <<"ossim::S3StreamBuffer::seekoff offset ====== " << offset << std::endl;
   //   std::cout << "ossim::S3StreamBuffer::seekoff\n";
   pos_type result = pos_type(off_type(-1));
   // bool withinBlock = true;
   if ((mode & std::ios_base::in) &&
       (mode & std::ios_base::out))
   {
      return result;
   }
   switch (dir)
   {
   case std::ios_base::beg:
   {
      ossim_int64 absolutePosition = getAbsoluteByteOffset();
      if ((offset <= (ossim_int64)m_fileSize) &&
          (offset >= 0))
      {
         result = pos_type(offset);
      }
      if (mode & std::ios_base::in)
      {
         absolutePosition = getAbsoluteByteOffset();
         ossim_int64 delta = offset - absolutePosition;
         setg(eback(), gptr() + delta, egptr());
      }
      break;
   }
   case std::ios_base::cur:
   {
      if (!offset)
      {
         result = getAbsoluteByteOffset();
      }
      else
      {
         result += offset;
         setg(eback(), gptr() + offset, egptr());
      }

      break;
   }
   case std::ios_base::end:
   {
      ossim_int64 absolutePosition = m_fileSize + offset;
      ossim_int64 currentAbsolutePosition = m_blockInfo.getStartByte() + (gptr() - eback());
      ossim_int64 delta = absolutePosition - currentAbsolutePosition;
      if (mode & std::ios_base::in)
      {
         setg(eback(), gptr() + delta, egptr());
         result = absolutePosition;
      }
      break;
   }
   default:
   {
      break;
   }
   }

   return result;
}

ossim::S3StreamBuffer::pos_type ossim::S3StreamBuffer::seekpos(pos_type pos, std::ios_base::openmode mode)
{
   pos_type result = pos_type(off_type(-1));
   ossim_int64 tempPos = static_cast<ossim_int64>(pos);
   ossim_int64 absoluteLocation = getAbsoluteByteOffset();
   if (mode & std::ios_base::in)
   {
      if ((pos >= 0) && (pos < (ossim_int64)m_fileSize))
      {
         ossim_int64 delta = tempPos - absoluteLocation;
         if (delta)
         {
            setg(eback(), gptr() + delta, egptr());
         }
         result = pos;
      }
   }
   return result;
}

std::streamsize ossim::S3StreamBuffer::xsgetn(char_type *s, std::streamsize n)
{
   if (!is_open())
      return EOF;
   // unsigned long int bytesLeftToRead = egptr()-gptr();
   // initialize if we need to to load the block at current position
   if (!withinWindow() && is_open())
   {
      ossim_int64 currentAbsolutePosition = m_blockInfo.getStartByte() + (gptr() - eback());
      if (!loadBlock(currentAbsolutePosition))
      {
         return EOF;
      }
   }
   ossim_int64 bytesNeedToRead = n;
   ossim_int64 bytesRead = 0;
   ossim_int64 currentAbsolutePosition = m_blockInfo.getStartByte() + (gptr() - eback());
   // ossim_int64 startOffset, endOffset;
   if (currentAbsolutePosition >= (ossim_int64)m_fileSize)
   {
      return EOF;
   }
   else if ((currentAbsolutePosition + bytesNeedToRead) > (ossim_int64)m_fileSize)
   {
      bytesNeedToRead = (m_fileSize - currentAbsolutePosition);
   }

   while (bytesNeedToRead > 0)
   {
      currentAbsolutePosition = m_blockInfo.getStartByte() + (gptr() - eback());
      //if(!withinWindow()
      if (!withinWindow())
      {
         if (!loadBlock(currentAbsolutePosition))
         {
            return bytesRead;
         }
         currentAbsolutePosition = m_blockInfo.getStartByte() + (gptr() - eback());
      }

      // get each bloc
      if (currentAbsolutePosition >= 0)
      {
         // number of bytes remaining in the current block
         ossim_int64 delta = egptr() - gptr();

         if (delta <= bytesNeedToRead)
         {

            std::memcpy(s + bytesRead, gptr(), delta);
            bytesRead += delta;
            bytesNeedToRead -= delta;
            setg(eback(), gptr() + delta, egptr());
         }
         else
         {
            std::memcpy(s + bytesRead, gptr(), bytesNeedToRead);
            setg(eback(), gptr() + bytesNeedToRead, egptr());
            bytesRead += bytesNeedToRead;
            bytesNeedToRead = 0;
         }
      }
      else
      {
         break;
      }
   }
   return std::streamsize(bytesRead);
}

ossim_int64 ossim::S3StreamBuffer::getAbsoluteByteOffset() const
{
   ossim_int64 result = -1;

   if (m_currentBlockPosition >= 0)
   {
      result = m_blockInfo.getStartByte() + (gptr() - eback());

   }

   return result;
}

bool ossim::S3StreamBuffer::withinWindow() const
{
   if (!gptr())
      return false;
   return ((gptr() >= eback()) && (gptr() < egptr()));
}

ossim_uint64 ossim::S3StreamBuffer::getFileSize() const
{
   return static_cast<ossim_uint32>(m_fileSize);
}

ossim_uint64 ossim::S3StreamBuffer::getBlockSize() const
{
   return m_buffer.size();
}
