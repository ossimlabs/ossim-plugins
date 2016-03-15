//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include <ossim/init/ossimInit.h>
#include <ossim/base/ossimStringProperty.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/util/ossimUtilityRegistry.h>
#include <assert.h>
#include "ossimPotraceUtil.h"

#define TEST_READER false

using namespace pdal;

int usage(char* app_name)
{
   cout << "\nUsage: "<<app_name<<" < [filename]\n" << endl;
   return 1;
}

int main(int argc, char** argv)
{
   if (argc == 1)
      return usage(argv[0]);

   ossimFilename fname;
   if (argc > 2)
      fname = argv[1];

   ossimRefPtr<ossimUtility> util = ossimUtilityRegistry::instance()->createUtility("potrace");

   if (!util.valid())
   {
      cout << "Bad pointer returned from utility factory." << endl;
      return 1;
   }

   util->execute();

   return 0;
}


