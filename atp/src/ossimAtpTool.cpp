//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "ossimAtpTool.h"
#include <math.h>
#include "correlation/CorrelationAtpGenerator.h"
#include "descriptor/DescriptorAtpGenerator.h"
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimGpt.h>
#include <ossim/base/ossimEcefPoint.h>
#include <ossim/base/ossimArgumentParser.h>
#include <ossim/base/ossimApplicationUsage.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimException.h>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>

using namespace std;

#define CINFO  ossimNotify(ossimNotifyLevel_INFO)
#define CWARN  ossimNotify(ossimNotifyLevel_WARN)
#define CFATAL ossimNotify(ossimNotifyLevel_FATAL)

namespace ATP
{
const char* ossimAtpTool::DESCRIPTION =
      "Provides automatic tiepoint generation functionality. This tool uses JSON format to "
      "communicate requests and results.";

ossimAtpTool::ossimAtpTool()
: m_inputStream (0),
  m_outputStream (0),
  m_verbose (false),
  m_featureBased (true)
{
}

ossimAtpTool::~ossimAtpTool()
{
}

void ossimAtpTool::setUsage(ossimArgumentParser& ap)
{
   // Add global usage options. Don't include ossimChipProcUtil options as not appropriate.
   ossimTool::setUsage(ap);

   // Set the general usage:
   ossimApplicationUsage* au = ap.getApplicationUsage();
   ossimString usageString = ap.getApplicationName();
   usageString += " atp [options] \n\n";
   usageString +=
         "Accesses automatic tiepoint generation functionality given JSON request on stdin (or input file if\n"
         "-i specified). The response JSON is output to stdout unless -o option is specified.\n";
   au->setCommandLineUsage(usageString);

   // Set the command line options:
   au->addCommandLineOption("--algorithms",
         "List available auto-tie-point algorithms");
   au->addCommandLineOption("-i <filename>",
         "Reads request JSON from the input file specified instead of stdin.");
   au->addCommandLineOption("-o <filename>",
         "Outputs response JSON to the output file instead of stdout.");
   au->addCommandLineOption("--parameters",
         "List all ATP parameters with default values.");
   au->addCommandLineOption("-v",
         "Verbose. All non-response (debug) output to stdout is enabled.");
}

bool ossimAtpTool::initialize(ossimArgumentParser& ap)
{
   string ts1;
   ossimArgumentParser::ossimParameter sp1(ts1);
   m_method = GENERATE;

   if (!ossimTool::initialize(ap))
      return false;
   if (m_helpRequested)
      return true;

   if ( ap.read("-v"))
      m_verbose = true;

   if ( ap.read("--algorithms"))
      m_method = GET_ALGO_LIST;

   if ( ap.read("--parameters"))
      m_method = GET_PARAMS;

   if ( ap.read("-i", sp1))
   {
      ifstream* s = new ifstream (ts1);
      if (s->fail())
      {
         CFATAL<<__FILE__<<" Could not open input file <"<<ts1<<">";
         return false;
      }
      m_inputStream = s;
   }
   else
      m_inputStream = &cin;

   if ( ap.read("-o", sp1))
   {
      ofstream* s = new ofstream (ts1);
      if (s->fail())
      {
         CFATAL<<__FILE__<<" Could not open output file <"<<ts1<<">";
         return false;
      }
      m_outputStream = s;
   }
   else
      m_outputStream = &clog;

   return true;
}

void ossimAtpTool::loadJSON(const Json::Value& queryRoot)
{
   ostringstream xmsg;
   xmsg<<"ossimAtpTool::loadJSON()  ";

   // Fetch the desired method from the JSON if provided, otherwise rely on command line options:
   m_method = GENERATE;
   string method = queryRoot["method"].asString();
   if (method == "getAlgorithms")
      m_method = GET_ALGO_LIST;
   else if (method == "getParameters")
      m_method = GET_PARAMS;

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
      // Load the active photoblock from JSON.
      // The root JSON object can optionally contain a photoblock with image list contained. If so,
      // use that object to load images, otherwise use the root.
      if (queryRoot.isMember("photoblock"))
         m_photoBlock.reset(new PhotoBlock(queryRoot["photoblock"]));
      else
         m_photoBlock.reset(new PhotoBlock(queryRoot));
   }
}

bool ossimAtpTool::execute()
{
   ostringstream xmsg;

   try
   {
      // Read input:
      Json::Value queryRoot;
      (*m_inputStream) >> queryRoot;

      // Call actual service and get response:
      loadJSON(queryRoot);

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

      // Serialize JSON object for return:
      (*m_outputStream) << m_responseJSON;
   }
   catch(ossimException &e)
   {
      CFATAL<<"Exception: "<<e.what()<<endl;
      *m_outputStream<<"{ \"ERROR\": \"" << e.what() << "\" }\n"<<endl;
   }

   // close any open file streams:
   ifstream* si = dynamic_cast<ifstream*>(m_inputStream);
   if (si)
   {
      si->close();
      delete si;
   }
   ofstream* so = dynamic_cast<ofstream*>(m_outputStream);
   if (so)
   {
      so->close();
      delete so;
   }

   return true;
}

void ossimAtpTool::getKwlTemplate(ossimKeywordlist& kwl)
{
}

void ossimAtpTool::getAlgorithms()
{
   m_responseJSON.clear();

   Json::Value algoList;
   algoList[0]["name"]        = "crosscorr";
   algoList[0]["description"] = "Matching by cross-correlation of raster patches";
   algoList[0]["label"]       = "Cross Correlation";
   algoList[1]["name"]        = "descriptor";
   algoList[1]["description"] = "Matching by feature descriptors";
   algoList[1]["label"]       = "Descriptor";
   //algoList[2]["name"]        = "nasa";
   //algoList[2]["description"] = "Tiepoint extraction using NASA tool.";
   //algoList[2]["label"]       = "NASA";

   m_responseJSON["algorithms"] = algoList;
}

void ossimAtpTool::getParameters()
{
   m_responseJSON.clear();

   Json::Value params;
   AtpConfig::instance().saveJSON(params);

   m_responseJSON["parameters"] = params;

}

void ossimAtpTool::generate()
{
   ostringstream xmsg;
   xmsg<<"ossimAtpTool::generate()  ";

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

void ossimAtpTool::doPairwiseMatching()
{
   static const char* MODULE = "ossimAtpTool::doPairwiseMatching()  ";
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

   if (AtpConfig::instance().diagnosticLevel(1))
      clog<<"\n"<<MODULE<<"Total TPs found = "<<m_photoBlock->getTiePointList().size()<<endl;
}


}


