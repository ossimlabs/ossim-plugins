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
#include <ossim/plugin/ossimSharedObjectBridge.h>
#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/base/ossimStreamFactoryRegistry.h>
#include <aws/core/Aws.h>
#include "S3StreamDefaults.h"

static void setDescription(ossimString& description)
{
   description = "AWS S3 plugin\n\n";
}


extern "C"
{
   ossimSharedObjectInfo  awsInfo;
   ossimString awsDescription;
   std::vector<ossimString> awsObjList;

   const char* getAwsDescription()
   {
      return awsDescription.c_str();
   }

   int getAwsNumberOfClassNames()
   {
      return (int)awsObjList.size();
   }

   const char* getAwsClassName(int idx)
   {
      if(idx < (int)awsObjList.size())
      {
         return awsObjList[idx].c_str();
      }
      return (const char*)0;
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(
      ossimSharedObjectInfo** info, const char* /*options*/)
   {
      Aws::SDKOptions options;
      Aws::InitAPI(options);

      awsInfo.getDescription = getAwsDescription;
      awsInfo.getNumberOfClassNames = getAwsNumberOfClassNames;
      awsInfo.getClassName = getAwsClassName;
      
      *info = &awsInfo;
      
      ossim::S3StreamDefaults::loadDefaults();
      /* Register our stream factory... */
      ossim::StreamFactoryRegistry::instance()->
         registerFactory( ossim::AwsStreamFactory::instance() );

      setDescription(awsDescription);
  }

   /* Note symbols need to be exported on windoze... */ 
  OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
  {
     ossim::StreamFactoryRegistry::instance()->
        unregisterFactory( ossim::AwsStreamFactory::instance() );
  }
}
