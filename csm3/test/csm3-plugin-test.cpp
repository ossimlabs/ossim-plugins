//**************************************************************************************************
//
// OSSIM (http://trac.osgeo.org/ossim/)
//
// License:  LGPL -- See LICENSE.txt file in the top level directory for more details.
//
//**************************************************************************************************
// $Id: plugin-test.cpp 23401 2015-06-25 15:00:31Z okramer $
#include "../src/ossimCsm3ProjectionFactory.h"
#include "../src/ossimCsm3Loader.h"
#include <ossim/base/ossimPreferences.h>
#include <ossim/base/ossimGpt.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/projection/ossimProjection.h>
#include <vector>
#include <string>
#include <iostream>

using namespace std;

// USAGE: csm3-plugin-test [<image_file>]
// optional <image_file> = filename of image to test instantiation of sensor model.

int main(int argc, char** argv)
{
   ossimFilename image_file;
   if (argc > 1)
      image_file = argv[1];

   ossimPreferences::instance()->loadPreferences();
   ossimCsm3Loader loader;
   vector<string> plugins;
   loader.getAvailablePluginNames(plugins);
   cout<<"\nFound "<<plugins.size()<<" plugins:"<<endl;
   for (int i=0; i<plugins.size(); i++)
      cout << "  "<<plugins[i]<<endl;
   cout<<endl;

   if (!image_file.empty())
   {
      ossimProjection* proj =
            ossimCsm3ProjectionFactory::instance()->createProjection(image_file, 0);
      if (!proj)
      {
         cout<<"Failed creating projection from file <"<<image_file<<">.\n"<<endl;
         return 1;
      }

      ossimGpt origGpt;
      ossimDpt imagePt1 (1, 1);
      ossimDpt imagePt2;
      proj->lineSampleHeightToWorld(imagePt1, 0, origGpt);
      proj->worldToLineSample(origGpt, imagePt2);
      ossimDpt ptDiff = imagePt2 - imagePt1;
      cout<<"Ground point: "<<origGpt<<endl;
      cout<<"Roundtrip error: "<<ptDiff<<endl;
      cout<<endl;
   }
   return 0;
}

