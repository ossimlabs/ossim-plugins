//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "ossimAtpToolFactory.h"
#include "ossimAtpTool.h"
#include <ossim/util/ossimToolRegistry.h>

using namespace std;

namespace ATP
{
ossimAtpToolFactory* ossimAtpToolFactory::s_instance = 0;

ossimAtpToolFactory* ossimAtpToolFactory::instance()
{
   if (!s_instance)
      s_instance = new ossimAtpToolFactory;
   return s_instance;
}

ossimAtpToolFactory::ossimAtpToolFactory()
{
}

ossimAtpToolFactory::~ossimAtpToolFactory()
{
   ossimToolRegistry::instance()->unregisterFactory(this);
}

ossimTool* ossimAtpToolFactory::createTool(const std::string& argName) const
{
   ossimString utilName (argName);
   utilName.downcase();

   if ((utilName == "atp") || (argName == "ossimAtpTool"))
      return new ossimAtpTool;

   return 0;
}

void ossimAtpToolFactory::getCapabilities(std::map<std::string, std::string>& capabilities) const
{
   capabilities.insert(pair<string, string>("atp", ossimAtpTool::DESCRIPTION));
}

std::map<std::string, std::string> ossimAtpToolFactory::getCapabilities() const
{
   std::map<std::string, std::string> result;
   getCapabilities(result);
   return result;
}

void ossimAtpToolFactory::getTypeNameList(vector<ossimString>& typeList) const
{
   typeList.push_back("ossimAtpTool");
}
}
