//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include <ossimAtpTool.h>
#include <ossim/base/ossimArgumentParser.h>
#include <ossim/base/ossimApplicationUsage.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimException.h>
#include <AutoTiePointService.h>
#include <json/json.h>
#include <iostream>
#include <memory>
#include <sstream>

using namespace std;

namespace ATP
{
const char* ossimAtpTool::DESCRIPTION =
      "Provides automatic tiepoint generation functionality .";

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
   au->addCommandLineOption("-i <filename>",
         "Reads request JSON from the input file specified instead of stdin.");
   au->addCommandLineOption("-o <filename>",
         "Outputs response JSON to the output file instead of stdout.");
   au->addCommandLineOption("-v",
         "Verbose. All non-response (debug) output to stdout is enabled.");
}

bool ossimAtpTool::initialize(ossimArgumentParser& ap)
{
   string ts1;
   ossimArgumentParser::ossimParameter sp1(ts1);

   if (!ossimTool::initialize(ap))
      return false;
   if (m_helpRequested)
      return true;

   if ( ap.read("-v"))
      m_verbose = true;

   if ( ap.read("-i", sp1))
   {
      ifstream* s = new ifstream (ts1);
      if (s->fail())
      {
         ossimNotify(ossimNotifyLevel_FATAL)<<__FILE__<<" Could not open input file <"<<ts1<<">";
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
         ossimNotify(ossimNotifyLevel_FATAL)<<__FILE__<<" Could not open output file <"<<ts1<<">";
         return false;
      }
      m_outputStream = s;
   }
   else
      m_outputStream = &clog;

   return true;
}

void ossimAtpTool::initialize(const std::string& json_query)
{
   m_jsonRequest = json_query;
}

bool ossimAtpTool::execute()
{
   ostringstream xmsg;

   // Read input:
   if (m_jsonRequest.empty())
   {
      stringstream json_query;
      streambuf *rqBuf = json_query.rdbuf();
      (*m_inputStream) >> rqBuf;
      m_jsonRequest = json_query.str();
   }
   try
   {
      Json::Value queryRoot;
      Json::Reader reader;
      bool parsingSuccessful = reader.parse( m_jsonRequest, queryRoot );
      if ( !parsingSuccessful )
      {
         xmsg<<"Failed to parse JSON query string. \n"
               << reader.getFormattedErrorMessages()<<endl;
         throw ossimException(xmsg.str());
      }

      m_service.reset(new AutoTiePointService);

      // Call actual service and get response:
      m_service->loadJSON(queryRoot);
      m_service->execute();
      Json::Value response;
      m_service->saveJSON(response);

      // Serialize JSON object for return:
      Json::StyledWriter writer;
      string json_response = writer.write(response);
      (*m_outputStream) << json_response;
   }
   catch(ossimException &e)
   {
      cerr<<"Exception: "<<e.what()<<endl;
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

}


