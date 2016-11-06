#ifndef ossimS3StreamBuffer_HEADER
#define ossimS3StreamBuffer_HEADER
#include <iostream>
#include <ossim/base/ossimConstants.h>
#include <aws/s3/S3Client.h>

namespace ossim{
class  S3StreamBuffer : public std::streambuf
{
public:
  S3StreamBuffer();

  S3StreamBuffer* open (const char* connectionString,  std::ios_base::openmode mode);
  S3StreamBuffer* open (const std::string& connectionString, std::ios_base::openmode mode);

  bool is_open() const
  {
    return m_opened;
  }
  virtual ~S3StreamBuffer()
  {

  }
    
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

  bool loadBlock(ossim_int64 blockIndex);

  void adjustForSeekgPosition(ossim_int64 seekPosition);

  Aws::S3::S3Client m_client;
  std::string m_bucket;
  std::string m_key;
  std::vector<char> m_buffer;
  ossim_uint64 m_bufferActualDataSize;
  ossim_int64 m_currentPosition;
  char* m_bufferPtr;
  char* m_currentPtr;
  ossim_uint64 m_fileSize;
  bool m_opened;
  std::ios_base::openmode m_mode;
};

}

#endif

