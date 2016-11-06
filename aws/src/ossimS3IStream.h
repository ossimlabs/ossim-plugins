#ifndef ossimS3IStream_HEADER
#define ossimS3IStream_HEADER
#include <ossim/base/ossimIoStream.h>
#include "ossimS3StreamBuffer.h"

namespace ossim{

   class S3IStream : public ossim::istream
   {
   public:
      S3IStream():std::istream(&m_s3membuf)
      {}

      void open (const char* connectionString,  
                 std::ios_base::openmode mode)
      {
         open(std::string(connectionString), mode);
      }
      void open (const std::string& connectionString, 
                 std::ios_base::openmode mode)
      {
        if(m_s3membuf.open(connectionString, mode))
        {
            clear();
        }
        else
        {
            setstate(std::ios::failbit);
        }
      }
   protected:
     S3StreamBuffer m_s3membuf;

   };
}

#endif