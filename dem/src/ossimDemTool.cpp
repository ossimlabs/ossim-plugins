//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "ossimDemTool.h"
#include "ossimDemToolConfig.h"
#include <ossim/base/ossimApplicationUsage.h>
#include <ossim/util/ossimToolRegistry.h>
#include <ossim/projection/ossimRpcSolver.h>
#include <ossim/base/ossimPreferences.h>
#include <ossim/point_cloud/ossimGenericPointCloudHandler.h>
#include <ossim/point_cloud/ossimPointCloudImageHandler.h>
#include <ossim/imaging/ossimImageFileWriter.h>
#include <ossim/imaging/ossimImageWriterFactoryRegistry.h>

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
  m_method (METHOD_UNASSIGNED),
  m_postSpacing (0),
  m_postSpacingUnits (UNITS_UNASSIGNED)
{
   // Read the default DEM extraction parameters:
   Json::Value configJson;
   ossimFilename configFilename ("omgConfig.json");
   ossimFilename shareDir = ossimPreferences::instance()->
           preferencesKWL().findKey( std::string( "ossim_share_directory" ) );
   shareDir += "/atp"; // TODO: Consolidate tool configs
   if (!shareDir.isDir())
      throw ossimException("Nonexistent share drive provided for config files.");

   configFilename.setPath(shareDir);
   ifstream configJsonStream (configFilename.string());
   if (configJsonStream.fail())
   {
      ossimNotify(ossimNotifyLevel_WARN) << __FILE__ << " Bad file open or parse of config file <"
                                         << configFilename << ">. Ignoring." << endl;
   }
   else
   {
      configJsonStream >> configJson;
      loadJSON(configJson);
   }
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

   if (queryRoot.isMember("postSpacing"))
      m_postSpacing = queryRoot["postSpacing"].asDouble();


   // If parameters were provided in the JSON payload, have the config override the defaults:
   m_parameters = queryRoot["parameters"];
   m_atpParameters = queryRoot["atpParameters"];
   //if (!m_parameters.isNull())
   //   config.loadJSON(m_parameters);

   //if (config.diagnosticLevel(2))
   //   clog<<"\nDEM configuration after loading:\n"<<config<<endl;

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
   string cmdPath (getenv("NGTASP_BIN_DIR"));
   cmd<<cmdPath<<"/stereo";

   // First obtain list of images from photoblock:
   std::vector<shared_ptr<Image> >& imageList = m_photoBlock->getImageList();
   std::vector<ossimFilename> rpcFilenameList;
   int numImages = (int) imageList.size();
   int numPairs = (numImages * (numImages-1))/2; // triangular number assumes all overlap
   for (int i=0; i<numImages; i++)
   {
      // Establish existence of RPB, and create it if not available:
      shared_ptr<Image> image = imageList[i];
      ossimFilename imageFilename (image->getFilename());
      ossimFilename rpcFilename(imageFilename);
      rpcFilename.setExtension("RPB");
      if (!rpcFilename.isReadable())
      {
         ossimRpcSolver rpcSolver;
         if (!rpcSolver.solve(imageFilename))
         {
            xmsg << "Error encountered in solving for RPC coefficients..";
            throw ossimException(xmsg.str());
         }

         ossimRefPtr<ossimRpcModel> rpcModel = rpcSolver.getRpcModel();
         ofstream rpbFile (rpcFilename.string());
         if (rpbFile.fail() || !rpcModel->toRPB(rpbFile))
         {
            xmsg << "Error encountered writing RPC to file <"<<rpcFilename<<">.";
            throw ossimException(xmsg.str());
         }
      }
      cout<<"Generated RPC for "<<imageFilename<<endl;
      rpcFilenameList.emplace_back(rpcFilename);
      cmd << " " <<imageFilename;
   }

   // Loop to add model filenames to the command line:
   for (const auto &rpcFilename : rpcFilenameList)
      cmd << " " <<rpcFilename;

   // Establish output DEM directory name:
   if (m_outputDemFile.empty())
   {
      m_outputDemFile = "asp-result";
      m_outputDemFile.appendTimestamp();
   }

   cmd<<" "<<m_outputDemFile<<ends;
   cout << "\nSpawning command: "<<cmd.str()<<endl;
   if (system(cmd.str().c_str()))
   {
      xmsg << "Error encountered running DEM generation command.";
      throw ossimException(xmsg.str());
   }
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
   ossimRefPtr<ossimTool> atpTool = ossimToolRegistry::instance()->createTool("ossimAtpTool") ;
   if (!atpTool)
   {
      xmsg << "OMG algorithm requires the ATP plugin but plugin not found in registry.";
      throw ossimException(xmsg.str());
   }

   // Pass along any user parameter modifications to the ATP tool:
   Json::Value atpJson;
   atpJson["algorithm"] = "crosscorr";
   atpJson["method"] = "generate";
   m_photoBlock->saveJSON(atpJson["photoblock"]);
   atpJson["parameters"] = m_atpParameters;
   atpTool->loadJSON(atpJson);

   // Generate dense tiepoint field:
   atpTool->execute();
   Json::Value omgJson;
   atpTool->saveJSON(omgJson);

   // Use dense points to do intersections to generate point cloud
   m_photoBlock->loadJSON(omgJson);
   int num_tiepoints = m_photoBlock->getTiePointList().size();

   // Get instance of MSP tool for performing n-way intersection:
   ossimRefPtr<ossimTool> mspTool = ossimToolRegistry::instance()->createTool("ossimMspTool") ;
   if (!mspTool)
   {
      xmsg << "OMG algorithm requires the MSP plugin but plugin not found in registry.";
      throw ossimException(xmsg.str());
   }

   // Prepare the JSON used to communicate with the mensuration tool and kick off bulk mensuration:
   omgJson["service"] = "mensuration";
   omgJson["method"] = "pointExtraction";
   omgJson["outputCoordinateSystem"] = "ecf";
   mspTool->loadJSON(omgJson);
   mspTool->execute();
   Json::Value mspJson;
   mspTool->saveJSON(mspJson);

   // Save resulting geo point to point cloud:
   const Json::Value& observations = mspJson["mensurationReport"];
   vector<ossimEcefPoint> pointCloud;
   for (const auto &observation : observations)
   {
      ossimEcefPoint ecfPt(observation["x"].asDouble(),
                           observation["y"].asDouble(),
                           observation["z"].asDouble());
      pointCloud.emplace_back(ecfPt);
   }

   // Convert point cloud to DEM
   ossimRefPtr<ossimGenericPointCloudHandler> pch = new ossimGenericPointCloudHandler(pointCloud);
   ossimRefPtr<ossimPointCloudImageHandler> pcih = new ossimPointCloudImageHandler;
   pcih->setPointCloudHandler(pch.get());
   ossimImageSource* lastSource = pcih.get();

   // TODO: Need to set up output projection to match desired GSD and units

   // TODO: Need smart interpolation resampling to fill in the nulls
   // lastSource = resampler...

   // Set up the writer and output the DEM:
   ossimRefPtr<ossimImageFileWriter> writer =
           ossimImageWriterFactoryRegistry::instance()->createWriter(m_outputDemFile);
   if (!writer)
   {
      xmsg << "Error encountered creating writer object given filename <"<<m_outputDemFile<<">.";
      throw ossimException(xmsg.str());
   }
   writer->connectMyInputTo(0, lastSource);
   if (!writer->execute())
   {
      xmsg << "Error encountered writing DEM file.";
      throw ossimException(xmsg.str());
   }

   // Need to populate response JSON with product filepath and statistics
}



