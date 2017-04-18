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
#include "ossimRegTool.h"

class ossimString;
class ossimFilename;
class ossimKeywordlist;

class OSSIM_PLUGINS_DLL ossimRegToolFactory: public ossimToolFactoryBase
{
public:
   static ossimRegToolFactory* instance();

   virtual ~ossimRegToolFactory();
   virtual ossimTool* createTool(const std::string& typeName) const;
   virtual void getTypeNameList(std::vector<ossimString>& typeList) const;
   virtual void getCapabilities(std::map<std::string, std::string>& capabilities) const;
   virtual std::map<std::string, std::string> getCapabilities() const;

protected:
   ossimRegToolFactory();
   ossimRegToolFactory(const ossimRegToolFactory&);
   void operator=(const ossimRegToolFactory&);

   /** static instance of this class */
   static ossimRegToolFactory* s_instance;

};

#endif /* end of #ifndef ossimPotraceUtilFactory_HEADER */
