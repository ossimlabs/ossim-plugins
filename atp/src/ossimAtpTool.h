//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#ifndef ossimAtpTool_HEADER
#define ossimAtpTool_HEADER 1

#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/util/ossimTool.h>
#include "AutoTiePointService.h"
#include <memory>

namespace ATP
{
class OSSIM_DLL ossimAtpTool : public ossimTool
{
public:
   static const char* DESCRIPTION;

   ossimAtpTool();

   virtual ~ossimAtpTool();

   virtual void setUsage(ossimArgumentParser& ap);

   virtual bool initialize(ossimArgumentParser& ap);

   virtual void initialize(const std::string& json_query);

   virtual bool execute();

   virtual ossimString getClassName() const { return "ossimAtpTool"; }

   virtual void getKwlTemplate(ossimKeywordlist& kwl);

private:
   std::istream* m_inputStream;
   std::ostream* m_outputStream;
   bool m_verbose;
   bool m_featureBased;
   std::shared_ptr<AutoTiePointService> m_service;
   std::string m_jsonRequest;

};
}
#endif /* #ifndef ossimAtpTool_HEADER */
