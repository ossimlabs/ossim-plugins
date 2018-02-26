//---
//
// License: MIT
//
// File: ossim-foo.cpp
//
// Description: Contains application definition "ossim-foo" app.
//
// NOTE:  This is supplied for simple quick test. DO NOT checkin your test to
//        the svn repository.  Simply edit ossim-foo.cpp and run your test.
//        After completion you can do a "git checkout -- ossimfoo.cpp" if
//        you want to keep your working repository up to snuff.
//
// $Id$
//---

// System includes:
#include <cmath>
#include <memory>
#include <sstream>
#include <iostream>

#include <ossim/base/ossimApplicationUsage.h>
#include <ossim/base/ossimArgumentParser.h>
#include <ossim/init/ossimInit.h>
#include <ossim/reg/PhotoBlock.h>
#include <ossim/projection/ossimSensorModelTuple.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>

using namespace std;
using namespace ossim;

// Summarizes results of ATP given ATP output JSON
int main(int argc, char *argv[])
{
   int returnCode = 0;

   ossimArgumentParser ap(&argc, argv);
   ossimInit::instance()->addOptions(ap);
   ossimInit::instance()->initialize(ap);

   if (argc < 2)
   {
      cout<<"\nUsage: "<<argv[0]<<" <atp-results.json>\n"<<endl;
      exit(0);
   }

   ossimFilename root_name (argv[1]);
   ifstream jsonInput (root_name.string());
   if (jsonInput.fail())
   {
      cout<<"\nError opening JSON file <"<<argv[1]<<">\n"<<endl;
      exit(1);
   }

   Json::Value root;
   jsonInput >> root;

   root_name = root_name.noExtension();

   const Json::Value& pbJson = root["photoblock"];
   PhotoBlock photoblock (pbJson);

   vector<shared_ptr<Image> >& imageList = photoblock.getImageList();
   vector<shared_ptr<TiePoint> >& tpList = photoblock.getTiePointList();

   if (tpList.empty())
   {
      cout << "\n  " << root_name << " -- No tiepoints found." << endl;
      return 0;
   }

   ossimRefPtr<ossimImageHandler> handler;
   auto factory = ossimImageHandlerRegistry::instance();
   map<string, ossimRefPtr<ossimImageGeometry> > geomList;

   for (auto &image : imageList)
   {
      handler = factory->open(image->getFilename());
      if (!handler)
      {
         cout<<"\nError loading image file <"<<image->getFilename()<<">\n"<<endl;
         exit(1);
      }
      geomList.emplace(image->getImageId(), handler->getImageGeometry());
   }

   ossimDpt ipA, ipB;
   ossimGpt gpA, gpB;
   vector<double> distances;
   double distance = 0;
   double accum = 0;
   ossimRefPtr<ossimImageGeometry> geomA, geomB;
   string imageID;
   NEWMAT::SymmetricMatrix cov;

   for (auto &tp : tpList)
   {
      tp->getImagePoint(0, imageID, ipA, cov);
      geomA = geomList.find(imageID)->second;

      tp->getImagePoint(1, imageID, ipB, cov);
      geomB = geomList.find(imageID)->second;

      if (!geomA || !geomB)
      {
         cout<<"\nError: null image geometry encountered.\n"<<endl;
         exit(1);
      }
      geomA->localToWorld(ipA, gpA);
      geomB->localToWorld(ipB, gpB);

      distance = gpA.distanceTo(gpB);
      distances.emplace_back(distance);
      accum += distance;
   }

   double mean = accum / tpList.size();
   double e = 0;
   for (double &distance : distances)
      e += (distance-mean)*(distance-mean);
   double sigma = sqrt(e/distances.size());

   cout<<"\n  "<<root_name<<"  mean residual: "<<mean<<" +- "<<sigma<<" m"<<endl;
   return 0;
}

