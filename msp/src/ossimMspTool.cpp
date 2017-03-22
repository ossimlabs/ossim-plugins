//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include <ossim/base/ossimArgumentParser.h>
#include <ossim/base/ossimApplicationUsage.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimException.h>
#include "ossimMspTool.h"
#include <sstream>

using namespace std;

const char* ossimMspTool::DESCRIPTION =
      "Provides access to MSP functionality.";

ossimMspTool::ossimMspTool()
{
}

ossimMspTool::~ossimMspTool()
{
}

void ossimMspTool::setUsage(ossimArgumentParser& ap)
{
   // Add global usage options. Don't include ossimChipProcUtil options as not appropriate.
   ossimTool::setUsage(ap);

   // Set the general usage:
   ossimApplicationUsage* au = ap.getApplicationUsage();
   ossimString usageString = ap.getApplicationName();
   usageString += " potrace [options] <input-raster-file> [<output-vector-file>]";
   au->setCommandLineUsage(usageString);

   // Set the command line options:
   au->addCommandLineOption("--alphamax <float>",
         "set the corner threshold parameter. The default value is 1. The smaller this value, the "
         "more sharp corners will be produced. If this parameter is 0, then no smoothing will be "
         "performed and the output is a polygon. If this parameter is greater than 1.3, then all "
         "corners are suppressed and the output is completely smooth");
}

bool ossimMspTool::initialize(ossimArgumentParser& ap)
{
   string ts1;
   ossimArgumentParser::ossimParameter sp1(ts1);

   if (!ossimTool::initialize(ap))
      return false;

   if ( ap.read("--alphamax", sp1))
      //m_kwl.addPair(ALPHAMAX_KW, ts1);

   return true;
}

void ossimMspTool::initialize(const ossimKeywordlist& kwl)
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

  // value = m_kwl.findKey(ALPHAMAX_KW);
  // if (!value.empty())
      //m_alphamax = value.toDouble();


   ossimTool::initialize(kwl);
}

bool ossimMspTool::execute()
{
   ostringstream xmsg;

   return true;
}

void ossimMspTool::getKwlTemplate(ossimKeywordlist& kwl)
{
}



