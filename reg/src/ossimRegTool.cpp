//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include <ossim/base/ossimArgumentParser.h>
#include <ossim/base/ossimApplicationUsage.h>
#include <ossim/base/ossimException.h>
#include "ossimRegTool.h"
#include <sstream>

using namespace std;

const char* ossimRegTool::DESCRIPTION =
      "Performs registration given list of images.";

ossimRegTool::ossimRegTool()
{
}

ossimRegTool::~ossimRegTool()
{
}

void ossimRegTool::setUsage(ossimArgumentParser& ap)
{
   // Add global usage options. Don't include ossimChipProcUtil options as not appropriate.
   ossimTool::setUsage(ap);

   // Set the general usage:
   ossimApplicationUsage* au = ap.getApplicationUsage();
   ossimString usageString = ap.getApplicationName();
   usageString += " registration [options] <input-raster-file> <input-raster-file> [...]";
   au->setCommandLineUsage(usageString);

   // Set the command line options:
   au->addCommandLineOption("--myoption <float>", "Blah blah");
}

bool ossimRegTool::initialize(ossimArgumentParser& ap)
{
   string ts1;
   ossimArgumentParser::ossimParameter sp1(ts1);

   if (!ossimTool::initialize(ap))
      return false;

   if ( ap.read("--myoption", sp1))
      m_kwl.addPair("myoption", ts1);

   processRemainingArgs(ap);
   return true;
}

void ossimRegTool::initialize(const ossimKeywordlist& kwl)
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

   value = m_kwl.findKey("myoption");
   if (!value.empty())
      double myoption = value.toDouble();

   ossimChipProcTool::initialize(kwl);
}

bool ossimRegTool::execute()
{
   ostringstream xmsg;

   // Establish intersections

   // Find tiepoints

   // Register images

   // Report results

   return true;
}


void ossimRegTool::getKwlTemplate(ossimKeywordlist& kwl)
{
   ostringstream value;
   value<<"<float> (optional, defaults to <ADD DEFAULT>)";
   kwl.addPair("myoption", value.str());

}


