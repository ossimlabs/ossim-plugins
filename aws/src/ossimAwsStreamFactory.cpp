//---
//
// License: MIT
//
// Description: OSSIM Amazon Web Services (AWS) plugin initialization
// code.
//
//---
// $Id$

#include "ossimAwsStreamFactory.h"
#include "ossimS3IStream.h"
#include "S3HeaderCache.h"

#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimPreferences.h>
#include <ossim/base/ossimStreamFactory.h>
#include <ossim/base/ossimTimer.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimUrl.h>

#include <aws/core/client/ClientConfiguration.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/HeadObjectRequest.h>


static ossimTrace traceDebug("ossimAwsStreamFactory:debug");

ossim::AwsStreamFactory* ossim::AwsStreamFactory::m_instance = 0;

ossim::AwsStreamFactory::~AwsStreamFactory()
{
}

ossim::AwsStreamFactory* ossim::AwsStreamFactory::instance()
{
   if( !m_instance )
   {
      m_instance = new ossim::AwsStreamFactory();
   }
   return m_instance;
}

std::shared_ptr<ossim::istream> ossim::AwsStreamFactory::createIstream(
   const std::string& connectionString, 
   const ossimKeywordlist& options,
   std::ios_base::openmode openMode) const
{
   std::shared_ptr<ossim::S3IStream> result = std::make_shared<ossim::S3IStream>();
   if(traceDebug())
   {
     ossimNotify(ossimNotifyLevel_WARN) << "ossim::AwsStreamFactory::createIstream: Entered...............\n";
   }
   
   //---
   // Hack for upstream code calling ossimFilename::convertToNative()
   // wrecking s3 url.
   //---
#if defined(_WIN32)
   ossimFilename f = connectionString;
   result->open( f.string(), options, openMode) ;
#else
   result->open( connectionString, options, openMode );
#endif
  
  if(!result->good())
  {
    result.reset();
  }
   if(traceDebug())
   {
     ossimNotify(ossimNotifyLevel_WARN) << "ossim::AwsStreamFactory::createIstream: Leaving...............\n";
   }

  return result;
}
      
std::shared_ptr<ossim::ostream> ossim::AwsStreamFactory::createOstream(
   const std::string& /*connectionString*/, 
   const ossimKeywordlist& /* options */,
   std::ios_base::openmode /*openMode*/) const
{
   return std::shared_ptr<ossim::ostream>(0);
}

std::shared_ptr<ossim::iostream> ossim::AwsStreamFactory::createIOstream(
   const std::string& /*connectionString*/, 
   const ossimKeywordlist& /* options */,
   std::ios_base::openmode /*openMode*/) const
{
   return std::shared_ptr<ossim::iostream>(0);
}

bool ossim::AwsStreamFactory::exists(
   const std::string& connectionString, bool& continueFlag) const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossim::AwsStreamFactory::exists() DEBUG: entered.....\n";

   }
   bool result = false;
   
   // ossimTimer::Timer_t startTimer = ossimTimer::instance()->tick();

   if ( connectionString.size() )
   {
      // Check for url string:
      std::size_t pos = connectionString.find( "://" );
      if ( pos != std::string::npos )
      {
         ossimUrl url(connectionString);
         if( (url.getProtocol() == "s3") || (url.getProtocol() == "S3") )
         {
            continueFlag = false; // Set to false to stop registry search.
            
            ossim_int64 filesize;
            if(ossim::S3HeaderCache::instance()->getCachedFilesize(connectionString, filesize))
            {
               if(filesize >=0)
               {
                  result = true;
               }
            }
            else
            {
               std::string bucket = url.getIp().string();
               std::string key    = url.getPath().string();
               Aws::S3::Model::HeadObjectRequest headObjectRequest;
               headObjectRequest.WithBucket( bucket.c_str() )
                  .WithKey( key.c_str() );
               auto headObject = m_client->HeadObject(headObjectRequest);
               if(headObject.IsSuccess())
               {
                  result = true;
                  filesize = headObject.GetResult().GetContentLength();
                  ossim::S3HeaderCache::Node_t nodePtr = std::make_shared<ossim::S3HeaderCacheNode>(filesize);
                  ossim::S3HeaderCache::instance()->addHeader(connectionString, nodePtr);               
               }
               else
               {
                  result = false;                  
                  ossim::S3HeaderCache::Node_t nodePtr = std::make_shared<ossim::S3HeaderCacheNode>(-1);
                  ossim::S3HeaderCache::instance()->addHeader(connectionString, nodePtr);               
               }
            }
         }
      }
   }

   // ossimTimer::Timer_t endTimer = ossimTimer::instance()->tick();
   // cout << "time: " << ossimTimer::instance()->delta_s(startTimer, endTimer) << endl;

   // std::cout << "aws result: " << result << std::endl;
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossim::AwsStreamFactory::exists() DEBUG: leaving.....\n";

   }
   
   return result;
}

// Hidden from use:
ossim::AwsStreamFactory::AwsStreamFactory()
{
   // the stream is now shared so we must allocate here.  
   // 
   initClient();
}

// Hidden from use:
ossim::AwsStreamFactory::AwsStreamFactory(const ossim::AwsStreamFactory& )
{
}

void ossim::AwsStreamFactory::initClient() const
{
   Aws::Client::ClientConfiguration config;
   
   // Look for AWS S3 region override:
   std::string region = ossimPreferences::instance()->
      preferencesKWL().findKey(std::string("ossim.plugins.aws.s3.region"));
   if ( region.size() )
   {
      config.region = region.c_str();
   }
   
   m_client = std::make_shared<Aws::S3::S3Client>(config);
//new Aws::S3::S3Client( config );
}
