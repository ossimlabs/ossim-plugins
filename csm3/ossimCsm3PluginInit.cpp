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
#include <Plugin.h>

#include <vector>
#include <sstream>

static void setDescription(ossimString& description)
{
    std::ostringstream out;
    out  << "Community Sensor Model 3.0.1 Plugin\n\n";
  
    // by this time, the plugins have already been registered by the ossimCsm3ProjectionFactory
    // the Plugin object should already contain a list of the available plugins
    // Get the plugins list
	csm::PluginList pluginList = csm::Plugin::getList( );

    // list all available plugins in the description
    if(pluginList.empty())
        out << "No plugins were found in directory.\n\n";
    else
    {
        out << "\nAvailable plugins are: \n\n";
        // iterate through the PluginList to get the PluginName
	    for( csm::PluginList::const_iterator i = pluginList.begin(); i != pluginList.end(); i++ ) 
            out <<  (*i)->getPluginName() << "\n\n";
    }

    description = out.str();
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

    OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(
        ossimSharedObjectInfo** info,  const char* /* options */ )
    {    
        myInfo.getDescription = getDescription;
        myInfo.getNumberOfClassNames = getNumberOfClassNames;
        myInfo.getClassName = getClassName;
      
        *info = &myInfo;
        /* Register the ProjectionFactory */
        ossimProjectionFactoryRegistry::instance()->
        registerFactoryToFront(ossimCsm3ProjectionFactory::instance());
 
        setDescription(theDescription);
    }

    OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
    {
        ossimProjectionFactoryRegistry::instance()->
            unregisterFactory(ossimCsm3ProjectionFactory::instance());
    }
}
