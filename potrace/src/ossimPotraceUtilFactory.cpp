//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "ossimPotraceUtilFactory.h"
#include <ossim/util/ossimUtilityRegistry.h>
#include "ossimPotraceUtil.h"

using namespace std;

ossimPotraceUtilFactory* ossimPotraceUtilFactory::s_instance = 0;

ossimPotraceUtilFactory* ossimPotraceUtilFactory::instance()
{
   if (!s_instance)
      s_instance = new ossimPotraceUtilFactory;
   return s_instance;
}

ossimPotraceUtilFactory::ossimPotraceUtilFactory()
{
}

ossimPotraceUtilFactory::~ossimPotraceUtilFactory()
{
   ossimUtilityRegistry::instance()->unregisterFactory(this);
}

ossimUtility* ossimPotraceUtilFactory::createUtility(const std::string& argName) const
{
   ossimString utilName (argName);
   utilName.downcase();

   if ((utilName == "potrace") || (argName == "ossimPotraceUtil"))
      return new ossimPotraceUtil;

   return 0;
}

void ossimPotraceUtilFactory::getCapabilities(std::map<std::string, std::string>& capabilities) const
{
   capabilities.insert(pair<string, string>("potrace", ossimPotraceUtil::DESCRIPTION));
}

std::map<std::string, std::string> ossimPotraceUtilFactory::getCapabilities() const
{
   std::map<std::string, std::string> result;
   getCapabilities(result);
   return result;
}

void ossimPotraceUtilFactory::getTypeNameList(vector<ossimString>& typeList) const
{
   typeList.push_back("ossimPotraceUtil");
}
