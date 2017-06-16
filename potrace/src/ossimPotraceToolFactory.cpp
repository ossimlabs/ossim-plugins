//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include <ossim/util/ossimToolRegistry.h>
#include <potrace/src/ossimPotraceTool.h>
#include <potrace/src/ossimPotraceToolFactory.h>

using namespace std;

ossimPotraceToolFactory* ossimPotraceToolFactory::s_instance = 0;

ossimPotraceToolFactory* ossimPotraceToolFactory::instance()
{
   if (!s_instance)
      s_instance = new ossimPotraceToolFactory;
   return s_instance;
}

ossimPotraceToolFactory::ossimPotraceToolFactory()
{
}

ossimPotraceToolFactory::~ossimPotraceToolFactory()
{
   ossimToolRegistry::instance()->unregisterFactory(this);
}

ossimTool* ossimPotraceToolFactory::createTool(const std::string& argName) const
{
   ossimString utilName (argName);
   utilName.downcase();

   if ((utilName == "potrace") || (argName == "ossimPotraceTool"))
      return new ossimPotraceTool;

   return 0;
}

void ossimPotraceToolFactory::getCapabilities(std::map<std::string, std::string>& capabilities) const
{
   capabilities.insert(pair<string, string>("potrace", ossimPotraceTool::DESCRIPTION));
}

std::map<std::string, std::string> ossimPotraceToolFactory::getCapabilities() const
{
   std::map<std::string, std::string> result;
   getCapabilities(result);
   return result;
}

void ossimPotraceToolFactory::getTypeNameList(vector<ossimString>& typeList) const
{
   typeList.push_back("ossimPotraceTool");
}
