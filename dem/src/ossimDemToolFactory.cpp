//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include <ossim/util/ossimToolRegistry.h>

#include "ossimDemToolFactory.h"
#include "ossimDemTool.h"

using namespace std;

ossimDemToolFactory* ossimDemToolFactory::s_instance = 0;

ossimDemToolFactory* ossimDemToolFactory::instance()
{
   if (!s_instance)
      s_instance = new ossimDemToolFactory;
   return s_instance;
}

ossimDemToolFactory::ossimDemToolFactory()
{
}

ossimDemToolFactory::~ossimDemToolFactory()
{
   ossimToolRegistry::instance()->unregisterFactory(this);
}

ossimTool* ossimDemToolFactory::createTool(const std::string& argName) const
{
   ossimString utilName (argName);
   utilName.downcase();

   if ((utilName == "registration") || (argName == "ossimRegUtil"))
      return new ossimDemTool;

   return 0;
}

void ossimDemToolFactory::getCapabilities(std::map<std::string, std::string>& capabilities) const
{
   capabilities.insert(pair<string, string>("registration", ossimDemTool::DESCRIPTION));
}

std::map<std::string, std::string> ossimDemToolFactory::getCapabilities() const
{
   std::map<std::string, std::string> result;
   getCapabilities(result);
   return result;
}

void ossimDemToolFactory::getTypeNameList(vector<ossimString>& typeList) const
{
   typeList.push_back("ossimRegUtil");
}
