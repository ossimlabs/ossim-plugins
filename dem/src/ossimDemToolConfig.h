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

/**
 * Singleton class maintaining parameters affecting the automatic tie point generation.
 * The state is imported and exported via JSON. There are default configuration files that must
 * be part of the install, that are accessed by this class. Custom settings can also be st
 */
class OSSIM_PLUGINS_DLL ossimDemToolConfig : public ossim::JsonConfig
{
public:
   //! Singleton implementation.
   static ossimDemToolConfig& instance();

   //! Destructor
   virtual ~ossimDemToolConfig();

   bool readConfig(const std::string& configName="");

   //! Convenience method returns TRUE if the currently set diagnostic level is <= level
   bool diagnosticLevel(unsigned int level) const;

private:
   ossimDemToolConfig();
   ossimDemToolConfig(const ossimDemToolConfig& /*hide_this*/) {}

};


