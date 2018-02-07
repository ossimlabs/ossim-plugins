//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "ossimDemToolConfig.h"

#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimPreferences.h>
#include <memory>
#include <ossim/base/ossimNotify.h>

using namespace std;
using namespace ossim;

ossimDemToolConfig& ossimDemToolConfig::instance()
{
   static ossimDemToolConfig singleton;
   return singleton;
}

ossimDemToolConfig::ossimDemToolConfig()
{

   // Register the ISA-common parameters in the params map:
   readConfig();
}
   
ossimDemToolConfig::~ossimDemToolConfig()
{
   m_paramsMap.clear();
}

bool ossimDemToolConfig::readConfig(const string& cn)
{
   // This method could eventually curl a spring config server for the param JSON. For now it
   // is reading the installed share/ossim-isa system directory for config JSON files.

   // The previous parameters list is cleared first for a fresh start:
   m_paramsMap.clear();

   ossimFilename configFilename;
   Json::Value jsonRoot;
   try
   {
      // First establish the directory location of the default config files:
      ossimFilename shareDir = ossimPreferences::instance()->
         preferencesKWL().findKey( std::string( "ossim_share_directory" ) );
      shareDir += "/dem";
      if (!shareDir.isDir())
         throw ossimException("Nonexistent share drive provided for config files.");

      // Read the default common parameters first:
      configFilename = "demToolConfig.json";
      configFilename.setPath(shareDir);
      if (!open(configFilename))
         throw ossimException("Bad file open or parse.");

      // Read the algorithm-specific default parameters if generic algo name specified as config:
//      ossimString configName (cn);
//      configFilename.clear();
//      if (configName == "crosscorr")
//         configFilename = "crossCorrConfig.json";
//      else if (configName == "descriptor")
//         configFilename = "descriptorConfig.json";
//      else if (configName == "nasa")
//         configFilename = "nasaConfig.json";
//      else if (!configName.empty())
//      {
//         // Custom configuration. TODO: the path here is still the install's share directory.
//         // Eventually want to provide a database access to the config JSON.
//         configFilename = configName + ".json";
//      }

      // Load the specified configuration. This will override the common defaults:
//      if (!configFilename.empty())
//      {
//         configFilename.setPath(shareDir);
//         if (!open(configFilename))
//            throw ossimException("Bad file open or parse.");
//      }
   }
   catch (ossimException& e)
   {
      ossimNotify(ossimNotifyLevel_WARN)<<"ossimDemConfig::readConfig():  Could not open/parse "
            "config file at <"<< configFilename << ">. Error: "<<e.what()<<endl;
      return false;
   }
   return true;
}

