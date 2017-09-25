//**************************************************************************************************
// OSSIM Open Source Geospatial Data Processing Library
// See top level LICENSE.txt file for license information
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

#include "ossimCsm3Config.h"
#include <ossim/base/ossimFilename.h>
#include <ossim/plugin/ossimPluginConstants.h>
#include <csm/RasterGM.h>
#include <string>
#include <vector>

class ossimCsm3SensorModel;

/**
 * This is the class responsible for loading all the sensor models found in the CSM plugin folder
 */
class OSSIM_PLUGINS_DLL ossimCsm3Loader 
{
public:
   /** Needs to be instantiated once to load all CSM plugins (unless MSP is used) */
   ossimCsm3Loader();

   /*!
    * Returns available plugins found in plugin path
    * plguin path is specified in prefrence file, in keyword "csm3_plugin_path"
    */
   static void getAvailablePluginNames(std::vector<std::string>& plugins);

   /*!
    * Returns a list of sensor model names contained in the specified plugin 
    */
   static void getAvailableSensorModelNames(const std::string& pluginName,
                                            std::vector<std::string>& models);
    
   /*!
    * Returns the sensor model for the specified image file name 
    */
	static ossimCsm3SensorModel* getSensorModel(const ossimFilename& filename, ossim_uint32 index);

   /*!
    * Load the named sensor models from the named plugin for the input image file
    */
	static csm::RasterGM* loadModelFromFile(const std::string& pPluginName,
	                                        const std::string& pSensorModelName,
	                                        const std::string& pInputImage,
	                                         ossim_uint32 index=0);

    /*!
    * Load the named sensor models from the named plugin from the sensor state
    */
	static csm::RasterGM* loadModelFromState(const std::string& pPluginName,
	                                         const std::string& pSensorModelName,
	                                         const std::string& pSensorState);
};


#endif

