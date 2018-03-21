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
#include "../AtpCommon.h"
#include "../src/AtpGenerator.h"

using namespace std;
using namespace ossim;
using namespace ATP;

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
      jsonInput.close();

      AtpConfig& config = AtpConfig::instance();
      const Json::Value& parameters = root["parameters"];
      config.loadJSON(parameters);

      const Json::Value& pbJson = root["photoblock"];
      PhotoBlock photoblock (pbJson);

      vector<shared_ptr<Image> >& imageList = photoblock.getImageList();
      vector<shared_ptr<TiePoint> >& tpList = photoblock.getTiePointList();

      shared_ptr<AtpGenerator> generator (new AtpGenerator(AtpGenerator::CROSSCORR));
      generator->setRefImage(imageList[0]);
      generator->setCmpImage(imageList[1]);
      generator->initialize();

      ossimRefPtr<AtpTileSource> atpTileSource = generator->getAtpTileSource();
      atpTileSource->setTiePoints(tpList);
      atpTileSource->filterPoints();

      //ossimGrect grect (41.9339,12.4563,41.9316,12.4594);
      //ossimDrect drect;
      //generator->getRefIVT()->getViewGeometry()->worldToLocal(grect, drect);
      //generator->m_annotatedRefImage->setAOI(drect);
      //generator->m_annotatedCmpImage->setAOI(drect);

      generator->m_annotatedRefImage->write();
      generator->m_annotatedCmpImage->write();

      cout<<"\nDone!"<<endl;
      generator.reset();
   }
   catch(const ossimException& e)
   {
      CWARN << e.what() << std::endl;
      returnCode = 1;
   }
   catch( ... )
   {
      CWARN << "atp-pax-test caught unhandled exception!" << std::endl;
      returnCode = 1;
   }

   return returnCode;
}
