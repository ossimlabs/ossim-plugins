//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include <ossim/plugin/ossimSharedObjectBridge.h>
#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/base/ossimString.h>
#include <ossim/util/ossimUtilityRegistry.h>
#include "ossimPotraceUtilFactory.h"
#include "ossimPotraceUtil.h"

extern "C"
{
   ossimSharedObjectInfo  myInfo;

   const char* getDescription()
   {
      return "Potrace utility plugin\n\n";
   }

   int getNumberOfClassNames()
   {
      return 1;
   }

   const char* getClassName(int idx)
   {
      if (idx == 0)
      {
         return "ossimPotraceUtil";
      }
      return (const char*)0;
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(ossimSharedObjectInfo** info)
   {    
      myInfo.getDescription = getDescription;
      myInfo.getNumberOfClassNames = getNumberOfClassNames;
      myInfo.getClassName = getClassName;
      
      *info = &myInfo;
      
      /* Register the utility... */
      ossimUtilityRegistry::instance()->
         registerFactory(ossimPotraceUtilFactory::instance());
      
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
   {
      ossimUtilityRegistry::instance()->
         unregisterFactory(ossimPotraceUtilFactory::instance());
   }
}
