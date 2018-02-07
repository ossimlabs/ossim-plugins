//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "ossimDemTool.h"
#include "ossimDemToolConfig.h"
#include <math.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimGpt.h>
#include <ossim/base/ossimEcefPoint.h>
#include <ossim/base/ossimArgumentParser.h>
#include <ossim/base/ossimApplicationUsage.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimException.h>
#include <ossim/util/ossimToolRegistry.h>
#include <ossim/projection/ossimRpcSolver.h>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>

using namespace std;
using namespace ossim;

#define CINFO  ossimNotify(ossimNotifyLevel_INFO)
#define CWARN  ossimNotify(ossimNotifyLevel_WARN)
#define CFATAL ossimNotify(ossimNotifyLevel_FATAL)

const char* ossimDemTool::DESCRIPTION =
      "Provides DEM generation functionality. This tool uses JSON format to "
      "communicate requests and results.";

ossimDemTool::ossimDemTool()
: m_outputStream (0),
  m_verbose (false),
  m_algorithm (ALGO_UNASSIGNED),
  m_method (METHOD_UNASSIGNED)
{
}

ossimDemTool::~ossimDemTool()
{
}

void ossimDemTool::setUsage(ossimArgumentParser& ap)
{
   // Add global usage options. Don't include ossimChipProcUtil options as not appropriate.
   ossimTool::setUsage(ap);

   // Set the general usage:
   ossimApplicationUsage* au = ap.getApplicationUsage();
   ossimString usageString = ap.getApplicationName();
   usageString += " dem [options] \n\n";
   usageString +=
         "Accesses DEM generation functionality given JSON request on stdin (or input file if\n"
         "-i specified). The response JSON is output to stdout unless -o option is specified.\n";
   au->setCommandLineUsage(usageString);

   // Set the command line options:
   au->addCommandLineOption("--algorithms",
         "List available DEM generation algorithms");
   au->addCommandLineOption("-i <filename>",
         "Reads request JSON from the input file specified instead of stdin.");
   au->addCommandLineOption("-o <filename>",
         "Outputs response JSON to the output file instead of stdout.");
   au->addCommandLineOption("--parameters",
         "List all algorithm parameters with default values.");
   au->addCommandLineOption("-v",
         "Verbose. All non-response (debug) output to stdout is enabled.");
}

bool ossimDemTool::initialize(ossimArgumentParser& ap)
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
      ifstream s (ts1);
      if (s.fail())
      {
         CFATAL<<__FILE__<<" Could not open input file <"<<ts1<<">";
         return false;
      }
      try
      {
         Json::Value queryJson;
         s >> queryJson;
         loadJSON(queryJson);
      }
      catch (exception& e)
      {
         ossimNotify(ossimNotifyLevel_FATAL)<<__FILE__<<" Could not parse input JSON <"<<ts1<<">";
         return false;
      }
   }

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

void ossimDemTool::loadJSON(const Json::Value& queryRoot)
{
   ostringstream xmsg;
   xmsg<<"ossimDemTool::loadJSON()  ";

   // Fetch the desired method from the JSON if provided, otherwise rely on command line options:
   m_method = GENERATE;
   string method = queryRoot["method"].asString();
   if (method == "getAlgorithms")
      m_method = GET_ALGO_LIST;
   else if (method == "getParameters")
      m_method = GET_PARAMS;

   m_outputDemFile = queryRoot["filename"].asString();

   // Fetch the desired algorithm or configuration:
   JsonConfig& config = ossimDemToolConfig::instance();
   string algorithm = queryRoot["algorithm"].asString();
   if (algorithm.empty())
      algorithm = "asp"; // default for now

   // Assign enum data member used throughout the service:
   if (algorithm == "asp")
      m_algorithm = ASP;
   else if (algorithm == "omg")
      m_algorithm = OMG;
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

bool ossimDemTool::execute()
{
   ostringstream xmsg;

   try
   {
      switch (m_method)
      {
      case GET_ALGO_LIST:
         getAlgorithms();
         break;
      case GET_PARAMS:
         getParameters();
         break;
         case GENERATE:
         // Establish server-side output filename:
         if (m_algorithm == ASP)
            doASP();
         if (m_algorithm == OMG)
            doOMG();
         else
         {
            m_responseJSON["status"] = "failed";
            m_responseJSON["report"] = "Unsupported algorthm requested for DEM generation.";
         }
         break;
      default:
         xmsg << "Fatal: No method selected prior to execute being called. I don't know what to do!";
         throw ossimException(xmsg.str());
      }

      // Serialize JSON object for return:
      if (m_outputStream)
         (*m_outputStream) << m_responseJSON;
   }
   catch(ossimException &e)
   {
      CFATAL<<"Exception: "<<e.what()<<endl;
      if (m_outputStream)
         *m_outputStream<<"{ \"ERROR\": \"" << e.what() << "\" }\n"<<endl;
   }

   // close any open file streams:
   ofstream* so = dynamic_cast<ofstream*>(m_outputStream);
   if (so)
   {
      so->close();
      delete so;
   }

   return true;
}

void ossimDemTool::getKwlTemplate(ossimKeywordlist& kwl)
{
}

void ossimDemTool::getAlgorithms()
{
   m_responseJSON.clear();

   Json::Value algoList;
   algoList[0]["name"]        = "asp";
   algoList[0]["description"] = "NASA Ames Stereo Pipeline";
   algoList[0]["name"]        = "omg";
   algoList[0]["description"] = "OSSIM/MSP Generator";

   m_responseJSON["algorithms"] = algoList;
}

void ossimDemTool::getParameters()
{
   m_responseJSON.clear();
   Json::Value params;
   ossimDemToolConfig::instance().saveJSON(params);
   m_responseJSON["parameters"] = params;
}

void ossimDemTool::doASP()
{
   static const char* MODULE = "ossimDemTool::doASP()  ";
   ostringstream xmsg;
   xmsg<<MODULE;

   if (!m_photoBlock || (m_photoBlock->getImageList().size() < 2))
   {
      xmsg << "No photoblock has been declared or it has less than two images. Cannot perform ATP.";
      throw ossimException(xmsg.str());
   }

   // Start the ASP command ine:
   ostringstream cmd;
   cmd<<"stereo";

   // First obtain list of images from photoblock:
   std::vector<shared_ptr<Image> >& imageList = m_photoBlock->getImageList();
   int numImages = (int) imageList.size();
   int numPairs = (numImages * (numImages-1))/2; // triangular number assumes all overlap
   for (int i=0; i<(numImages-1); i++)
   {
      // Establish existence of RPB, and create it if not available:
      shared_ptr<Image> imageA = imageList[i];
      ossimFilename imageFile(imageA->getFilename());
      ossimFilename rpcFile(imageFile);
      rpcFile.setExtension("RPB");
      if (!rpcFile.isReadable()) {
         ossimRpcSolver rpcSolver;
         rpcSolver.solve(imageFile);
      }
      cmd<<" "<<imageFile;
   }

   // Establish output DEM name:
   if (m_outputDemFile.empty())
   {
      m_outputDemFile = "asp-dem-";
      m_outputDemFile.appendTimestamp();
      m_outputDemFile.setExtension("tif");
   }

   cmd<<" "<<m_outputDemFile<<ends;
}

void ossimDemTool::doOMG()
{
   static const char* MODULE = "ossimDemTool::doOMG()  ";
   ostringstream xmsg;
   xmsg<<MODULE;

   if (!m_photoBlock || (m_photoBlock->getImageList().size() < 2))
   {
      xmsg << "No photoblock has been declared or it has less than two images. Cannot perform ATP.";
      throw ossimException(xmsg.str());
   }

   // OMG uses the ATP plugin:
   ossimRefPtr<ossimTool> tool = ossimToolRegistry::instance()->createTool("ossimAtpTool") ;

   // Create JSON to perform needed ATP:

   // Use dense points to do intersections to generate point cloud

   // Convert point cloud to DEM
}



