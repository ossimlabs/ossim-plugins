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
#include "ossimDemTool.h"
#include "ossimDemToolFactory.h"

extern "C"
{
   ossimSharedObjectInfo  myDemInfo;

   const char* getDemDescription()
   {
      return "Dem utility plugin\n\n";
   }

   int getDemNumberOfClassNames()
   {
      return 1;
   }

   const char* getDemClassName(int idx)
   {
      if (idx == 0)
      {
         return "ossimDemTool";
      }
      return (const char*)0;
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(ossimSharedObjectInfo** info)
   {    
      myDemInfo.getDescription = getDemDescription;
      myDemInfo.getNumberOfClassNames = getDemNumberOfClassNames;
      myDemInfo.getClassName = getDemClassName;
      
      *info = &myDemInfo;
      
      /* Demister the utility... */
      ossimToolRegistry::instance()->
         registerFactory(ossimDemToolFactory::instance());
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
   {
      ossimToolRegistry::instance()->
         unregisterFactory(ossimDemToolFactory::instance());
   }
}
