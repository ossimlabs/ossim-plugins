//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#ifndef ossimAtpToolFactory_HEADER
#define ossimAtpToolFactory_HEADER 1

#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/util/ossimToolFactoryBase.h>

class ossimString;
class ossimFilename;
class ossimKeywordlist;

namespace ATP {

class OSSIM_PLUGINS_DLL ossimAtpToolFactory: public ossimToolFactoryBase
{
public:
   static ossimAtpToolFactory* instance();

   virtual ~ossimAtpToolFactory();
   virtual ossimTool* createTool(const std::string& typeName) const;
   virtual void getTypeNameList(std::vector<ossimString>& typeList) const;
   virtual void getCapabilities(std::map<std::string, std::string>& capabilities) const;
   virtual std::map<std::string, std::string> getCapabilities() const;

protected:
   ossimAtpToolFactory();
   ossimAtpToolFactory(const ossimAtpToolFactory&);
   void operator=(const ossimAtpToolFactory&);

   /** static instance of this class */
   static ossimAtpToolFactory* s_instance;

};
}
#endif /* end of #ifndef ossimAtpToolFactory_HEADER */
