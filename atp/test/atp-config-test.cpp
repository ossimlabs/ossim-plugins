//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#include "../src/AtpConfig.h"
#include "../src/correlation/CorrelationAtpGenerator.h"
#include "../src/TiePoint.h"
#include "../src/Image.h"
#include <iostream>
#include <ossim/init/ossimInit.h>
#include <ossim/base/ossimKeywordlist.h>
#include <memory>

using namespace std;
using namespace ATP;

/**************************************************************************************************
 *
 * Test of AtpConfig subsystem
 *
 **************************************************************************************************/

int main(int argc, char** argv)
{
   clog << "ATP Config Test" << endl;
   ossimInit::instance()->initialize(argc, argv);

   // Initialize correlation parameters:
   AtpConfig& atpConfig = AtpConfig::instance();
   clog << "\nDump of default common ATP params:"<<endl;
   clog << atpConfig << endl;

   if (!atpConfig.readConfig("crosscorr"))
   {
      clog <<"\nError reading testConfig!"<<endl;
      return 1;
   }
   clog << "\n******************************************************************"<<endl;
   clog << "\nDump of crosscorr ATP params:"<<endl;
   clog << atpConfig << endl;

   if (!atpConfig.readConfig("testConfig"))
   {
      clog <<"\nError reading testConfig!"<<endl;
      return 1;
   }
   clog << "\n******************************************************************"<<endl;
   clog << "\nDump of testConfig ATP params:"<<endl;
   clog << atpConfig << endl;

   Json::Value shortForm;
   shortForm["myCustomParam"] = false;
   shortForm["peakThreshold"] = 0.8;
   shortForm["algorithm"] = "override-test";
   shortForm["newParam"] = "oops";
   shortForm["diagnosticLevel"] = 3;
   atpConfig.loadJSON(shortForm);
   clog << "\n******************************************************************"<<endl;
   clog << "\nDump of short-form ATP params override:"<<endl;
   clog << atpConfig << endl;

   if (atpConfig.diagnosticLevel(2) && !atpConfig.diagnosticLevel(4))
      clog<<"\nCorrectly processed diagnostic level."<<endl;
   else
      clog<<"\nDiagnostic level test failed."<<endl;

   return 0;
}
