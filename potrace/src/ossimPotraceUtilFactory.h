//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#ifndef ossimPotraceUtilFactory_HEADER
#define ossimPotraceUtilFactory_HEADER 1

#include <ossim/util/ossimUtilityFactoryBase.h>
#include <ossim/plugin/ossimPluginConstants.h>
#include "ossimPotraceUtil.h"

class ossimString;
class ossimFilename;
class ossimKeywordlist;

class OSSIM_PLUGINS_DLL ossimPotraceUtilFactory: public ossimUtilityFactoryBase
{
public:
   static ossimPotraceUtilFactory* instance();

   virtual ~ossimPotraceUtilFactory();
   virtual ossimUtility* createUtility(const std::string& typeName) const;
   virtual void getTypeNameList(std::vector<ossimString>& typeList) const;
   virtual void getCapabilities(std::map<std::string, std::string>& capabilities) const;
   virtual std::map<std::string, std::string> getCapabilities() const;

protected:
   ossimPotraceUtilFactory();
   ossimPotraceUtilFactory(const ossimPotraceUtilFactory&);
   void operator=(const ossimPotraceUtilFactory&);

   /** static instance of this class */
   static ossimPotraceUtilFactory* s_instance;

};

#endif /* end of #ifndef ossimPotraceUtilFactory_HEADER */
