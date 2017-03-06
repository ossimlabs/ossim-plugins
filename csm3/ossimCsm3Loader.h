//**************************************************************************************************
// ossimCSM3Loader.h
//
// Author:  cchuah
//
// This is the class responsible for loading all the sensor models found in the
// CSM plugin folder
//
//**************************************************************************************************
//  $Id: ossimCSM3Loader.h 1577 2015-06-05 18:47:18Z cchuah $

#ifndef CSM3Loader_HEADER
#define CSM3Loader_HEADER

#include <ossim/base/ossimFilename.h>
#include <ossim/plugin/ossimDynamicLibrary.h>
#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/base/ossimRefPtr.h>
#include <RasterGM.h>
#include <string>
#include <vector>

class ossimCsm3SensorModel;

class OSSIM_PLUGINS_DLL ossimCsm3Loader 
{
public:
   /*!
    * Constructor
    */
    ossimCsm3Loader();

   /*!
    * Returns available plugins found in plugin path
    * plguin path is specified in prefrence file, in keyword "csm3_plugin_path"
    */
	std::vector<std::string> getAvailablePluginNames() const;

   /*!
    * Returns a list of sensor model names contained in the specified plugin 
    */
	std::vector<std::string> getAvailableSensorModelNames( std::string& pPluginName) const;
    
   /*!
    * Returns the sensor model for the specified image file name 
    */
    ossimCsm3SensorModel* getSensorModel( std::string& filename, ossim_uint32 index) const;

   /*!
    * Remove the specified plugin from our list.
    * All available plugins are loaded automatically in the constructor.
    */
    bool removePlugin(const std::string& pluginName);

	//enum imageTypeSelections { NITF20, NITF21, NOT_NITF };
	//static imageTypeSelections determineNITFType(const char* pInputImage);

protected:

    ossimFilename thePluginDir;
    std::vector<std::string> thePluginNames;
    std::vector<std::string> theSensorModelNames;
    std::vector<ossimRefPtr<ossimDynamicLibrary> > thePluginLibs;

   /*!
    * Called by constructor to load all plugins found in plugin path.
    */
    bool loadPlugins();

   /*!
    * Load the named sensor models from the named plugin for the input image file
    */
    csm::RasterGM* loadModelFromFile( std::string& pPluginName, std::string& pSensorModelName, 
			        std::string& pInputImage, ossim_uint32 index) const;

    /*!
    * Load the named sensor models from the named plugin from the sensor state
    */
    csm::RasterGM* loadModelFromState( std::string& pPluginName, std::string& pSensorModelName, 
			        std::string& pSensorState) const;

};


#endif

