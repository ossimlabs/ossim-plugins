#ifndef S3StreamDefaults_HEADER
#define S3StreamDefaults_HEADER 1
#include <ossim/base/ossimConstants.h>

namespace ossim{
   

   class S3StreamDefaults
   { 
      public:
         static void loadDefaults();

         static ossim_int64 m_readBlocksize;

   };

}

#endif
