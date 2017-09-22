//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#ifndef ossimMspLoader_HEADER
#define ossimMspLoader_HEADER


#include <ossim/base/ossimFilename.h>
#include <ossim/plugin/ossimDynamicLibrary.h>
#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/base/ossimRefPtr.h>
#include <csm/RasterGM.h>
#include <string>
#include <vector>
#include <memory>

class ossimCsm3SensorModel;

class OSSIM_PLUGINS_DLL ossimMspLoader
{
public:
   /** Returns available plugins found in plugin path */
	static void getAvailablePluginNames(std::vector<std::string>& plugins);

   /** Returns a list of sensor model names contained in the specified plugin */
	static void getAvailableSensorModelNames(std::string& pluginName,
	                                  std::vector<std::string>& models);
    
   /** Returns the sensor model for the specified image file name */
	static ossimCsm3SensorModel* getSensorModel(const ossimFilename& inputImage,
	                                            ossim_uint32 index);

   /**
    * Load the named sensor models from the named plugin for the input image file
    */
	static csm::RasterGM* loadModelFromFile(const std::string&   sensorModelName,
                                     const ossimFilename& inputImage,
                                     ossim_uint32   entryIndex);

    /**
    * Load the named sensor models from the named plugin from the sensor state
    */
	static csm::RasterGM* loadModelFromState(const std::string& modelState);

private:
   ossimMspLoader();

};


#endif

