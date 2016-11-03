#ifndef ossimS3StreamBuffer_HEADER
#define ossimS3StreamBuffer_HEADER
#include <iostream>
#include <ossim/base/ossimConstants.h>
#include <aws/s3/S3Client.h>

class  ossimS3StreamBuffer : public std::streambuf
{
public:
   ossimS3StreamBuffer();

   bool open(const std::string& connectionString);    
    
   virtual ~ossimS3StreamBuffer()
   {

   }
    
protected:
  //virtual int_type pbackfail(int_type __c  = traits_type::eof());
  virtual pos_type seekoff(off_type offset, std::ios_base::seekdir dir,
                          std::ios_base::openmode __mode = std::ios_base::in | std::ios_base::out);
  virtual pos_type seekpos(pos_type pos, std::ios_base::openmode __mode = std::ios_base::in | std::ios_base::out);
  virtual std::streamsize xsgetn(char_type* __s, std::streamsize __n);
  virtual int underflow();
  // virtual std::streamsize xsputn(const char_type* __s, std::streamsize __n);

  
  void clearAll();
  
  ossim_int64 getBlockIndex(ossim_uint64 byteOffset)const;
  ossim_int64 getBlockOffset(ossim_uint64 byteOffset)const;
  bool getBlockRangeInBytes(ossim_int64 blockIndex, 
    ossim_uint64& startRange, 
    ossim_uint64& endRange)const;

  bool loadBlock(ossim_int64 blockIndex);

  Aws::S3::S3Client m_client;
  std::string m_bucket;
  std::string m_key;
  std::vector<char> m_buffer;
  ossim_uint64 m_bufferActualDataSize;
  char* m_bufStart;
  char* m_bufEnd;
  ossim_uint64 m_fileSize;
};
#endif

