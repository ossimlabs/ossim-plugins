//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "ossimAtpToolFactory.h"
#include <ossim/plugin/ossimSharedObjectBridge.h>
#include <ossim/util/ossimToolRegistry.h>

extern "C"
{
ossimSharedObjectInfo  atpInfo;

const char* getAtpDescription()
{
   return "ossim-atp tool plugin\n\n";
}

int getAtpNumberOfClassNames()
{
   return 1;
}

const char* getAtpClassName(int idx)
{
   if (idx == 0)
   {
      return "ossimAtpTool";
   }
   return (const char*)0;
}

/* Note symbols need to be exported on windoze... */
OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(ossimSharedObjectInfo** info)
{
   atpInfo.getDescription = getAtpDescription;
   atpInfo.getNumberOfClassNames = getAtpNumberOfClassNames;
   atpInfo.getClassName = getAtpClassName;

   *info = &atpInfo;

   /* Register the utility... */
   ossimToolRegistry::instance()->registerFactory(ATP::ossimAtpToolFactory::instance());

}

/* Note symbols need to be exported on windoze... */
OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
{
   ossimToolRegistry::instance()->unregisterFactory(ATP::ossimAtpToolFactory::instance());
}
}
