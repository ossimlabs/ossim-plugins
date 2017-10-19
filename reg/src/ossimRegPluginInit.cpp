//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include <ossim/plugin/ossimSharedObjectBridge.h>
#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/base/ossimString.h>
#include <ossim/util/ossimToolRegistry.h>
#include "ossimRegTool.h"
#include "ossimRegToolFactory.h"

extern "C"
{
   ossimSharedObjectInfo  myRegInfo;

   const char* getRegDescription()
   {
      return "Reg utility plugin\n\n";
   }

   int getRegNumberOfClassNames()
   {
      return 1;
   }

   const char* getRegClassName(int idx)
   {
      if (idx == 0)
      {
         return "ossimRegTool";
      }
      return (const char*)0;
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(ossimSharedObjectInfo** info)
   {    
      myRegInfo.getDescription = getRegDescription;
      myRegInfo.getNumberOfClassNames = getRegNumberOfClassNames;
      myRegInfo.getClassName = getRegClassName;
      
      *info = &myRegInfo;
      
      /* Register the utility... */
      ossimToolRegistry::instance()->
         registerFactory(ossimRegToolFactory::instance());
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
   {
      ossimToolRegistry::instance()->
         unregisterFactory(ossimRegToolFactory::instance());
   }
}
