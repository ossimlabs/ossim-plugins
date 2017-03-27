//---
//
// License: MIT
//
// Author: Garrett Potts
// 
// Description:
// 
//
//---
// $Id$

#include "ossimCurlStreamBuffer.h"
#include "CurlHeaderCache.h"

#include <ossim/base/ossimUrl.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimTimer.h>
#include <ctime>

#include <cstdio> /* for EOF */
#include <cstring> /* for memcpy */
#include <ios>
#include <iostream>
#include <streambuf>
#include <sstream>
#include <vector>

static ossimTrace traceDebug("ossimCurlStreamBuffer:debug");

ossim::CurlStreamBuffer::CurlStreamBuffer(ossim_int64 blockSize)
   :
   m_bucket(""),
   m_key(""),
   m_buffer(blockSize),
   m_bufferActualDataSize(0),
   m_currentBlockPosition(-1),
   m_bufferPtr(0),
   m_fileSize(0),
   m_opened(false)
   //m_mode(0)
{
   setg(m_bufferPtr, m_bufferPtr, m_bufferPtr);
}

ossim_int64 ossim::CurlStreamBuffer::getBlockIndex(ossim_int64 byteOffset)const
{
   ossim_int64 blockNumber = -1;
  
   if(byteOffset < (ossim_int64)m_fileSize)
   {
      if(m_buffer.size()>0)
      {
         blockNumber = byteOffset/m_buffer.size();
      }    
   }

   return blockNumber;
}

ossim_int64 ossim::CurlStreamBuffer::getBlockOffset(ossim_int64 byteOffset)const
{
   ossim_int64 blockOffset = -1;
  
   if(m_buffer.size()>0)
   {
      blockOffset = byteOffset%m_buffer.size();
   }    

   return blockOffset;
}

bool ossim::CurlStreamBuffer::getBlockRangeInBytes(ossim_int64 blockIndex, 
                                                 ossim_int64& startRange, 
                                                 ossim_int64& endRange)const
{
   bool result = false;

   if(blockIndex >= 0)
   {
      startRange = blockIndex*m_buffer.size();
      endRange = startRange + m_buffer.size()-1;

      result = true;    
   }

   return result;
}

bool ossim::CurlStreamBuffer::loadBlock(ossim_int64 absolutePosition)
{
   bool result = false;
   m_bufferPtr = 0;
   //GetObjectRequest getObjectRequest;
   std::stringstream stringStream;
   ossim_int64 startRange, endRange;
   ossim_int64 blockIndex = getBlockIndex(absolutePosition);
   // std::cout << "blockIndex = " << blockIndex << "\n";
   if(absolutePosition<0) return false;
   if((absolutePosition < 0) || (absolutePosition > (ossim_int64)m_fileSize))
   {
      return false;
   }
   if(getBlockRangeInBytes(blockIndex, startRange, endRange))
   {
      m_curlHttpRequest.clearHeaderOptions();
      stringStream << "bytes=" << startRange << "-" << endRange;
      m_curlHttpRequest.addHeaderOption("Range", stringStream.str().c_str());
      // std::cout << "HEADER ====== " << stringStream.str().c_str() << "\n";
      ossimRefPtr<ossimWebResponse> webResponse = m_curlHttpRequest.getResponse();
      ossimHttpResponse* response = dynamic_cast<ossimHttpResponse*>(webResponse.get());
      //std::cout << "GOT RESPONSE!!!\n";

      // may not get 200 for this is a partial content call
      //
      ossim_int32 code = response->getStatusCode();
      if(response&&( (code >=200) && (code < 300)) )
      {
         m_buffer.clear();
         response->copyAllDataFromInputStream(m_buffer);
         m_bufferPtr = &m_buffer.front();
         response->convertHeaderStreamToKeywordlist();
         const ossimKeywordlist& headerKwl = response->headerKwl();
         ossim_int64 contentLen = response->getContentLength();
         if(contentLen >=0)
         {
            m_bufferActualDataSize = contentLen;
            ossim_int64 delta = absolutePosition-startRange;
            m_bufferPtr = &m_buffer.front();
            setg(m_bufferPtr, m_bufferPtr + delta, m_bufferPtr+m_bufferActualDataSize);
            m_currentBlockPosition = startRange;
            result = true;
         }
         else
         {
            m_buffer.clear();
            m_bufferActualDataSize = 0;
            // std::cout << "FAILED LOADED BLOCK\n";

         }
      } 
      else if(response)
      {
         // std::cout << "Status code was " << response->getStatusCode() << "\n";
      }
      // getObjectRequest.WithBucket(m_bucket.c_str())
      //    .WithKey(m_key.c_str()).WithRange(stringStream.str().c_str());
      // auto getObjectOutcome = m_client.GetObject(getObjectRequest);

      // if(getObjectOutcome.IsSuccess())
      // {
      //    Aws::IOStream& bodyStream = getObjectOutcome.GetResult().GetBody();
      //    ossim_int64 bufSize = getObjectOutcome.GetResult().GetContentLength();
      //    m_bufferActualDataSize = bufSize;
      //    bodyStream.read(&m_buffer.front(), bufSize);
      //    m_bufferPtr = &m_buffer.front();

      //    ossim_int64 delta = absolutePosition-startRange;
      //    setg(m_bufferPtr, m_bufferPtr + delta, m_bufferPtr+m_bufferActualDataSize);
      //    m_currentBlockPosition = startRange;
      //    result = true;
      // }
      // else
      // {
      //    m_bufferActualDataSize = 0;
      // }
   }

   return result;
}

ossim::CurlStreamBuffer* ossim::CurlStreamBuffer::open (const char* connectionString,  
                                                    std::ios_base::openmode m)
{
   std::string temp(connectionString);
   return open(temp, m);
}

ossim::CurlStreamBuffer* ossim::CurlStreamBuffer::open (const std::string& connectionString, 
                                                    std::ios_base::openmode /* mode */)
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossim::CurlStreamBuffer::open DEBUG: entered..... with connection " << connectionString << std::endl;
   }
   // bool result = false;
   ossimUrl url(connectionString);
   clearAll();
   // m_mode = mode;
   ossimTimer::Timer_t startTimer = ossimTimer::instance()->tick();

   // AWS server is case insensitive:
   if( (url.getProtocol() == "http") || (url.getProtocol() == "https") )
   {
      ossim_int64 filesize;
      if(ossim::CurlHeaderCache::instance()->getCachedFilesize(connectionString, filesize))
      {
         m_fileSize = filesize;
         m_opened = true;
         m_currentBlockPosition = 0;
      }
      else
      {
         ossimKeywordlist header;
         m_opened = true;
         m_curlHttpRequest.set(url, header);
         m_fileSize = m_curlHttpRequest.getContentLength();
         m_currentBlockPosition = 0;

         if(m_fileSize > 0)
         {
            m_opened = true;
         }
         ossim::CurlHeaderCache::Node_t nodePtr = std::make_shared<ossim::CurlHeaderCacheNode>(m_fileSize);
         ossim::CurlHeaderCache::instance()->addHeader(connectionString, nodePtr);
      }
   }
   ossimTimer::Timer_t endTimer = ossimTimer::instance()->tick();

   if(traceDebug())
   {
      ossim_float64 delta = ossimTimer::instance()->delta_s(startTimer, endTimer);

      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossim::CurlStreamBuffer::open DEBUG: Took " << delta << " seconds to open" << std::endl;
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossim::CurlStreamBuffer::open DEBUG: leaving....." << std::endl;

   }
   if(m_opened) return this;

   return 0;
}



void ossim::CurlStreamBuffer::clearAll()
{
   m_bucket = "";
   m_key    = "";
   m_fileSize = 0;
   m_opened = false;
   m_currentBlockPosition = 0;
}


int ossim::CurlStreamBuffer::underflow()
{
   if(!is_open())
   {
      return EOF;
   }
   else if( !withinWindow() )
   {
      ossim_int64 absolutePosition = getAbsoluteByteOffset();
      if(absolutePosition < 0)
      {
         return EOF;
      }

      if(!loadBlock(absolutePosition))
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

ossim::CurlStreamBuffer::pos_type ossim::CurlStreamBuffer::seekoff(off_type offset, 
                                                               std::ios_base::seekdir dir,
                                                               std::ios_base::openmode mode)
{ 
   // std::cout <<"ossim::CurlStreamBuffer::seekoff type size === " << sizeof(off_type) << std::endl;
   // std::cout <<"ossim::CurlStreamBuffer::seekoff offset ====== " << offset << std::endl;
   // std::cout << "ossim::CurlStreamBuffer::seekoff\n";
   pos_type result = pos_type(off_type(-1));
   // bool withinBlock = true;
   if((mode & std::ios_base::in)&&
      (mode & std::ios_base::out))
   {
      return result;
   }
   switch(dir)
   {
      case std::ios_base::beg:
      {
         ossim_int64 absolutePosition = getAbsoluteByteOffset();
         // really would like to figure out a better way but for now
         // we have to have one valid block read in to properly adjust
         // gptr() with gbump
         //
         if(!gptr())
         {
            if(!loadBlock(offset))
            {
               return result;
            }
         }
         if((offset <= (ossim_int64)m_fileSize)&&
            (offset >=0))
         {
            result = pos_type(offset);
         }
         if(!gptr())
         {
            if(!loadBlock(result))
            {
               return EOF;
            }
         }
         else if(mode  & std::ios_base::in)
         {
            absolutePosition = getAbsoluteByteOffset();
            ossim_int64 delta = offset - absolutePosition;
            setg(eback(), gptr()+delta, egptr());
            //gbump(offset - absolutePosition);
         }
         // std::cout << "ossim::CurlStreamBuffer::seekoff beg RESULT??????????????????? " << result << std::endl;
         break;
      }
      case std::ios_base::cur:
      {
         if(!gptr())
         {
            // std::cout << "LOADING BLOCK!!!!!!!!!!!!!!\n" << std::endl;
            if(!loadBlock(0))
            {
               return result;
            }
         }
         result = getAbsoluteByteOffset();
         // std::cout << "INITIAL ABSOLUTE BYTE OFFSET ==== " << result << "\n";
         if(!offset)
         {
            result = getAbsoluteByteOffset();
         }
         else
         {
            result += offset;
//            gbump(offset);
            setg(eback(), gptr()+offset, egptr());

         }
         // std::cout << "ossim::CurlStreamBuffer::seekoff cur RESULT??????????????????? " << result << std::endl;

        break;
      }
      case std::ios_base::end:
      {
         ossim_int64 absolutePosition  = m_fileSize + offset;
         if(!gptr())
         {
             if(absolutePosition==m_fileSize)
             {
                if(!loadBlock(absolutePosition))
                {
                   return result;
                }
             }
             else
             {
               if(!loadBlock(absolutePosition))
               {
                  return result;
               }               
            }
         }
         ossim_int64 currentAbsolutePosition = getAbsoluteByteOffset();
         ossim_int64 delta = absolutePosition-currentAbsolutePosition;

//         std::cout << "CURRENT ABSOLUTE POSITION === " << currentAbsolutePosition << std::endl;
//         std::cout << "CURRENT ABSOLUTE delta POSITION === " << delta << std::endl;
         if(mode & std::ios_base::in )
         {
            setg(eback(), gptr()+delta, egptr());

//            gbump(delta);
            result = absolutePosition;
         }
         // std::cout << "ossim::CurlStreamBuffer::seekoff end RESULT??????????????????? " << result << std::endl;
         break;
      }
      default:
      {
         break;
      }
   }

   return result; 
} 

ossim::CurlStreamBuffer::pos_type ossim::CurlStreamBuffer::seekpos(pos_type pos, std::ios_base::openmode mode)
{
   // std::cout << "ossim::CurlStreamBuffer::seekpos: " << pos << std::endl;
   pos_type result = pos_type(off_type(-1));
   ossim_int64 tempPos = static_cast<ossim_int64>(pos);
   // Currently we must initialize to a block
   if(!gptr())
   {
      if(!loadBlock(tempPos))
      {
         return result;
      }
   }
   ossim_int64 absoluteLocation = getAbsoluteByteOffset();
   if(mode & std::ios_base::in)
   {
      if((pos >= 0)&&(pos < (ossim_int64)m_fileSize))
      {
         ossim_int64 delta = tempPos-absoluteLocation;
         if(delta)
         {
            setg(eback(), gptr()+delta, egptr());
            //gbump(delta);
         }
         result = pos;
      }
   }
   return result;
}

std::streamsize ossim::CurlStreamBuffer::xsgetn(char_type* s, std::streamsize n)
{      
  // std::cout << "ossim::CurlStreamBuffer::xsgetn" << std::endl;

   if(!is_open()) return EOF;
   // unsigned long int bytesLeftToRead = egptr()-gptr();
   // initialize if we need to to load the block at current position
   if((!withinWindow())&&is_open())
   {
      if(!gptr())
      {
         if(!loadBlock(0))
         {
            return EOF;
         }
      }
      else if(!loadBlock(getAbsoluteByteOffset()))
      {
         return EOF;
      }
   }
   ossim_int64 bytesNeedToRead = n;
   // ossim_int64 bytesToRead = 0;
   ossim_int64 bytesRead = 0;
   ossim_int64 currentAbsolutePosition = getAbsoluteByteOffset();
   // ossim_int64 startOffset, endOffset;
   if(currentAbsolutePosition >= (ossim_int64)m_fileSize)
   {
      return EOF;
   }
   else if((currentAbsolutePosition + bytesNeedToRead)>(ossim_int64)m_fileSize)
   {
      bytesNeedToRead = (m_fileSize - currentAbsolutePosition);
   }

   while(bytesNeedToRead > 0)
   {
      currentAbsolutePosition = getAbsoluteByteOffset();
      if(!withinWindow())
      {
         if(!loadBlock(currentAbsolutePosition))
         {
            return bytesRead;
         }
         currentAbsolutePosition = getAbsoluteByteOffset();
      }

      // get each bloc  
      if(currentAbsolutePosition>=0)
      {
         //getBlockRangeInBytes(getBlockIndex(m_currentBlockPosition), startOffset, endOffset);      
    
         ossim_int64 delta = (egptr()-gptr());//(endOffset - m_currentBlockPosition)+1;

         if(delta <= bytesNeedToRead)
         {
            std::memcpy(s+bytesRead, gptr(), delta);
            //m_currentBlockPosition += delta;
            //setg(eback(), egptr(), egptr());
            bytesRead+=delta;
            bytesNeedToRead-=delta;
            //gbump(delta);
            setg(eback(), gptr()+delta, egptr());

         }
         else
         {
            std::memcpy(s+bytesRead, gptr(), bytesNeedToRead);
            setg(eback(), gptr()+bytesNeedToRead, egptr());
            //gbump(bytesNeedToRead);
//            std::cout << "gbump 2 DELTA ========= " << bytesNeedToRead << std::endl;
            bytesRead+=bytesNeedToRead;
            bytesNeedToRead=0;
         }
      }
      else
      {
         break;
      }
   }
   return std::streamsize(bytesRead);
}

ossim_int64 ossim::CurlStreamBuffer::getAbsoluteByteOffset()const
{
   ossim_int64 result = -1;
   
   if(m_currentBlockPosition >= 0)
   {
      result = m_currentBlockPosition;
      if(gptr()&&eback())
      {
         result += (gptr()-eback());
      }
   }

   // std::cout << "RESULT getAbsoluteByteOffset======== " << result << "\n";
   return result;
}

bool ossim::CurlStreamBuffer::withinWindow()const
{
   if(!gptr()) return false;
   return ((gptr()>=eback()) && (gptr()<egptr()));
}

ossim_int64 ossim::CurlStreamBuffer::getFileSize() const
{
   return static_cast<ossim_int64>(m_fileSize);
}

ossim_uint64 ossim::CurlStreamBuffer::getBlockSize() const
{
   return m_buffer.size();
}
