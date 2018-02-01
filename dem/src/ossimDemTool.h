//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#ifndef ossimDemTool_HEADER
#define ossimDemTool_HEADER 1

#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/util/ossimTool.h>
#include <ossim/reg/PhotoBlock.h>
#include <memory>

class OSSIM_DLL ossimDemTool : public ossimTool
{
public:
   enum Algorithm { ALGO_UNASSIGNED=0, ASP, OMG };
   enum Method { METHOD_UNASSIGNED=0, GET_ALGO_LIST, GET_PARAMS, GENERATE };

   static const char* DESCRIPTION;

   ossimDemTool();

   virtual ~ossimDemTool();

   virtual void setUsage(ossimArgumentParser& ap);

   virtual bool initialize(ossimArgumentParser& ap);

   virtual bool execute();

   virtual ossimString getClassName() const { return "ossimDemTool"; }

   virtual void getKwlTemplate(ossimKeywordlist& kwl);

   virtual void loadJSON(const Json::Value& json);

   virtual void saveJSON(Json::Value& json) const { json = m_responseJSON; }

private:
   void getAlgorithms();
   void getParameters();

   void doASP();
   void doOMG();

   std::ostream* m_outputStream;
   bool m_verbose;
   Algorithm m_algorithm;
   Method m_method;
   std::string m_configuration;
   Json::Value m_responseJSON;
   std::shared_ptr<ossim::PhotoBlock> m_photoBlock;
};

#endif /* #ifndef ossimDemTool_HEADER */
