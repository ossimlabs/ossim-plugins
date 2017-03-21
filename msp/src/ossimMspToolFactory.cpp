//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include <ossim/util/ossimToolRegistry.h>
#include "ossimMspTool.h"
#include "ossimMspToolFactory.h"

using namespace std;

ossimMspToolFactory* ossimMspToolFactory::s_instance = 0;

ossimMspToolFactory* ossimMspToolFactory::instance()
{
   if (!s_instance)
      s_instance = new ossimMspToolFactory;
   return s_instance;
}

ossimMspToolFactory::ossimMspToolFactory()
{
}

ossimMspToolFactory::~ossimMspToolFactory()
{
   ossimToolRegistry::instance()->unregisterFactory(this);
}

ossimTool* ossimMspToolFactory::createUtility(const std::string& argName) const
{
   ossimString utilName (argName);
   utilName.downcase();

   if ((utilName == "potrace") || (argName == "ossimPotraceUtil"))
      return new ossimMspTool;

   return 0;
}

void ossimMspToolFactory::getCapabilities(std::map<std::string, std::string>& capabilities) const
{
   capabilities.insert(pair<string, string>("potrace", ossimMspTool::DESCRIPTION));
}

std::map<std::string, std::string> ossimMspToolFactory::getCapabilities() const
{
   std::map<std::string, std::string> result;
   getCapabilities(result);
   return result;
}

void ossimMspToolFactory::getTypeNameList(vector<ossimString>& typeList) const
{
   typeList.push_back("ossimPotraceUtil");
}
