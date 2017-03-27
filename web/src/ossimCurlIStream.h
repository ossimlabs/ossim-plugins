#ifndef ossimCurlIStream_HEADER
#define ossimCurlIStream_HEADER
#include <ossim/base/ossimIoStream.h>
#include "ossimCurlStreamBuffer.h"

namespace ossim{

   class CurlIStream : public ossim::istream
   {
   public:
      CurlIStream():std::istream(&m_curlStreamBuffer)
      {}

      void open (const char* connectionString,  
                 std::ios_base::openmode mode)
      {
         open(std::string(connectionString), mode);
      }
      void open (const std::string& connectionString, 
                 std::ios_base::openmode mode)
      {
        if(m_curlStreamBuffer.open(connectionString, mode))
        {
            clear();
        }
        else
        {
            setstate(std::ios::failbit);
        }
      }
   protected:
     CurlStreamBuffer m_curlStreamBuffer;

   };
}

#endif
