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
   enum Units { UNITS_UNASSIGNED=0, METERS, DEGREES };

   static const char* DESCRIPTION;

   ossimDemTool();

   ~ossimDemTool() override;

   void setUsage(ossimArgumentParser& ap) override;

   bool initialize(ossimArgumentParser& ap) override;

   bool execute() override;

   ossimString getClassName() const override { return "ossimDemTool"; }

   void getKwlTemplate(ossimKeywordlist& kwl) override;

   void loadJSON(const Json::Value& json) override;

   void saveJSON(Json::Value& json) const override { json = m_responseJSON; }

private:
   void getAlgorithms();
   void getParameters();

   void doASP();
   void doOMG();

   std::ostream* m_outputStream;
   bool m_verbose;
   Algorithm m_algorithm;
   Method m_method;
   Json::Value m_responseJSON;
   std::shared_ptr<ossim::PhotoBlock> m_photoBlock;
   ossimFilename m_outputDemFile;
   double m_postSpacing;
   Units m_postSpacingUnits;
   Json::Value m_parameters;
   Json::Value m_atpParameters;
};

#endif /* #ifndef ossimDemTool_HEADER */
