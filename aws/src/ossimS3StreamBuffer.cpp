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
  m_buffer(4096),
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

bool ossim::S3StreamBuffer::loadBlock(ossim_int64 blockIndex)
{
    bool result = false;
    m_bufferPtr = 0;
    m_currentPtr = 0;
    GetObjectRequest getObjectRequest;
    std::stringstream stringStream;
    ossim_int64 startRange, endRange;
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
  if((!gptr())&&is_open())
  {
    if(m_currentPosition >= 0)
    {
      loadBlock(getBlockIndex(m_currentPosition));
    }
    else
    {
      loadBlock(0);
    }

    if(!gptr())
    {
      return EOF;
    }
  }
  else if(!is_open())
  {
    return EOF;
  }
  else if(egptr() == gptr())
  {
    m_currentPosition += m_buffer.size();
    if(m_currentPosition < m_fileSize)
    {
      if(!loadBlock(getBlockIndex(m_currentPosition)))
      {
        return EOF;
      }
    }
    else
    {
     // std::cout << "SHOULD BE EOF RETURNED\n";
      return EOF;
    }
  }

//  std::cout << "GPTR CHARACTER ========== " << (int)(*gptr()) << std::endl;
  return (int)(*gptr());
}
#if 0
int ossim::S3StreamBuffer::uflow()
{
 // std::cout << "ossim::S3StreamBuffer::uflow()\n";

  int result = underflow();

  if(result != EOF)
  {
    result = (int) (*gptr());
    ++m_currentPosition;
    pbump(1);

  }

  return result;
}
#endif
ossim::S3StreamBuffer::pos_type ossim::S3StreamBuffer::seekoff(off_type offset, 
                                                               std::ios_base::seekdir dir,
                                                               std::ios_base::openmode mode)
{ 
//  std::cout << "ossim::S3StreamBuffer::seekoff\n";
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
      // if we are determing an absolute position from the beginning then 
      // just make sure the offset is within range of the current buffer size
      //
      if((offset < m_fileSize)&&
         (offset >=0))
      {
         result = pos_type(offset);
      }
      break;
    }
    case std::ios_base::cur:
    {
      ossim_int64 testOffset = m_currentPosition + offset;
      if((testOffset >= 0)||(testOffset<m_fileSize))
      {
        result = testOffset;
      }

      break;
    }
    case std::ios_base::end:
    {
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
  if(m_currentPosition != result)
  {
    adjustForSeekgPosition(result);
  }

  return result; 
} 

ossim::S3StreamBuffer::pos_type ossim::S3StreamBuffer::seekpos(pos_type pos, std::ios_base::openmode mode)
{
//  std::cout << "ossim::S3StreamBuffer::seekpos\n";
   pos_type result = pos_type(off_type(-1));
   if(mode & std::ios_base::in)
   {
      if(pos >= 0)
      {
         if(pos < m_fileSize)
         {
            result = pos;
         }
      }
      adjustForSeekgPosition(result);
   }

   return result;
}

void ossim::S3StreamBuffer::adjustForSeekgPosition(ossim_int64 seekPosition)
{
  if(seekPosition >= 0)
  {
    ossim_int64 testPosition = static_cast<ossim_int64> (seekPosition);

    if(m_currentPosition >= 0 )
    {
      ossim_int64 blockIndex1 = getBlockIndex(testPosition);
      ossim_int64 blockIndex2 = getBlockIndex(m_currentPosition);

      if(blockIndex1 != blockIndex2)
      {
        m_bufferPtr = m_currentPtr = 0;

        // clear out the pointers and force a load on next read
        setg(m_bufferPtr, m_bufferPtr, m_bufferPtr);
        m_currentPosition = seekPosition;
      }
      else
      {
        ossim_int64 startOffset, endOffset;
        ossim_int64 delta;
        getBlockRangeInBytes(blockIndex1, startOffset, endOffset);
        delta = testPosition-startOffset;
        m_currentPosition = testPosition;
        setg(m_bufferPtr, m_bufferPtr+delta, m_bufferPtr+m_buffer.size());
      }
    }
    else
    {
      m_bufferPtr = m_currentPtr = 0;
      m_currentPosition = seekPosition;
      setg(m_bufferPtr, m_bufferPtr, m_bufferPtr);
    }
  }
  else
  {
    if(m_currentPosition < 0)
    {
      m_bufferPtr = m_currentPtr = 0;
      setg(m_bufferPtr, m_bufferPtr, m_bufferPtr);
    }
  }
}

std::streamsize ossim::S3StreamBuffer::xsgetn(char_type* s, std::streamsize n)
{      
//  std::cout << "ossim::S3StreamBuffer::xsgetn" << std::endl;

  if(!is_open()) return EOF;
   // unsigned long int bytesLeftToRead = egptr()-gptr();
  // initialize if we need to to load the block at current position
  if((egptr()==gptr())&&is_open())
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
  ossim_int64 bytesNeedToRead = n;
  ossim_int64 bytesToRead = 0;
  ossim_int64 bytesRead = 0;
  ossim_int64 startOffset, endOffset;
  if(m_currentPosition >= m_fileSize)
  {
    return EOF;
  }
  else if((m_currentPosition + bytesNeedToRead)>(m_fileSize))
  {
    bytesNeedToRead = (m_fileSize - m_currentPosition);
  }

  while(bytesNeedToRead > 0)
  {
    if(egptr()==gptr())
    {
      // load next block
      if(m_currentPosition < m_fileSize)
      {
        if(!loadBlock(getBlockIndex(m_currentPosition)))
        {
          return bytesRead;
        }
      }
      else
      {
        return bytesRead;
      }
    }

    // get each bloc  
    if(m_currentPosition>=0)
    {
      //getBlockRangeInBytes(getBlockIndex(m_currentPosition), startOffset, endOffset);      
    
      ossim_int64 delta = (egptr()-gptr());//(endOffset - m_currentPosition)+1;

      if(delta <= bytesNeedToRead)
      {
        std::memcpy(s+bytesRead, gptr(), delta);
        m_currentPosition += delta;
        setg(eback(), egptr(), egptr());
        bytesRead+=delta;
        bytesNeedToRead-=delta;
      }
      else
      {
        std::memcpy(s+bytesRead, gptr(), bytesNeedToRead);
        m_currentPosition += bytesNeedToRead;
        setg(eback(), gptr()+bytesNeedToRead, egptr());
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
