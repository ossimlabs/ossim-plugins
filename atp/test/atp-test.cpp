//---
//
// License: MIT
//
/// System includes:
#include <cmath>
#include <memory>
#include <sstream>
#include <iostream>

// ossim includes:  These are here just to save time/typing...
#include <ossim/base/ossimApplicationUsage.h>
#include <ossim/base/ossimArgumentParser.h>
#include <ossim/init/ossimInit.h>
#include <ossim/reg/PhotoBlock.h>
#include <ossim/projection/ossimSensorModelTuple.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include "../AtpCommon.h"

using namespace std;
using namespace ossim;

int main(int argc, char *argv[])
{
   int returnCode = 0;

   ossimArgumentParser ap(&argc, argv);
   ossimInit::instance()->addOptions(ap);
   ossimInit::instance()->initialize(ap);

   if (argc < 2)
   {
      cout<<"\nUsage: "<<argv[0]<<"<mensuration-input.json>\n"<<endl;
      exit(0);
   }

   try
   {
      ifstream jsonInput (argv[1]);
      Json::Value root;
      jsonInput >> root;

      const Json::Value& pbJson = root["photoblock"];
      PhotoBlock photoblock (pbJson);

      vector<shared_ptr<Image> >& imageList = photoblock.getImageList();
      vector<shared_ptr<TiePoint> >& tpList = photoblock.getTiePointList();

      ossimProjectionFactoryRegistry* factory = ossimProjectionFactoryRegistry::instance();
      ossimSensorModelTuple smt;
      ossimRefPtr<ossimSensorModel> model;
      NEWMAT::SymmetricMatrix  cov;
      string imageId;
      int numImages = imageList.size();

      // Initialize tuple with image sensor models:
      for (int i=0; i<numImages; i++)
      {
         ossimFilename imageFile (imageList[i]->getFilename());
         unsigned int index = imageList[i]->getEntryIndex();
         ossimProjection* proj = factory->createProjection(imageFile, 0);
         model = dynamic_cast<ossimSensorModel*>(proj);
         if (!model)
         {
            delete proj;
            continue;
         }
         smt.addImage(model.get());
      }

      vector<ossimDpt> observations;
      ossimDpt imagePoint;
      ossimSensorModelTuple::IntersectStatus status = ossimSensorModelTuple::OP_FAIL;
      ossimEcefPoint resultEcf;
      ossimGpt resultGpt;
      NEWMAT::Matrix  cov3d;

      // Now perform intersection for each TP entry:
      for (unsigned int n=0; n<tpList.size(); n++)
      {
         for (unsigned int i=0; i<numImages; i++)
         {
            tpList[n]->getImagePoint(i, imageId, imagePoint, cov);
            observations.push_back(imagePoint);
         }

         status = smt.intersect(observations, resultEcf, cov3d);
         resultGpt = ossimGpt(resultEcf);
         cout <<"\n intersection GPT: "<<resultGpt<<"\n"<<cov3d<<endl;
      }
      cout<<"\nDone!"<<endl;
   }
   catch(const ossimException& e)
   {
      CWARN << e.what() << std::endl;
      returnCode = 1;
   }
   catch( ... )
   {
      CWARN << "ossim-foo caught unhandled exception!" << std::endl;
      returnCode = 1;
   }

   return returnCode;
}
