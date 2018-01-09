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
#include "PhotoBlock.h"
#include <memory>

namespace ATP
{
class OSSIM_DLL ossimAtpTool : public ossimTool
{
public:
   enum Algorithm { ALGO_UNASSIGNED=0, CROSSCORR, DESCRIPTOR, NASA };
   enum Method { METHOD_UNASSIGNED=0, GET_ALGO_LIST, GET_PARAMS, GENERATE };

   static const char* DESCRIPTION;

   ossimAtpTool();

   virtual ~ossimAtpTool();

   virtual void setUsage(ossimArgumentParser& ap);

   virtual bool initialize(ossimArgumentParser& ap);

   virtual bool execute();

   virtual ossimString getClassName() const { return "ossimAtpTool"; }

   virtual void getKwlTemplate(ossimKeywordlist& kwl);

   virtual void loadJSON(const Json::Value& json);

   virtual void saveJSON(Json::Value& json) const { json = m_responseJSON; }

private:
   void getAlgorithms();
   void getParameters();
   void generate();

   /**
    * When the ATP generator works with image pairs (crosscorr and descriptor), This method is
    * used to loop over all image pairs and assemble the final tiepoint list for all */
   void doPairwiseMatching();

   std::ostream* m_outputStream;
   bool m_verbose;
   bool m_featureBased;
   Algorithm m_algorithm;
   Method m_method;
   std::string m_configuration;
   Json::Value m_responseJSON;
   std::shared_ptr<PhotoBlock> m_photoBlock;
};
}
#endif /* #ifndef ossimAtpTool_HEADER */
