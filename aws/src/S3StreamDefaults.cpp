#include "S3StreamDefaults.h"
#include <ossim/base/ossimPreferences.h>

// defaults to a 1 megabyte block size
//
ossim_int64 ossim::S3StreamDefaults::m_readBlocksize = 1048576;
ossim_int64 ossim::S3StreamDefaults::m_nReadCacheHeaders = 10000;

void ossim::S3StreamDefaults::loadDefaults()
{
   ossimString s3ReadBlocksize = ossimPreferences::instance()->findPreference("ossim.plugins.aws.s3.readBlocksize");
   ossimString nReadCacheHeaders = ossimPreferences::instance()->findPreference("ossim.plugins.aws.s3.nReadCacheHeaders");

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
   if(!nReadCacheHeaders.empty())
   {
      m_nReadCacheHeaders =  nReadCacheHeaders.toInt64();
      if(m_nReadCacheHeaders < 0)
      {
        m_nReadCacheHeaders = 10000;
      }     
   }
}