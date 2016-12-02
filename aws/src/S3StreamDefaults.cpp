#include "S3StreamDefaults.h"
#include <ossim/base/ossimPreferences.h>
#include <ossim/base/ossimEnvironmentUtility.h>
#include <ossim/base/ossimTrace.h>

// defaults to a 1 megabyte block size
//
ossim_int64 ossim::S3StreamDefaults::m_readBlocksize = 32768;
ossim_int64 ossim::S3StreamDefaults::m_nReadCacheHeaders = 10000;
static ossimTrace traceDebug("ossimS3StreamDefaults:debug");

void ossim::S3StreamDefaults::loadDefaults()
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossim::S3StreamDefaults::loadDefaults() DEBUG: entered.....\n";
   }
   ossimString s3ReadBlocksize = ossimEnvironmentUtility::instance()->getEnvironmentVariable("OSSIM_PLUGINS_AWS_S3_READBLOCKSIZE");

   ossimString  nReadCacheHeaders = ossimPreferences::instance()->findPreference("OSSIM_PLUGINS_AWS_S3_NREADCACHEHEADERS");

   
   if(s3ReadBlocksize.empty())
   {
       s3ReadBlocksize = ossimPreferences::instance()->findPreference("ossim.plugins.aws.s3.readBlocksize");
   }
   if(!s3ReadBlocksize.empty())
   {
     ossim_int64 blockSize = s3ReadBlocksize.toInt64();
     if(blockSize > 0)
     {
         ossimString byteType(s3ReadBlocksize.begin()+(s3ReadBlocksize.size()-1), s3ReadBlocksize.end());
         byteType.upcase();
         m_readBlocksize = blockSize;
         if ( byteType == "K")
         {
            m_readBlocksize *=static_cast<ossim_int64>(1024);
         }
         else if ( byteType == "M")
         {
            m_readBlocksize *=static_cast<ossim_int64>(1048576);
         }
         else if ( byteType == "G")
         {
            m_readBlocksize *=static_cast<ossim_int64>(1073741824);
         }
     }
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
