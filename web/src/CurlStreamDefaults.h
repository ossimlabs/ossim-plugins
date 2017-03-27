#ifndef CurlStreamDefaults_HEADER
#define CurlStreamDefaults_HEADER 1
#include <ossim/base/ossimConstants.h>

namespace ossim{
   

   class CurlStreamDefaults
   { 
      public:
         static void loadDefaults();

         static ossim_int64 m_readBlocksize;
         static ossim_int64 m_nReadCacheHeaders;

   };

}

#endif
