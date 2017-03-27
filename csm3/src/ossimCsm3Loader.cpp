//**************************************************************************************************
// ossimCSM3Loader.cpp
//
// Author:  cchuah
//
// DESCRIPTION:
//   Contains implementation of class ossimCSM3Loader
//
//**************************************************************************************************
//  $Id: ossimCSM3Loader.cpp 1578 2015-06-08 16:27:20Z cchuah $


#include "ossimCsm3Loader.h"
#include "ossimCsm3SensorModel.h"
#include <ossim/base/ossimPreferences.h>
#include <ossim/base/ossimDirectory.h>
#include <ossim/base/ossimNotify.h>
#include <csm/Plugin.h>
#include <csm/NitfIsd.h>
#include "VTSMisc.h"

#ifdef _WIN32
#include <windows.h>
# include <io.h>
#else
# include <cstdio>  // for sprintf
# include <cstring> // for strstr
# include <dirent.h>
# include <dlfcn.h> // for dlopen, dlclose
# include <errno.h> // for errno
#endif


using namespace csm;

#ifdef _WIN32
    typedef HINSTANCE DllHandle;
    static const string dylibExt = ".dll";
#else
    typedef void* DllHandle;
    static const string dylibExt = ".so";
#endif

ossimCsm3Loader::ossimCsm3Loader()
{
    loadPlugins();
}

bool ossimCsm3Loader::loadPlugins()
{
   // get plugin path from the preferences file and verify it
   thePluginDir = ossimFilename(ossimPreferences::instance()->findPreference("csm3_plugin_path"));
   if(thePluginDir.empty())
      return false;

   // Load all of the dynamic libraries in the plugin path
   // first get the list of all the dynamic libraries found
   std::vector<ossimFilename> dllfiles;
   ossimDirectory pluginDir(thePluginDir);
   pluginDir.findAllFilesThatMatch(dllfiles, dylibExt );

   for (int i=0; i<dllfiles.size(); i++)
   {
      // when loaded, each dynamic libraries found will register itself in the Plugin object
      // the list is accessible using  PluginList pluginList = Plugin::getList( )

      ossimDynamicLibrary *lib = new ossimDynamicLibrary;
      if(lib->load(dllfiles[i]))
      {
         thePluginLibs.push_back(lib);
      }
      else
         ossimNotify(ossimNotifyLevel_WARN)
         << "loadPlugins: " + dllfiles[i] + "\" file failed to load." << std::endl;
      //if (NULL == dll)
      //    DWORD ret = GetLastError();
   }

   return true;
}


bool ossimCsm3Loader::removePlugin(const std::string& pluginName)
{
    WarningList warnings;
    Plugin::removePlugin(pluginName, &warnings);
           
    if(warnings.size() == 0)
    {
        // TBD: remove the shared object/DLL from theImpl->theDlls
        // TBD: close the shared object/DLL
        return true;
    }
    else
        ossimNotify(ossimNotifyLevel_WARN) 
        << "removePlugin: Cannot find plugin \"" + pluginName + "\" for removal." << std::endl;

    return false;
}


vector<string> ossimCsm3Loader::getAvailablePluginNames() const
{
	vector<string> returnVector;

    // now get the PluginList to get the PluginName
	PluginList pluginList = Plugin::getList( );

    // iterate through the PluginList to get the PluginName
	for( PluginList::const_iterator i = pluginList.begin(); i != pluginList.end(); i++ ) 
		returnVector.push_back( (*i)->getPluginName() );

	return returnVector;
} 


vector<string> ossimCsm3Loader::getAvailableSensorModelNames( std::string& pluginName ) const
{
	vector<string> returnVector;

    // now get the PluginList to get the PluginName
	PluginList pluginList = Plugin::getList( );

	// iterate through the plugins list, looking for the specific Plugin
	for( PluginList::const_iterator i = pluginList.begin(); i != pluginList.end(); i++ ) 
    {
		if( (*i)->getPluginName() != string(pluginName) )
			continue;

        // if the plugin is found, return the list of its sensor models
		for( size_t j = 0; j < (*i)->getNumModels(); j++ ) 
			returnVector.push_back( (*i)->getModelName( j ) );

		return returnVector;
    }

    ossimNotify(ossimNotifyLevel_WARN)
        << "getAvailableSensorModelNames: No plugins were found with the name \"" + pluginName + "\""
        << std::endl;

	return returnVector;
} 

RasterGM* ossimCsm3Loader::loadModelFromState(  std::string& pPluginName,
		std::string& pSensorModelName, std::string& pSensorState ) const
{
	try {
		// Make sure the input data is not NULL
		if( pPluginName.empty() || pSensorModelName.empty() || pSensorState.empty() ) 
        {
            ossimNotify(ossimNotifyLevel_WARN)
                << "loadModelFromState: Plugin Name, Sensor Model Name, and sensor state must be specified."
                << std::endl;

			return NULL;
		}

	    const Plugin* plugin = Plugin::findPlugin( pPluginName );
		if( plugin == NULL ) 
        {
            ossimNotify(ossimNotifyLevel_WARN)
                <<  "loadModelFromState: No plugin with the name \"" << pPluginName << "\" was found."
                << std::endl;
			return NULL;
		}

		// Load the sensor model using the appropriate information
		Model* sensorModel = NULL;
        bool constructible = false;

		// See if it's possible to construct the sensor model from the state
		constructible = plugin->canModelBeConstructedFromState(pSensorModelName, pSensorState);
		if( !constructible ) 
        {
            ossimNotify(ossimNotifyLevel_WARN)
                <<  "loadModelFromState: The specified state cannot be used to construct a sensor model."
                << std::endl;
			return NULL;
		}

		// Create the sensor model from the plugin and sensor state
		sensorModel = plugin->constructModelFromState( pSensorState );
		if( sensorModel == NULL ) 
        {
            ossimNotify(ossimNotifyLevel_WARN)
			    << "loadModelFromState: Failed to create sensor model from sensor state."
                << std::endl;
            return NULL;
		}
        else
		    return  dynamic_cast<RasterGM*>(sensorModel);
	} 
	catch (csm::Error& err) 
	{
		ossimNotify(ossimNotifyLevel_WARN) 
            << err.getFunction() << '\n' << err.getMessage() << '\n' << std::endl;
	}
	catch(...) 
    {
        ossimNotify(ossimNotifyLevel_WARN)
	        << "Exception thrown in ossimCSM3Loader::loadModelFromState"
            << std::endl;
    }
	return NULL;
}


RasterGM* ossimCsm3Loader::loadModelFromFile(  std::string& pPluginName,
		std::string& pSensorModelName, std::string& pInputImage, ossim_uint32 index ) const
{
	try {
		// Make sure the input data is not NULL
		if( pPluginName.empty() || pSensorModelName.empty() || pInputImage.empty() ) 
        {
            ossimNotify(ossimNotifyLevel_WARN)
                << "loadModelFromFile: Plugin Name, Sensor Model Name, and Image Name must be specified."
                << std::endl;
			return NULL;
		}

	    const Plugin* plugin = Plugin::findPlugin( pPluginName );
		if( plugin == NULL ) 
        {
            ossimNotify(ossimNotifyLevel_WARN)
                <<  "loadModelFromFile: No plugin with the name \"" << pPluginName << "\" was found."
                << std::endl;
			return NULL;
		}

		// Load the sensor model using the appropriate information
        WarningList warnings;
		Model* sensorModel = NULL;
        bool constructible = false;
        
        // First try to construct the sensor model from the filename ISD
		Isd* fnameIsd = new Isd( pInputImage );
		try {
			constructible = plugin->canModelBeConstructedFromISD( *fnameIsd, 
                                                        pSensorModelName, &warnings);
		}
		catch(...) 
		{
            delete fnameIsd;
			constructible = false;
		}

		if (constructible) 
		{
            ossimNotify(ossimNotifyLevel_INFO) << "Constructing sensor model via filename" << std::endl;
			sensorModel = plugin->constructModelFromISD( *fnameIsd, pSensorModelName );
            delete fnameIsd;
		    return  dynamic_cast<RasterGM*>(sensorModel);
		}

		// Next try a NITF 2.1 ISD
		Nitf21Isd* nitf21Isd = new Nitf21Isd( pInputImage );                
		try {
			initNitf21ISD( nitf21Isd, pInputImage, index, &warnings );
			constructible = plugin->canModelBeConstructedFromISD( *nitf21Isd, 
                                                    pSensorModelName, &warnings);
		} 
		catch(...) 
		{
            delete nitf21Isd;
			constructible = false;
		}

		if (constructible) 
		{
            ossimNotify(ossimNotifyLevel_INFO) << "Constructing sensor model from NITF 2.1 file" << std::endl;
			sensorModel = plugin->constructModelFromISD( *nitf21Isd, pSensorModelName );
            delete nitf21Isd;
		    return  dynamic_cast<RasterGM*>(sensorModel);
		}

		// finally try a NITF 2.0 ISD
		Nitf20Isd* nitf20Isd = new Nitf20Isd( pInputImage );
                
		try {
			initNitf20ISD( nitf20Isd, pInputImage, index, &warnings );
			constructible = plugin->canModelBeConstructedFromISD( *nitf20Isd, 
                                                    pSensorModelName, &warnings);
		} 
		catch(...) 
		{
            delete nitf20Isd;
			constructible = false;
		}

		if (constructible) 
		{
            ossimNotify(ossimNotifyLevel_INFO) << "Constructing sensor model from NITF 2.0 file" << std::endl;
			sensorModel = plugin->constructModelFromISD( *nitf20Isd, pSensorModelName );
            delete nitf20Isd;
		    return  dynamic_cast<RasterGM*>(sensorModel);
		}

        // if we reach here, we have failed to create a sensor model
		if( sensorModel == NULL ) 
        {
            ossimNotify(ossimNotifyLevel_WARN)
                << "loadModelFromFile: Unable to create sensor model \"" << pSensorModelName
                << "\" using the image \"" + pInputImage + "\""  << std::endl;
			return NULL;
		}
	} 
	catch (csm::Error& err) 
	{
		ossimNotify(ossimNotifyLevel_WARN) 
            << err.getFunction() << '\n' << err.getMessage() << '\n' << std::endl;
	}
	catch(...) 
	{
        ossimNotify(ossimNotifyLevel_WARN) 
            << "Exception thrown in ossimCSM3Loader::loadModelFromFile" << std::endl;
    }

	return NULL;
}


ossimCsm3SensorModel* ossimCsm3Loader::getSensorModel( std::string& filename, ossim_uint32 index) const
{
    ossimCsm3SensorModel* model = 0;
    std::string pluginName = "";
    std::string sensorName = "";
    std::string fname = filename;

    // now try to get available sensor model name and see if we can instantiate a sensor model from it
    RasterGM* internalCsmModel = 0;   
    std::vector<string> pluginNames = getAvailablePluginNames( );    
      
    for(int i = 0; i < pluginNames.size(); ++i)
    {
        std::vector<string> sensorModelNames = getAvailableSensorModelNames( pluginNames[i] );    
        for(int j = 0; j < sensorModelNames.size(); ++j)
        { 
            internalCsmModel = loadModelFromFile( pluginNames[i], sensorModelNames[j], filename, index);
            
            // if we successfully get an internal model, create the ossim sensor model from it
            if(internalCsmModel)
            {
                model = new ossimCsm3SensorModel( pluginNames[i], sensorModelNames[j], 
                                filename, internalCsmModel);
                return model;
            }
        }
    }

    return NULL;
}

//ossimCSM3Loader::imageTypeSelections ossimCSM3Loader::determineNITFType( const char* pInputImage )
//{
//	imageTypeSelections returnType = NOT_NITF; // default is that it is not an NITF
//    FILE *filePtr = fopen(pInputImage, "rt"); 
//	if (filePtr != NULL)
//	{
//		char line[80];
//
//		// file format is label:value
//		if ( fgets(line, 80, filePtr) != NULL)
//		{
//			if (strncmp(line, "NITF02.1", 8) == 0)
//			{
//				returnType = NITF21;
//			}
//			else if (strncmp(line, "NITF02.0", 8) == 0)
//			{
//				returnType = NITF20;
//			}
//			else
//			{
//				returnType = NOT_NITF;
//			}
//		}
//	}
//	fclose(filePtr);		
//
//	return returnType;
//}
