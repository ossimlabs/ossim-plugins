//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  cchuah
//
// DESCRIPTION:
//   Implementation of class ossimCsm3PluginInit
//
//**************************************************************************************************
//  $Id: ossimCsm3PluginInit.cpp 1579 2015-06-08 17:09:57Z cchuah $

#include "ossimCsm3ProjectionFactory.h"
#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/plugin/ossimSharedObjectBridge.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <csm/Plugin.h>

#include <vector>
#include <sstream>

extern "C"
{
static ossimSharedObjectInfo  myCsm3Info;
static ossimString theCsm3Description;
static std::vector<ossimString> theCsm3ObjList;

static const char* getCsm3Description()
{
   if (theCsm3Description.empty())
   {
      std::ostringstream out;
      out  << "Community Sensor Model 3.0.1 Plugin. ";

      // by this time, the plugins have already been registered by the ossimCsm3ProjectionFactory
      // the Plugin object should already contain a list of the available plugins
      // Get the plugins list
      csm::PluginList pluginList = csm::Plugin::getList( );

      // list all available plugins in the description
      if(pluginList.empty())
         out << "No plugins were found in directory.\n";
      else
      {
         out << "Available plugins are: \n";
         // iterate through the PluginList to get the PluginName
         for( csm::PluginList::const_iterator i = pluginList.begin(); i != pluginList.end(); i++ )
            out << "     "<< (*i)->getPluginName() << "\n";
      }

      theCsm3Description = out.str();
   }
   return theCsm3Description.c_str();
}

static int getCsm3NumberOfClassNames()
{
   return (int)theCsm3ObjList.size();
}

static const char* getCsm3ClassName(int idx)
{
   if(idx < (int)theCsm3ObjList.size())
   {
      return theCsm3ObjList[idx].c_str();
   }
   return (const char*)0;
}

OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(
      ossimSharedObjectInfo** info,  const char* /* options */ )
{
   theCsm3ObjList.push_back("ossimCsm3SensorModel");

   myCsm3Info.getDescription = getCsm3Description;
   myCsm3Info.getNumberOfClassNames = getCsm3NumberOfClassNames;
   myCsm3Info.getClassName = getCsm3ClassName;

   *info = &myCsm3Info;

   /* Register the ProjectionFactory */
   ossimProjectionFactoryRegistry::instance()->
         registerFactoryToFront(ossimCsm3ProjectionFactory::instance());
}

OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
{
   ossimProjectionFactoryRegistry::instance()->
         unregisterFactory(ossimCsm3ProjectionFactory::instance());
}
}
