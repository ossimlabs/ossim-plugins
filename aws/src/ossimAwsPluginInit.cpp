//---
//
// License: MIT
//
// Description: OSSIM Amazon Web Services (AWS) plugin initialization
// code.
//
//---
// $Id$

#include <ossim/plugin/ossimSharedObjectBridge.h>
#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/base/ossimStreamFactoryRegistry.h>

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
      myInfo.getDescription = getDescription;
      myInfo.getNumberOfClassNames = getNumberOfClassNames;
      myInfo.getClassName = getClassName;
      
      *info = &myInfo;
      
      /* Register our stream factory... */
      // ossimStreamFactoryRegistry::instance()->
      //   registerFactory(ossimAwsStreamFactory::instance());
  }

   /* Note symbols need to be exported on windoze... */ 
  OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
  {
     // ossimStreamFactoryRegistry::instance()->
     //   unregisterFactory(ossimAwsStreamFactory::instance());
  }
}
