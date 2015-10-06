//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Hicks
//
// Description: OpenCV plugin initialization code.
//
//----------------------------------------------------------------------------
// $Id

#include <ossim/plugin/ossimSharedObjectBridge.h>
#include "ossimPluginConstants.h"
#include "ossimOpenCvObjectFactory.h"
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimObjectFactoryRegistry.h>

static void setDescription(ossimString& description)
{
   description = "OpenCV plugin\n\n";
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
      ossimSharedObjectInfo** info)
   {    
      myInfo.getDescription = getDescription;
      myInfo.getNumberOfClassNames = getNumberOfClassNames;
      myInfo.getClassName = getClassName;
      
      *info = &myInfo;
      
     /* Register objects... */
     ossimObjectFactoryRegistry::instance()->
        registerFactory(ossimOpenCvObjectFactory::instance());
      
      setDescription(theDescription);
   }
   
   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
   {
     ossimObjectFactoryRegistry::instance()->
        unregisterFactory(ossimOpenCvObjectFactory::instance());
   }
}
