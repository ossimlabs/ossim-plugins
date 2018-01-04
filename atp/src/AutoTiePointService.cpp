//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************



#include <AutoTiePointService.h>
#include <math.h>
#include <PhotoBlock.h>
#include <SessionManager.h>
#include <Session.h>
#include <correlation/CorrelationAtpGenerator.h>
#include <descriptor/DescriptorAtpGenerator.h>
#include <json/json.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimGpt.h>
#include <ossim/base/ossimEcefPoint.h>
#include <iostream>
#include <cstdlib>

using namespace std;

namespace ATP
{

AutoTiePointService::AutoTiePointService()
:  m_algorithm(ALGO_UNASSIGNED),
   m_method(METHOD_UNASSIGNED)
{
   m_responseJSON.clear();
}

AutoTiePointService::~AutoTiePointService()
{
}

void AutoTiePointService::loadJSON(const Json::Value& queryRoot)
{
   ostringstream xmsg;
   xmsg<<"AutoTiePointService::loadJSON()  ";

   string service = queryRoot["service"].asString();
   if (service != "autoTiePoint")
   {
      xmsg << "Fatal: ATP service called but input JSON doesn't contain the request!";
      throw ossimException(xmsg.str());
   }

   // Fetch the desired method:
   string method = queryRoot["method"].asString();
   if (method == "getAlgorithms")
      m_method = GET_ALGO_LIST;
   else if (method == "getParameters")
      m_method = GET_PARAMS;
   else if (method == "generate")
      m_method = GENERATE;
   else
   {
      xmsg << "Fatal error. No valid method selected. The value provided was \""
            <<method<<"\" which is not among the permitted strings.";
      throw ossimException(xmsg.str());
   }

   // Fetch the desired algorithm or configuration:
   AtpConfig& config = AtpConfig::instance();
   string algorithm = queryRoot["algorithm"].asString();
   if (algorithm.empty())
   {
      // An optional field specifies a configuration that contains the base-name of a custom
      // parameters JSON file including algorithm type. Read that if specified:
      m_configuration = queryRoot["configuration"].asString();
      if (!m_configuration.empty())
      {
         if (!config.readConfig(m_configuration))
         {
            xmsg << "Fatal error trying to read default ATP configuration for selected "
                  "configuration <"<<m_configuration<<">";
            throw ossimException(xmsg.str());
         }
         algorithm = config.getParameter("algorithm").asString();
      }
   }
   // Otherwise read the algorithm's default config params from the system:
   else if (!config.readConfig(algorithm))
   {
      xmsg << "Fatal error trying to read default ATP configuration for selected algorithm <"
            <<algorithm<<">";
      throw ossimException(xmsg.str());
   }

   // Assign enum data member used throughout the service:
   if (algorithm == "crosscorr")
      m_algorithm = CROSSCORR;
   else if (method == "descriptor")
      m_algorithm = DESCRIPTOR;
   else if (method == "nasa")
      m_algorithm = NASA;
   else
      m_algorithm = ALGO_UNASSIGNED;

   // If parameters were provided in the JSON payload, have the config override the defaults:
   const Json::Value& parameters = queryRoot["parameters"];
   if (!parameters.isNull())
      config.loadJSON(parameters);

   if (config.diagnosticLevel(2))
      clog<<"\nATP configuration after loading:\n"<<config<<endl;

   if (m_method == GENERATE)
   {
      // Load the active photoblock from JSON:
      const Json::Value& pbJson = queryRoot["photoblock"];
      if (!pbJson)
      {
         xmsg <<__FILE__<<":"<<__LINE__<<" Fatal: Null photoblock returned!";
         throw ossimException(xmsg.str());
      }
      m_photoBlock.reset(new PhotoBlock(pbJson));
   }
}

void AutoTiePointService::execute()
{
   ostringstream xmsg;
   xmsg<<"AutoTiePointService::execute()  ";

   switch (m_method)
   {
   case GET_ALGO_LIST:
      getAlgorithms();
      break;
   case GET_PARAMS:
      getParameters();
      break;
   case GENERATE:
      generate();
      break;
   default:
      xmsg << "Fatal: No method selected prior to execute being called. I don't know what to do!";
      throw ossimException(xmsg.str());
   }
}

void AutoTiePointService::getAlgorithms()
{
   m_responseJSON.clear();

   Json::Value algoList;
   algoList[0]["name"]        = "crosscorr";
   algoList[0]["description"] = "Matching by cross-correlation of raster patches";
   algoList[0]["label"]       = "Cross Correlation";
   //algoList[1]["name"]        = "descriptor";
   //algoList[1]["description"] = "Matching by feature descriptors";
   //algoList[1]["label"]       = "Descriptor";
   //algoList[2]["name"]        = "nasa";
   //algoList[2]["description"] = "Tiepoint extraction using NASA tool.";
   //algoList[2]["label"]       = "NASA";

   m_responseJSON["algorithms"] = algoList;
}

void AutoTiePointService::getParameters()
{
   m_responseJSON.clear();

   Json::Value params;
   AtpConfig::instance().saveJSON(params);

   m_responseJSON["parameters"] = params;

}

void AutoTiePointService::generate()
{
   ostringstream xmsg;
   xmsg<<"AutoTiePointService::generate()  ";

   switch (m_algorithm)
   {
   case CROSSCORR:
   case DESCRIPTOR:
      doPairwiseMatching();
      break;
   case NASA:
      xmsg << "NASA Algorithm not yet implemented!";
      throw ossimException(xmsg.str());
      break;
   case ALGO_UNASSIGNED:
   default:
      xmsg << "Fatal: No algorithm selected prior to execute being called. I don't know what to do!";
      throw ossimException(xmsg.str());
   }
}

void AutoTiePointService::doPairwiseMatching()
{
   static const char* MODULE = "AutoTiePointService::doPairwiseMatching()  ";
   ostringstream xmsg;
   xmsg<<MODULE;

   if (!m_photoBlock || (m_photoBlock->getImageList().size() < 2))
   {
      xmsg << "No photoblock has been declared or it has less than two images. Cannot perform ATP.";
      throw ossimException(xmsg.str());
   }

   shared_ptr<AtpGeneratorBase> generator;

   // First obtain list of images from photoblock:
   std::vector<shared_ptr<Image> >& imageList = m_photoBlock->getImageList();
   int numImages = (int) imageList.size();
   int numPairs = (numImages * (numImages-1))/2; // triangular number assumes all overlap
   for (int i=0; i<(numImages-1); i++)
   {
      shared_ptr<Image> imageA = imageList[i];
      for (int j=i+1; j<numImages; j++)
      {
         shared_ptr<Image> imageB = imageList[j];

         // Instantiate the ATP generator for this pair:
         if (m_algorithm == CROSSCORR)
            generator.reset(new CorrelationAtpGenerator());
         else if (m_algorithm == DESCRIPTOR)
            generator.reset(new DescriptorAtpGenerator());
         else if (m_algorithm == NASA)
         {
            xmsg << "NASA algorithm not yet supported. Cannot perform ATP.";
            throw ossimException(xmsg.str());
         }

         // Generate tie points using A for features:
         generator->setRefImage(imageA);
         generator->setCmpImage(imageB);
         ATP::AtpList tpList;
         generator->generateTiePointList(tpList);
         m_photoBlock->addTiePoints(tpList);

         if (AtpConfig::instance().diagnosticLevel(2))
            clog<<"Completed pairwise match for pair "<<i<<"-"<<j<<endl;

         // Now reverse CMP and REF to find possible additional features on B:
         bool doTwoWaySearch = AtpConfig::instance().getParameter("doTwoWaySearch").asBool();
         if (doTwoWaySearch)
         {
            generator->setRefImage(imageB);
            generator->setCmpImage(imageA);
            tpList.clear();
            generator->generateTiePointList(tpList);
            m_photoBlock->addTiePoints(tpList);

            if (AtpConfig::instance().diagnosticLevel(2))
               clog<<"Completed pairwise match for pair "<<j<<"-"<<i<<endl;
         }
      }
   }

   // Save list to photoblock JSON:
   Json::Value pbJson;
   m_photoBlock->saveJSON(pbJson);
   m_responseJSON["photoblock"] = pbJson;

   AtpConfig& config = AtpConfig::instance();
   if (config.diagnosticLevel(1))
      clog<<"\n"<<MODULE<<"Total TPs found = "<<m_photoBlock->getTiePointList().size()<<endl;
}


} // end O2REG namespace

