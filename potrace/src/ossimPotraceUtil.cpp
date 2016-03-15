//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "ossimPotraceUtil.h"
#include <ossim/base/ossimApplicationUsage.h>
#include <sstream>

using namespace std;

const char* ossimPotraceUtil::DESCRIPTION =
      "Computes vector representation of input image.";

static const string TOLERANCE_KW = "tolerance";


ossimPotraceUtil::ossimPotraceUtil()
:  m_tolerance(0)
{
}

ossimPotraceUtil::~ossimPotraceUtil()
{
}

void ossimPotraceUtil::setUsage(ossimArgumentParser& ap)
{
   // Add global usage options.
   ossimUtility::setUsage(ap);

   // Set the general usage:
   ossimApplicationUsage* au = ap.getApplicationUsage();
   ossimString usageString = ap.getApplicationName();
   usageString += " [options] <output-vector-file>";
   au->setCommandLineUsage(usageString);

   // Set the command line options:
   au->addCommandLineOption("--tolerance <float>",
         "tolerance +- deviation from threshold for marginal classifications. Defaults to 0.05.");
}

bool ossimPotraceUtil::initialize(ossimArgumentParser& ap)
{
   string ts1;
   ossimArgumentParser::ossimParameter sp1(ts1);
   string ts2;
   ossimArgumentParser::ossimParameter sp2(ts2);
   string ts3;
   ossimArgumentParser::ossimParameter sp3(ts3);

   if ( ap.read("--tolerance", sp1))
      m_kwl.addPair(TOLERANCE_KW, ts1);

   if (!ossimUtility::initialize(ap))
      return false;
   return true;
}

void ossimPotraceUtil::initialize(const ossimKeywordlist& kwl)
{
   ossimString value;
   ostringstream xmsg;

   // Don't copy KWL if member KWL passed in:
   if (&kwl != &m_kwl)
   {
      // Start with clean options keyword list.
      m_kwl.clear();
      m_kwl.addList( kwl, true );
   }

   value = m_kwl.findKey(TOLERANCE_KW);
   if (!value.empty())
      m_tolerance = value.toDouble();

   ossimUtility::initialize(kwl);
}

bool ossimPotraceUtil::execute()
{
   cout << "\n EXECUTING Potrace Utility!"<<endl;
   return true;
}

void ossimPotraceUtil::getKwlTemplate(ossimKeywordlist& kwl)
{
   ossimUtility::getKwlTemplate(kwl);
   kwl.add(TOLERANCE_KW.c_str(), "TBR");
}

