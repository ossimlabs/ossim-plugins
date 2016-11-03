#ifndef ossimS3IStream_HEADER
#define ossimS3IStream_HEADER
#include <ossim/base/ossimIoStream.h>
#include "ossimS3StreamBuffer.h"
class ossimS3IStream : public ossim::istream
{
public:
  ossimS3IStream():std::istream(&m_s3membuf)
  {}

  bool open(const std::string& connectionString)
  {
    return m_s3membuf.open(connectionString);
  }
protected:
  ossimS3StreamBuffer m_s3membuf;

};

#endif
