//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#ifndef ossimPotraceUtilFactory_HEADER
#define ossimPotraceUtilFactory_HEADER 1

#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/util/ossimToolFactoryBase.h>
#include "ossimMspTool.h"

class ossimString;
class ossimFilename;
class ossimKeywordlist;

class OSSIM_PLUGINS_DLL ossimMspToolFactory: public ossimToolFactoryBase
{
public:
   static ossimMspToolFactory* instance();

   virtual ~ossimMspToolFactory();
   virtual ossimTool* createUtility(const std::string& typeName) const;
   virtual void getTypeNameList(std::vector<ossimString>& typeList) const;
   virtual void getCapabilities(std::map<std::string, std::string>& capabilities) const;
   virtual std::map<std::string, std::string> getCapabilities() const;

protected:
   ossimMspToolFactory();
   ossimMspToolFactory(const ossimMspToolFactory&);
   void operator=(const ossimMspToolFactory&);

   /** static instance of this class */
   static ossimMspToolFactory* s_instance;

};

#endif /* end of #ifndef ossimPotraceUtilFactory_HEADER */
