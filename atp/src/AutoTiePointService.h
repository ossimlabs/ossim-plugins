//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#ifndef AutoTiePointService_HEADER
#define AutoTiePointService_HEADER 1

#include <ServiceBase.h>
#include <PhotoBlock.h>
#include <AtpConfig.h>
#include <vector>
#include <string>
#include <memory>

namespace ATP
{

/**
 * Top-level class for interfacing to automatic tie-point generation service from 3DISA client.
 */
class AutoTiePointService : public ServiceBase
{
public:
   enum Algorithm { ALGO_UNASSIGNED=0, CROSSCORR, DESCRIPTOR, NASA };
   enum Method { METHOD_UNASSIGNED=0, GET_ALGO_LIST, GET_PARAMS, GENERATE };

   AutoTiePointService();
   ~AutoTiePointService();

   /*
   * Refer to <a href="https://docs.google.com/document/d/1DXekmYm7wyo-uveM7mEu80Q7hQv40fYbtwZq-g0uKBs/edit?usp=sharing">3DISA API document</a>
   * for JSON format used.
   */
   virtual void loadJSON(const Json::Value& json);

   /*
   * Refer to <a href="https://docs.google.com/document/d/1DXekmYm7wyo-uveM7mEu80Q7hQv40fYbtwZq-g0uKBs/edit?usp=sharing">3DISA API document</a>
   * for JSON format used.
   */
   virtual void saveJSON(Json::Value& json) const { json = m_responseJSON; }

   virtual void execute();

private:
   void getAlgorithms();
   void getParameters();
   void generate();

   /**
    * When the ATP generator works with image pairs (crosscorr and descriptor), This method is
    * used to loop over all image pairs and assemble the final tiepoint list for all */
   void doPairwiseMatching();

   std::shared_ptr<PhotoBlock> m_photoBlock;
   Algorithm m_algorithm;
   Method m_method;
   std::string m_configuration;
   Json::Value m_responseJSON;
};

} // End namespace ATP

#endif
