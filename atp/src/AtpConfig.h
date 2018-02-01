//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#pragma once

#include <ostream>
#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/base/ossimString.h>
#include <ossim/init/JsonConfig.h>
#include <vector>
#include <map>

namespace ATP
{
/**
 * Singleton class maintaining parameters affecting the automatic tie point generation.
 * The state is imported and exported via JSON. There are default configuration files that must
 * be part of the install, that are accessed by this class. Custom settings can also be st
 */
class OSSIM_PLUGINS_DLL AtpConfig : public ossim::JsonConfig
{
public:
   //! Singleton implementation.
   static AtpConfig& instance();

   //! Destructor
   virtual ~AtpConfig();

   bool readConfig(const std::string& configName="");

private:
   AtpConfig();
   AtpConfig(const AtpConfig& /*hide_this*/) {}

};

}
