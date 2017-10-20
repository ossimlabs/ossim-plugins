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
#include <ossim/plugin/ossimPluginConstants.h>
#include "ossimOpenCvObjectFactory.h"
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimObjectFactoryRegistry.h>

static void setOpenCVDescription(ossimString& description)
{
   description = "OpenCV plugin\n\n";
}


extern "C"
{
   ossimSharedObjectInfo  myOpenCVInfo;
   ossimString theOpenCVDescription;
   std::vector<ossimString> theOpenCVObjList;
   
   const char* getOpenCVDescription()
   {
      return theOpenCVDescription.c_str();
   }
   
   int getOpenCVNumberOfClassNames()
   {
      return (int)theOpenCVObjList.size();
   }
   
   const char* getOpenCVClassName(int idx)
   {
      if(idx < (int)theOpenCVObjList.size())
      {
         return theOpenCVObjList[idx].c_str();
      }
      return (const char*)0;
   }
   
   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(
      ossimSharedObjectInfo** info)
   {    
      myOpenCVInfo.getDescription = getOpenCVDescription;
      myOpenCVInfo.getNumberOfClassNames = getOpenCVNumberOfClassNames;
      myOpenCVInfo.getClassName = getOpenCVClassName;
      
      *info = &myOpenCVInfo;
      
     /* Register objects... */
     ossimObjectFactoryRegistry::instance()->
        registerFactory(ossimOpenCvObjectFactory::instance());
      
      setOpenCVDescription(theOpenCVDescription);
      ossimOpenCvObjectFactory::instance()->getTypeNameList(theOpenCVObjList);
   }
   
   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
   {
     ossimObjectFactoryRegistry::instance()->
        unregisterFactory(ossimOpenCvObjectFactory::instance());
   }
}
