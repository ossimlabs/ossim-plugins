//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#ifndef ossimDemToolFactory_HEADER
#define ossimDemToolFactory_HEADER 1

#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/util/ossimToolFactoryBase.h>
#include "ossimDemTool.h"

class ossimString;
class ossimFilename;
class ossimKeywordlist;

class OSSIM_PLUGINS_DLL ossimDemToolFactory: public ossimToolFactoryBase
{
public:
   static ossimDemToolFactory* instance();

   virtual ~ossimDemToolFactory();
   virtual ossimTool* createTool(const std::string& typeName) const;
   virtual void getTypeNameList(std::vector<ossimString>& typeList) const;
   virtual void getCapabilities(std::map<std::string, std::string>& capabilities) const;
   virtual std::map<std::string, std::string> getCapabilities() const;

protected:
   ossimDemToolFactory();
   ossimDemToolFactory(const ossimDemToolFactory&);
   void operator=(const ossimDemToolFactory&);

   /** static instance of this class */
   static ossimDemToolFactory* s_instance;

};

#endif
