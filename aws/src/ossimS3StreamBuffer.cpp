#include "ossimS3StreamBuffer.h"

#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/GetObjectResult.h>
#include <aws/s3/model/HeadObjectRequest.h>

#include <aws/core/Aws.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h> 
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

ossim::S3StreamBuffer::S3StreamBuffer()
:
  m_bucket(""),
  m_key(""),
  m_buffer(2048),
  m_bufferActualDataSize(0),
  m_currentPosition(-1),
  m_bufferPtr(0),
  m_currentPtr(0),
  m_fileSize(0),
  m_opened(false),
  m_mode(0)
{
//    std::cout << "CONSTRUCTED!!!!!" << std::endl;
//  setp(0);
  setg(m_currentPtr, m_currentPtr, m_currentPtr);
}

ossim_int64 ossim::S3StreamBuffer::getBlockIndex(ossim_int64 byteOffset)const
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

ossim_int64 ossim::S3StreamBuffer::getBlockOffset(ossim_int64 byteOffset)const
{
  ossim_int64 blockOffset = -1;
  
  if(m_buffer.size()>0)
  {
    blockOffset = byteOffset%m_buffer.size();
  }    

  return blockOffset;
}

bool ossim::S3StreamBuffer::getBlockRangeInBytes(ossim_int64 blockIndex, 
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

bool ossim::S3StreamBuffer::loadBlock(ossim_int64 blockIndex)
{
    bool result = false;
    m_bufferPtr = 0;
    m_currentPtr = 0;
    GetObjectRequest getObjectRequest;
    std::stringstream stringStream;
    ossim_uint64 startRange, endRange;
    if(getBlockRangeInBytes(blockIndex, startRange, endRange))
    {
      stringStream << "bytes=" << startRange << "-" << endRange;
      getObjectRequest.WithBucket(m_bucket.c_str())
          .WithKey(m_key.c_str()).WithRange(stringStream.str().c_str());
      auto getObjectOutcome = m_client.GetObject(getObjectRequest);

      if(getObjectOutcome.IsSuccess())
      {
//        std::cout << "GOOD CALL!!!!!!!!!!!!\n";
        Aws::IOStream& bodyStream = getObjectOutcome.GetResult().GetBody();
        ossim_uint64 bufSize = getObjectOutcome.GetResult().GetContentLength();
//        std::cout << "SIZE OF RESULT ======== " << bufSize << std::endl;
        m_bufferActualDataSize = bufSize;
        bodyStream.read(&m_buffer.front(), bufSize);
        m_bufferPtr = &m_buffer.front();
        m_currentPtr = m_bufferPtr;

        if(m_currentPosition>=0)
        {
          ossim_int64 delta = m_currentPosition-startRange;
          setg(m_bufferPtr, m_bufferPtr + delta, m_bufferPtr+m_bufferActualDataSize);
        }
        else
        {
          setg(m_bufferPtr, m_bufferPtr, m_bufferPtr + m_bufferActualDataSize);
          m_currentPosition = startRange;
        }
        result = true;
          //std::cout << "Successfully retrieved object from s3 with value: " << std::endl;
          //std::cout << getObjectOutcome.GetResult().GetBody().rdbuf() << std::endl << std::endl;;  
      }
    }

    return result;
}

ossim::S3StreamBuffer* ossim::S3StreamBuffer::open (const char* connectionString,  
                                                    std::ios_base::openmode m)
{
  std::string temp(connectionString);
  return open(temp, m);
}

ossim::S3StreamBuffer* ossim::S3StreamBuffer::open (const std::string& connectionString, 
                                                    std::ios_base::openmode mode)
{
  bool result = false;
  ossimUrl url(connectionString);
  clearAll();
  m_mode = mode;

  std::cout << "ossim::S3StreamBuffer::open: " << connectionString << "\n";
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
        std::cout << "ossim::S3StreamBuffer::open: CONTENT LENGTH ==== " << m_fileSize << "\n";
        m_opened = true;
      }
    }
  }

  if(m_opened) return this;

  return 0;
}



void ossim::S3StreamBuffer::clearAll()
{
  m_bucket = "";
  m_key    = "";
  m_currentPtr = 0;
  m_fileSize = 0;
  m_opened = false;

}


int ossim::S3StreamBuffer::underflow()
{
  std::cout << "ossim::S3StreamBuffer::underflow()\n";
   if ( gptr() && ( gptr() < egptr()))
   {
      return * reinterpret_cast<ossim_uint8*>( gptr());
   }
    if(is_open())
    {
      if(!gptr())
      {
        if(m_fileSize)
        {
          loadBlock(0);
        }
      }
    }
    if(gptr())
    {
      return * reinterpret_cast<ossim_uint8 *>( gptr());
    }

    return EOF;
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
ossim::S3StreamBuffer::pos_type ossim::S3StreamBuffer::seekoff(off_type offset, 
                                                               std::ios_base::seekdir dir,
                                                               std::ios_base::openmode mode)
{ 
  std::cout << "ossim::S3StreamBuffer::seekoff: ossimS3StreamBuffer::seekoff.........................\n";
  pos_type result = pos_type(off_type(-1));
  bool withinBlock = true;
  if((mode & std::ios_base::in)&&
    (mode & std::ios_base::out))
  {
    return result;
  }
  switch(dir)
  {
    case std::ios_base::beg:
    {
      std::cout << "ossim::S3StreamBuffer::seekoff: SEEKING FROM BEG\n";
       // if we are determing an absolute position from the beginning then 
       // just make sure the offset is within range of the current buffer size
       //
        if((offset < m_fileSize)&&
           (offset >=0))
        {
           result = pos_type(offset);
        }
       //  if(mode  & std::ios_base::in)
       //  {
       // //    gbump(offset - (gptr() - eback()));
       //  }
       break;
    }
    case std::ios_base::cur:
    {
      std::cout << "ossim::S3StreamBuffer::seekoff: SEEKING FROM CURRENT\n";
      ossim_int64 testOffset = m_currentPosition + offset;
      if((testOffset >= 0)||(testOffset<m_fileSize))
      {
        result = testOffset;
      }

      break;
    }
    case std::ios_base::end:
    {
      std::cout << "ossim::S3StreamBuffer::seekoff: SEEKING FROM END\n";
      ossim_int64 testOffset = m_fileSize - offset;

      if((testOffset >= 0)||(testOffset<m_fileSize))
      {
        result = testOffset;
      }      

      break;
    }
    default:
    {
       break;
    }
  }

  if(result >= 0)
  {
    ossim_int64 testResult = static_cast<ossim_int64> (result);

    if(m_currentPosition >= 0 )
    {
      ossim_int64 blockIndex1 = getBlockIndex(testResult);
      ossim_int64 blockIndex2 = getBlockIndex(m_currentPosition);

      if(testResult != m_currentPosition)
      {
        m_bufferPtr = m_currentPtr = 0;

        // clear out the pointers and force a load on next read
        setg(m_bufferPtr, m_bufferPtr, m_bufferPtr);
      }

    }
  }
  else
  {
    if(m_currentPosition < 0)
    {
      m_bufferPtr = m_currentPtr = 0;
    }
  }
  return result; 
} 

ossim::S3StreamBuffer::pos_type ossim::S3StreamBuffer::seekpos(pos_type pos, std::ios_base::openmode mode)
{
  std::cout << "ossim::S3StreamBuffer::seekpos.........................\n";

   pos_type result = pos_type(off_type(-1));
    
   if(mode & std::ios_base::in)
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
 /*  else if(mode & std::ios_base::out)
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

std::streamsize ossim::S3StreamBuffer::xsgetn(char_type* s, std::streamsize n)
{
  std::cout << "ossim::S3StreamBuffer::xsgetn.........................\n";
   // unsigned long int bytesLeftToRead = egptr()-gptr();
  // initialize if we need to to load the block at current position
  if(!gptr()&&is_open())
  {
    if(m_currentPosition >= 0)
    {
      loadBlock(getBlockIndex(m_currentPosition));
    }
    else
    {
      loadBlock(0);
    }
  }
  bool needMore = true;

  ossim_int64 bytesNeedToRead = n;
  ossim_int64 bytesToRead = 0;
  ossim_int64 bytesRead = 0;

  while(bytesNeedToRead > 0)
  {
    char_type* currentPtr = s;
    ossim_int64 startOffset, endOffset;

    if(m_currentPosition)
    {
      getBlockOffset(getBlockIndex(m_currentPosition, startOffset, endOffset));      
    
      ossim_int64 delta = endOffset - m_currenPosition;

      if(delta > =0 )
      {
        bytesToRead = delta;
      }
    }
    else
    {
      break;
    }
//    if((m_currentPosition + bytesNeedToRead) > endOffset)
//    {
//      bytesToRead = endOffset - m_currentPosition;
//      bytesNeedToRead -= bytesToRead;
//    }
//    else
//    {
//      bytesNeedToRead -= bytesToRead;
//    }
    if(bytesToRead)
    {
      std::memcpy(s, gptr(), bytesToRead);
      setg(m_bufferPtr, 
           m_bufferPtr+bytesToRead,
           m_bufferPtr+m_bufferActualDataSize);
      m_currentPosition += bytesToRead;
      bytesRead+=bytesToRead;
    }
  }



    //m_bufferActualDataSize

   // if(bytesToRead > bytesLeftToRead)
   // {
   //    bytesToRead = bytesLeftToRead;
   // }
   // if(bytesToRead)
   // {
  //    std::memcpy(__s, gptr(), bytesToRead);
   //   gbump(bytesToRead); // bump the current get pointer
   // }
   return std::streamsize(bytesRead);
}
