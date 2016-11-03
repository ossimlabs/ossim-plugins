#include "ossimS3StreamBuffer.h"

#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/GetObjectResult.h>
#include <aws/s3/model/HeadObjectRequest.h>

#include <aws/core/Aws.h>
#include <aws/core/utils/memory/stl/AwsStringStream.h> 
#include <iostream>
#include <ossim/base/ossimUrl.h>
//static const char* KEY = "test-file.txt";
//static const char* BUCKET = "ossimlabs";
#include <cstdio> /* for EOF */
#include <streambuf>
#include <sstream>
#include <iosfwd>
#include <ios>
#include <vector>
#include <cstring> /* for memcpy */

using namespace Aws::S3;
using namespace Aws::S3::Model;

ossimS3StreamBuffer::ossimS3StreamBuffer()
:
  m_bucket(""),
  m_key(""),
  m_buffer(2048),
  m_bufferActualDataSize(0),
  m_bufStart(0),
  m_bufEnd(0),
  m_fileSize(0)
{
    std::cout << "CONSTRUCTED!!!!!" << std::endl;
//  setp(0);
  setg(m_bufStart, m_bufStart, m_bufEnd);
}

ossim_int64 ossimS3StreamBuffer::getBlockIndex(ossim_uint64 byteOffset)const
{
  ossim_int64 blockNumber = -1;
  
  if(byteOffset < m_fileSize)
  {
    if(m_buffer.size()>0)
    {
      blockNumber = byteOffset/m_buffer.size();

      if(byteOffset%m_buffer.size()) blockNumber++;
    }    
  }

  return blockNumber;
}

ossim_int64 ossimS3StreamBuffer::getBlockOffset(ossim_uint64 byteOffset)const
{
  ossim_int64 blockOffset = -1;
  
  if(m_buffer.size()>0)
  {
    blockOffset = byteOffset%m_buffer.size();
  }    

  return blockOffset;
}

bool ossimS3StreamBuffer::getBlockRangeInBytes(ossim_int64 blockIndex, 
  ossim_uint64& startRange, 
  ossim_uint64& endRange)const
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

bool ossimS3StreamBuffer::loadBlock(ossim_int64 byteOffset)
{
    bool result = false;
    GetObjectRequest getObjectRequest;
    std::stringstream stringStream;
    ossim_uint64 startRange, endRange;
    ossim_int64 blockIndex = getBlockIndex(byteOffset);
    if(getBlockRangeInBytes(blockIndex, startRange, endRange))
    {
      stringStream << "bytes=" << startRange << "-" << endRange;
      getObjectRequest.WithBucket(m_bucket.c_str())
          .WithKey(m_key.c_str()).WithRange(stringStream.str().c_str());
      auto getObjectOutcome = m_client.GetObject(getObjectRequest);

      if(getObjectOutcome.IsSuccess())
      {
        std::cout << "GOOD CALL!!!!!!!!!!!!\n";
        Aws::IOStream& bodyStream = getObjectOutcome.GetResult().GetBody();
        ossim_uint64 bufSize = getObjectOutcome.GetResult().GetContentLength();
        std::cout << "SIZE OF RESULT ======== " << bufSize << std::endl;
        m_bufferActualDataSize = bufSize;
        bodyStream.read(&m_buffer.front(), bufSize);
          //std::cout << "Successfully retrieved object from s3 with value: " << std::endl;
          //std::cout << getObjectOutcome.GetResult().GetBody().rdbuf() << std::endl << std::endl;;  
      }
    }

    return result;
}

bool ossimS3StreamBuffer::open(const std::string& connectionString)   
{
  bool result = false;
  ossimUrl url(connectionString);
  clearAll();

  std::cout << connectionString << "\n";
  if(url.getProtocol() == "s3")
  {
    m_bucket = url.getIp().c_str();
    m_key = url.getPath().c_str();

    if(!m_bucket.empty() && !m_key.empty())
    {
      HeadObjectRequest headObjectRequest;
      headObjectRequest.WithBucket(m_bucket.c_str())
          .WithKey(m_key.c_str());
      auto headObject = m_client.HeadObject(headObjectRequest);
      if(headObject.IsSuccess())
      {
        m_fileSize = headObject.GetResult().GetContentLength();
        std::cout << "CONTENT LENGTH ==== " << m_fileSize << "\n";
        result = true;

        loadBlock(0);
      }
    }
  }

  return result;
}


void ossimS3StreamBuffer::clearAll()
{
  m_bucket = "";
  m_key    = "";
  m_bufStart = 0;
  m_bufEnd   = 0,
  m_fileSize = 0;

}

// added so we can set a buffer and make it shared
// std::streambuf* ossimByteStreamBuffer::setBuf(char* buf, std::streamsize bufSize, bool shared)
// {
   
   // setp(0,0);
   // setg(0,0,0);
   // char_type* tempBuf = buf;
   // if(!shared&&bufSize&&buf)
   // {
   //    tempBuf = new char_type[bufSize];
   //    memcpy(tempBuf, buf, bufSize);
   // }
   // m_buffer = tempBuf;
   // m_sharedBuffer = shared;
   // m_bufferSize = bufSize;
   // setp(m_buffer, m_buffer+bufSize);
   // if(m_buffer)
   // {
   //    setg(m_buffer, m_buffer, m_buffer+bufSize);
   // }
    
//    return this;
// }

 int ossimS3StreamBuffer::underflow()
 {
    std::cout << "ossimS3StreamBuffer::underflow.........................\n";
    return EOF;
   // if(m_sharedBuffer)
   // {
   //    return EOF;
   // }
   // else
   // {            
   //    unsigned long int oldSize = m_bufferSize;
   //    extendBuffer(1);
   //    pbump(1);
   //    m_buffer[oldSize] = (char_type)c;
   // }
    
   //return c;
}



// ossimByteStreamBuffer::int_type ossimByteStreamBuffer::pbackfail(int_type __c )
// {
//    int_type result = __c;
//    long int delta = gptr()-eback();
//    if(delta!=0)
//    {
//       setg(m_buffer, m_buffer+(delta-1), m_buffer+m_bufferSize);
//       if(__c != traits_type::eof())
//       {
//          *gptr() = static_cast<char_type>(__c);
//       }
//    }
//    else
//    {
//       result = traits_type::eof();
//    }
//    return result;
// }
ossimS3StreamBuffer::pos_type ossimS3StreamBuffer::seekoff(off_type offset, std::ios_base::seekdir dir,
                                                               std::ios_base::openmode __mode)
{ 
  std::cout << "ossimS3StreamBuffer::seekoff.........................\n";
    pos_type result = pos_type(off_type(-1));
   if((__mode & std::ios_base::in)&&
      (__mode & std::ios_base::out))
   {
    std::cout << "WHY ARE WE BOTH IN AND OUT\n";
      // we currently will not support both input and output stream at the same time
      //
      return result;
   }
   switch(dir)
   {
      case std::ios_base::beg:
      {
        std::cout << "SEEKING FROM BEG\n";
         // if we are determing an absolute position from the beginning then 
         // just make sure the offset is within range of the current buffer size
         //
          if((offset < m_fileSize)&&
             (offset >=0))
          {
             result = pos_type(offset);
          }
          if(__mode  & std::ios_base::in)
          {
         //    gbump(offset - (gptr() - eback()));
          }
         break;
      }
      case std::ios_base::cur:
      {
         std::cout << "SEEKING FROM CURRENT\n";
         // if we are determing an absolute position from the current pointer then 
         // add the offset as a relative displacment
         //
         // pos_type newPosition = result;
         // long int delta = 0;
         // if(__mode & std::ios_base::in)
         // {
         //    delta = gptr()-eback();
         // }
         // else if(__mode & std::ios_base::out)
         // {
         //    delta = pptr()-pbase();
         // }
         // newPosition = pos_type(delta + offset);
         // if((newPosition >= 0)&&(newPosition < m_bufferSize))
         // {
         //    result = newPosition;
         //    if(__mode  & std::ios_base::in)
         //    {
         //       gbump(offset);
         //    }
         //    else if(__mode & std::ios_base::out)
         //    {
         //       pbump(offset);
         //    }
         // }
         break;
      }
      case std::ios_base::end:
      {
         std::cout << "SEEKING FROM END\n";
         // pos_type newPosition = result;
         // long int delta = 0;
         // if(__mode & std::ios_base::in)
         // {
         //    delta = egptr()-eback();
         // }
         // else if(__mode & std::ios_base::out)
         // {
         //    delta = epptr()-pbase();
         // }
         // newPosition = pos_type(delta + offset);
         // if((newPosition >= 0)&&(newPosition < m_bufferSize))
         // {
         //    result = newPosition;
         //    if(__mode  & std::ios_base::in)
         //    {
         //       gbump(offset - (gptr() - eback()));
         //    }
         //    else if(__mode & std::ios_base::out)
         //    {
         //       pbump(offset - (epptr() - pptr()));
         //    }
         // }
         break;
      }
      default:
      {
         break;
      }
   }

   return result; 
} 

ossimS3StreamBuffer::pos_type ossimS3StreamBuffer::seekpos(pos_type pos, std::ios_base::openmode __mode)
{
  std::cout << "ossimS3StreamBuffer::seekpos.........................\n";

   pos_type result = pos_type(off_type(-1));
    
   if(__mode & std::ios_base::in)
   {
      if(pos >= 0)
      {
         if(pos < m_fileSize)
         {
            //setg(m_buffer, m_buffer + pos, m_buffer + m_bufferSize);
            result = pos;
         }
      }
   }
 /*  else if(__mode & std::ios_base::out)
   {
      if(pos >=0)
      {
         setp(m_buffer, m_buffer+m_bufferSize);
         if(pos < m_bufferSize)
         {
            pbump(pos);
            result = pos;
         }
         else if(!m_sharedBuffer)
         {
            long int delta = (long int)(pos) - m_bufferSize;
            if(delta > 0)
            {
               extendBuffer(delta+1);
               pbump(pos);
               result = pos;
            }
         }
      }
   }
    */
   return result;
}

std::streamsize ossimS3StreamBuffer::xsgetn(char_type* __s, std::streamsize __n)
{
  std::cout << "ossimS3StreamBuffer::xsgetn.........................\n";
   // unsigned long int bytesLeftToRead = egptr()-gptr();
    unsigned long int bytesToRead = __n;
    
   // if(bytesToRead > bytesLeftToRead)
   // {
   //    bytesToRead = bytesLeftToRead;
   // }
   // if(bytesToRead)
   // {
  //    std::memcpy(__s, gptr(), bytesToRead);
      gbump(bytesToRead); // bump the current get pointer
   // }
   return std::streamsize(bytesToRead);
}
/*
std::streamsize ossimS3StreamBuffer::xsputn(const char_type* __s, std::streamsize __n)
{
   long int bytesLeftToWrite = epptr()-pptr();
   long int bytesToWrite = __n;
   if(__n > bytesLeftToWrite)
   {
      if(!m_sharedBuffer)
      {
         extendBuffer(__n-bytesLeftToWrite);
      }
      else
      {
         bytesToWrite = bytesLeftToWrite;
      }
   }
   if(bytesToWrite)
   {
      std::memcpy(pptr(), __s, bytesToWrite);
      pbump(bytesToWrite);
   }
   return bytesToWrite;
}
*/    
/*
void ossimS3StreamBuffer::deleteBuffer()
{
   if(!m_sharedBuffer&&m_buffer)
   {
      delete [] m_buffer;
   }
   m_buffer = 0;
   m_bufferSize=0;
}
*/
 /*
void ossimS3StreamBuffer::extendBuffer(unsigned long int bytes)
{
   // ossim_uint32 oldSize = m_bufferSize;
   char* inBegin = eback();
   char* inCur   = gptr();
   unsigned long int pbumpOffset = pptr()-pbase();
    
   long int relativeInCur = inCur-inBegin;
    
    
   if(!m_buffer)
   {
      if(bytes>0)
      {
         m_buffer = new char[m_bufferSize + bytes];
      }
   }
   else 
   {
      if(bytes>0)
      {
         char* newBuf = new char[m_bufferSize + bytes];
         std::memcpy(newBuf, m_buffer, m_bufferSize);
         delete [] m_buffer;
         m_buffer = newBuf;
      }
   }
   m_bufferSize += bytes;
    
   setp(m_buffer, m_buffer+m_bufferSize);
   setg(m_buffer, m_buffer+relativeInCur, m_buffer + m_bufferSize);
   pbump(pbumpOffset); // reallign to the current location
}
*/
