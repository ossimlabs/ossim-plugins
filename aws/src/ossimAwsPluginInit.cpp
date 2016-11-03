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


static void setDescription(ossimString& description)
{
   description = "AWS S3 plugin\n\n";
}


extern "C"
{
   ossimSharedObjectInfo  myInfo;
   ossimString theDescription;
   std::vector<ossimString> theObjList;

   const char* getDescription()
   {
      return theDescription.c_str();
   }

   int getNumberOfClassNames()
   {
      return (int)theObjList.size();
   }

   const char* getClassName(int idx)
   {
      if(idx < (int)theObjList.size())
      {
         return theObjList[0].c_str();
      }
      return (const char*)0;
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(
      ossimSharedObjectInfo** info, const char* /*options*/)
   {    
      Aws::SDKOptions options;
      Aws::InitAPI(options);

      myInfo.getDescription = getDescription;
      myInfo.getNumberOfClassNames = getNumberOfClassNames;
      myInfo.getClassName = getClassName;
      
      *info = &myInfo;
      
      /* Register our stream factory... */
      ossim::StreamFactoryRegistry::instance()->
         registerFactory( ossim::AwsStreamFactory::instance() );

      setDescription(theDescription);
  }

   /* Note symbols need to be exported on windoze... */ 
  OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
  {
     ossim::StreamFactoryRegistry::instance()->
        unregisterFactory( ossim::AwsStreamFactory::instance() );
  }
}
