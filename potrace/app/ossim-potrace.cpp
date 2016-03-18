//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include <ossim/init/ossimInit.h>
#include <ossim/base/ossimStringProperty.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/util/ossimUtilityRegistry.h>
#include <assert.h>
#include "../src/ossimPotraceUtil.h"

#define TEST_READER false

int usage(char* app_name)
{
   cout << "\nUtility app to convert a raster image to geojson vector list. Any non-null pixel in "
         "the raster is treated as an \"on\" pixel for converting to bitmap before performing "
         "trace."<<endl;
   cout << "\nUsage: "<<app_name<<" <filename> \n" << endl;
   return 1;
}

int main(int argc, char** argv)
{
   ossimInit::instance()->initialize(argc, argv);

   if ((argc < 2)  || (ossimString(argv[1]).contains("--help")))
      return usage(argv[0]);

   ossimFilename fname = argv[1];

   ossimRefPtr<ossimUtility> util = ossimUtilityRegistry::instance()->createUtility("potrace");

   if (!util.valid())
   {
      cout << "Bad pointer returned from utility factory." << endl;
      return 1;
   }
   ossimKeywordlist kwl;
   kwl.add(ossimKeywordNames::IMAGE_FILE_KW, fname.chars());
   util->initialize(kwl);
   util->execute();

   return 0;
}


