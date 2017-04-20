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
   if ( m_client )
   {
      delete m_client;
      m_client = 0;
   }
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
   f.convertBackToForwardSlashes();
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
   if ( !m_client )
   {
      initClient(); // First time through...
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
               // std::cout << "cached..." << std::endl;
               result = true;
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

                  // Add to cache ???
               }
            }
         }
      }
   }

   // ossimTimer::Timer_t endTimer = ossimTimer::instance()->tick();
   // cout << "time: " << ossimTimer::instance()->delta_s(startTimer, endTimer) << endl;

   // std::cout << "aws result: " << result << std::endl;
   
   return result;
}

// Hidden from use:
ossim::AwsStreamFactory::AwsStreamFactory()
{
}

// Hidden from use:
ossim::AwsStreamFactory::AwsStreamFactory(const ossim::AwsStreamFactory& )
{
}

void ossim::AwsStreamFactory::initClient() const
{
   Aws::Client::ClientConfiguration config;
   
   // Look for AWS S3 regionn override:
   std::string region = ossimPreferences::instance()->
      preferencesKWL().findKey(std::string("ossim.plugins.aws.s3.region"));
   if ( region.size() )
   {
      config.region = region.c_str();
   }
   
   m_client = new Aws::S3::S3Client( config );
}
