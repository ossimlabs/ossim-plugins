#ifndef CurlStreamDefaults_HEADER
#define CurlStreamDefaults_HEADER 1
#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimFilename.h>

namespace ossim{
   

   class CurlStreamDefaults
   { 
      public:
         static void loadDefaults();

         static ossim_int64 m_readBlocksize;
         static ossim_int64 m_nReadCacheHeaders;
         static ossimFilename m_cacert;
         static ossimFilename m_clientCert;
         static ossimFilename m_clientKey;
         static ossimString m_clientCertType;
         static ossimString m_clientKeyPassword;
   };

}

#endif
