#include "S3StreamDefaults.h"
#include <ossim/base/ossimPreferences.h>
#include <ossim/base/ossimEnvironmentUtility.h>
#include <ossim/base/ossimTrace.h>

ossim_int64 ossim::S3StreamDefaults::m_readBlocksize = 32768;
ossim_int64 ossim::S3StreamDefaults::m_nReadCacheHeaders = 10000;
bool ossim::S3StreamDefaults::m_cacheInvalidLocations = true;

static ossimTrace traceDebug("ossimS3StreamDefaults:debug");

void ossim::S3StreamDefaults::loadDefaults()
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossim::S3StreamDefaults::loadDefaults() DEBUG: entered.....\n";
   }
   ossimString s3ReadBlocksize       = ossimEnvironmentUtility::instance()->getEnvironmentVariable("OSSIM_PLUGINS_AWS_S3_READBLOCKSIZE");
   ossimString nReadCacheHeaders     = ossimEnvironmentUtility::instance()->getEnvironmentVariable("OSSIM_PLUGINS_AWS_S3_NREADCACHEHEADERS");
   ossimString cacheInvalidLocations = ossimEnvironmentUtility::instance()->getEnvironmentVariable("OSSIM_PLUGINS_AWS_S3_CACHEINVALIDLOCATIONS");
 
   
   if(s3ReadBlocksize.empty())
   {
       s3ReadBlocksize = ossimPreferences::instance()->findPreference("ossim.plugins.aws.s3.readBlocksize");
   }
   if(cacheInvalidLocations.empty())
   {
       cacheInvalidLocations = ossimPreferences::instance()->findPreference("ossim.plugins.aws.s3.cacheInvalidLocations");
   }
   if(!cacheInvalidLocations.empty())
   {
      m_cacheInvalidLocations = cacheInvalidLocations.toBool();
   }
   if(!s3ReadBlocksize.empty())
   {
     ossim_int64 blockSize = s3ReadBlocksize.memoryUnitToInt64();
   }
   if(nReadCacheHeaders.empty())
   {
     nReadCacheHeaders = ossimPreferences::instance()->findPreference("ossim.plugins.aws.s3.nReadCacheHeaders");
   }
   if(!nReadCacheHeaders.empty())
   {
      m_nReadCacheHeaders =  nReadCacheHeaders.toInt64();
      if(m_nReadCacheHeaders < 0)
      {
        m_nReadCacheHeaders = 10000;
      }     
   }
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "m_readBlocksize: " << m_readBlocksize << "\n";
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "m_nReadCacheHeaders: " << m_nReadCacheHeaders << "\n";
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossim::S3StreamDefaults::loadDefaults() DEBUG: leaving.....\n";
   }
}
