//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include <ossim/util/ossimToolRegistry.h>
#include "ossimRegTool.h"
#include "ossimRegToolFactory.h"

using namespace std;

ossimRegToolFactory* ossimRegToolFactory::s_instance = 0;

ossimRegToolFactory* ossimRegToolFactory::instance()
{
   if (!s_instance)
      s_instance = new ossimRegToolFactory;
   return s_instance;
}

ossimRegToolFactory::ossimRegToolFactory()
{
}

ossimRegToolFactory::~ossimRegToolFactory()
{
   ossimToolRegistry::instance()->unregisterFactory(this);
}

ossimTool* ossimRegToolFactory::createTool(const std::string& argName) const
{
   ossimString utilName (argName);
   utilName.downcase();

   if ((utilName == "registration") || (argName == "ossimRegUtil"))
      return new ossimRegTool;

   return 0;
}

void ossimRegToolFactory::getCapabilities(std::map<std::string, std::string>& capabilities) const
{
   capabilities.insert(pair<string, string>("registration", ossimRegTool::DESCRIPTION));
}

std::map<std::string, std::string> ossimRegToolFactory::getCapabilities() const
{
   std::map<std::string, std::string> result;
   getCapabilities(result);
   return result;
}

void ossimRegToolFactory::getTypeNameList(vector<ossimString>& typeList) const
{
   typeList.push_back("ossimRegUtil");
}
