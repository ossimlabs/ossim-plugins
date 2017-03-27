#include "CurlStreamDefaults.h"
#include <ossim/base/ossimPreferences.h>
#include <ossim/base/ossimEnvironmentUtility.h>
#include <ossim/base/ossimTrace.h>

// defaults to a 1 megabyte block size
//
ossim_int64 ossim::CurlStreamDefaults::m_readBlocksize = 32768;
ossim_int64 ossim::CurlStreamDefaults::m_nReadCacheHeaders = 10000;
static ossimTrace traceDebug("ossimCurlStreamDefaults:debug");

void ossim::CurlStreamDefaults::loadDefaults()
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossim::CurlStreamDefaults::loadDefaults() DEBUG: entered.....\n";
   }
   ossimString curlReadBlocksize = ossimEnvironmentUtility::instance()->getEnvironmentVariable("OSSIM_PLUGINS_WEB_CURL_READBLOCKSIZE");

   ossimString  nReadCacheHeaders = ossimPreferences::instance()->findPreference("OSSIM_PLUGINS_WEB_CURL_NREADCACHEHEADERS");

   
   if(curlReadBlocksize.empty())
   {
       curlReadBlocksize = ossimPreferences::instance()->findPreference("ossim.plugins.web.curl.readBlocksize");
   }
   if(!curlReadBlocksize.empty())
   {
     ossim_int64 blockSize = curlReadBlocksize.toInt64();
     if(blockSize > 0)
     {
         ossimString byteType(curlReadBlocksize.begin()+(curlReadBlocksize.size()-1), curlReadBlocksize.end());
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
     nReadCacheHeaders = ossimPreferences::instance()->findPreference("ossim.plugins.web.curl.nReadCacheHeaders");
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
         << "ossim::CurlStreamDefaults::loadDefaults() DEBUG: leaving.....\n";
   }
}
