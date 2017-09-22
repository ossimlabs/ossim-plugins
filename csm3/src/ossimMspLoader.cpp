//**************************************************************************************************
// ossimMspLoader.cpp
//
// Author:  cchuah
//
// DESCRIPTION:
//   Contains implementation of class ossimMspLoader
//
//**************************************************************************************************
//  $Id: ossimMspLoader.cpp 1578 2015-06-08 16:27:20Z cchuah $


#include "ossimMspLoader.h"
#include "ossimCsm3SensorModel.h"
#include <SensorModel/SensorModelService.h>
#include <ossim/base/ossimPreferences.h>
#include <ossim/base/ossimDirectory.h>
#include <ossim/base/ossimNotify.h>
#include <csm/Plugin.h>
#include <csm/NitfIsd.h>
#include "VTSMisc.h"

#ifdef _WIN32
#include <windows.h>
# include <io.h>
typedef HINSTANCE DllHandle;
static const string dylibExt = ".dll";
#else
# include <cstdio>  // for sprintf
# include <cstring> // for strstr
# include <dirent.h>
# include <dlfcn.h> // for dlopen, dlclose
# include <errno.h> // for errno
#endif


using namespace csm;
using namespace std;

#ifdef _WIN32
#else
typedef void* DllHandle;
static const string dylibExt = ".so";
#endif

ossimMspLoader::ossimMspLoader()
{
}


void ossimMspLoader::getAvailablePluginNames(vector<string>& plugins)
{
   plugins.clear();
   MSP::SMS::NameList pluginList;

   try
   {
      MSP::SMS::SensorModelService sms;
      sms.getAllRegisteredPlugins(pluginList);
   }
   catch(exception &mspError)
   {
      cout<<"Exception: "<<mspError.what()<<endl;;
   }

   MSP::SMS::NameList::iterator plugin = pluginList.begin();
   while (plugin != pluginList.end())
   {
      plugins.push_back(*plugin);
      ++plugin;
   }
} 


void ossimMspLoader::getAvailableSensorModelNames(string& pluginName, vector<string>& models )
{
   models.clear();
   MSP::SMS::NameList modelList;

   try
   {
      MSP::SMS::SensorModelService sms;
      sms.listModels(pluginName, modelList);
   }
   catch(exception &mspError)
   {
      cout<<"Exception: "<<mspError.what()<<endl;;
   }

   MSP::SMS::NameList::iterator model = modelList.begin();
   while (model != modelList.end())
   {
      models.push_back(*model);
      ++model;
   }
} 

RasterGM* ossimMspLoader::loadModelFromState(const string& modelState )
{
   RasterGM* rgm = 0;
   try
   {
      // Make sure the input data is not NULL
      if( modelState.empty() )
      {
         ossimNotify(ossimNotifyLevel_WARN)<< "ossimMspLoader::loadModelFromState(): "
               "Sensor state is empty." << std::endl;
         return NULL;
      }

      MSP::SMS::SensorModelService sms;
      csm::Model* base = sms.createModelFromState(modelState.c_str());
      rgm = dynamic_cast<csm::RasterGM*>(base);
   }
   catch(exception &mspError)
   {
      cout<<"Exception: "<<mspError.what()<<endl;;
   }

   return rgm;
}


RasterGM* ossimMspLoader::loadModelFromFile(const std::string& sensorModelName,
                                            const ossimFilename& imageFilename,
                                            ossim_uint32 index)
{
   RasterGM* rgm = 0;

   try
   {
      MSP::SMS::SensorModelService sms;
      const char* modelName = 0;
      if (sensorModelName.size())
         modelName = sensorModelName.c_str();
      MSP::ImageIdentifier entry ("IMAGE_INDEX", ossimString::toString(index).string());
      sms.setPluginPreferencesRigorousBeforeRpc();
      csm::Model* base = sms.createModelFromFile(imageFilename.c_str(), modelName, &entry);
      rgm = dynamic_cast<csm::RasterGM*>(base);
   }
   catch (exception& mspError)
   {
      cout<<"Exception: "<<mspError.what()<<endl;;
   }

   return rgm;
}


ossimCsm3SensorModel* ossimMspLoader::getSensorModel(const ossimFilename& filename,
                                                                ossim_uint32 index)
{
   ossimCsm3SensorModel* model = 0;
   std::string pluginName = "";
   std::string sensorName = "";
   std::string fname = filename;

   // Try to get available sensor model name and see if we can instantiate a sensor model from it
   RasterGM* internalCsmModel = 0;
   std::vector<string> pluginNames;
   getAvailablePluginNames(pluginNames);

   int numPlugins = pluginNames.size();
   for(int i = 0; (i < numPlugins) && !model; ++i)
   {
      std::vector<string> sensorModelNames;
      getAvailableSensorModelNames( pluginNames[i], sensorModelNames );
      int numModels = sensorModelNames.size();

      for(int j = 0; (j < numModels) && !model; ++j)
      {
         internalCsmModel = loadModelFromFile(sensorModelNames[j], filename, index);
         if(internalCsmModel)
         {
            model = new ossimCsm3SensorModel( pluginNames[i], sensorModelNames[j],
                                              filename, internalCsmModel);
         }
      }
   }

   return model;
}

//ossimMspLoader::imageTypeSelections ossimMspLoader::determineNITFType( const char* pInputImage )
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
