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

// ossim includes:  These are here just to save time/typing...
#include <ossim/base/ossimApplicationUsage.h>
#include <ossim/base/ossimArgumentParser.h>
#include <ossim/base/ossimConnectableObject.h>
#include <ossim/base/ossimCsvFile.h>
#include <ossim/base/ossimDate.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimDrect.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimObjectFactory.h>
#include <ossim/base/ossimObjectFactoryRegistry.h>
#include <ossim/base/ossimProperty.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimScalarTypeLut.h>
#include <ossim/base/ossimStdOutProgress.h>
#include <ossim/base/ossimStreamFactoryRegistry.h>
#include <ossim/base/ossimStringProperty.h>
#include <ossim/base/ossimTimer.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimUrl.h>
#include <ossim/base/ossimVisitor.h>
#include <ossim/base/ossimEcefPoint.h>
#include <ossim/base/ossimEcefVector.h>
#include <ossim/base/ossim2dBilinearTransform.h>

#include <ossim/imaging/ossimNitfTileSource.h>
#include <ossim/imaging/ossimBrightnessContrastSource.h>
#include <ossim/imaging/ossimBumpShadeTileSource.h>
#include <ossim/imaging/ossimFilterResampler.h>
#include <ossim/imaging/ossimFusionCombiner.h>
#include <ossim/imaging/ossimImageData.h>
#include <ossim/imaging/ossimImageFileWriter.h>
#include <ossim/imaging/ossimImageGeometry.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/imaging/ossimImageMosaic.h>
#include <ossim/imaging/ossimImageRenderer.h>
#include <ossim/imaging/ossimImageSource.h>
#include <ossim/imaging/ossimImageSourceFilter.h>
#include <ossim/imaging/ossimImageToPlaneNormalFilter.h>
#include <ossim/imaging/ossimImageWriterFactoryRegistry.h>
#include <ossim/imaging/ossimIndexToRgbLutFilter.h>
#include <ossim/imaging/ossimRectangleCutFilter.h>
#include <ossim/imaging/ossimScalarRemapper.h>
#include <ossim/imaging/ossimSFIMFusion.h>
#include <ossim/imaging/ossimTwoColorView.h>
#include <ossim/imaging/ossimImageSourceFactoryRegistry.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/support_data/ImageHandlerState.h>

#include <ossim/init/ossimInit.h>

#include <ossim/projection/ossimEquDistCylProjection.h>
#include <ossim/projection/ossimImageViewAffineTransform.h>
#include <ossim/projection/ossimMapProjection.h>
#include <ossim/projection/ossimProjection.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <ossim/projection/ossimUtmProjection.h>

#include <ossim/support_data/ossimSrcRecord.h>
#include <ossim/support_data/ossimWkt.h>

#include <ossim/base/Barrier.h>
#include <ossim/base/Block.h>
#include <ossim/base/Thread.h>
#include <ossim/support_data/TiffHandlerState.h>
#include <ossim/support_data/ImageHandlerStateRegistry.h>

// Put your includes here:
#include<ossim/projection/ossimNitfRpcModel.h>
#include<ossim/projection/ossimQuickbirdRpcModel.h>
#include<json/json.h>

using namespace std;

int main(int argc, char *argv[])
{
   int returnCode = 0;
   
   ossimArgumentParser ap(&argc, argv);
   ossimInit::instance()->addOptions(ap);
   ossimInit::instance()->initialize(ap);

   if (argc < 2)
   {
      cout<<"\nUsage: "<<argv[0]<<"<triangulation-results.json> <mensuration-input.json>\n"<<endl;
      cout<<"\n       "<<argv[0]<<"<mensuration-output.json>\n"<<endl;
      exit(0);
   }

   try
   {
      ifstream jsonInput (argv[1]);
      Json::Value root;
      jsonInput >> root;

      if (argc > 2)
      {
         Json::Value outRoot;

         outRoot["service"] = "mensuration";
         outRoot["method"] = "pointExtraction";
         outRoot["outputCoordinateSystem"] = "ecf";

         const Json::Value& photoblock = root["photoblock"];
         outRoot["images"] = photoblock["images"];
         outRoot["observations"] = photoblock["tiePoints"];

         ofstream outFile (argv[2]);
         outFile << outRoot << endl;
         outFile.close();
      }
      else
      {
         const Json::Value& mensurationReport = root["mensurationReport"];
         unsigned int count = mensurationReport.size();
         double ce90, le90;
         double sumCe90 = 0;
         double sumLe90 = 0;
         double maxCe90 = 0;
         double maxLe90 = 0;
         for (unsigned int i=0; i<count; ++i)
         {
            ce90 = mensurationReport[i]["ce90"].asDouble();
            le90 = mensurationReport[i]["le90"].asDouble();

            if (ce90 > maxCe90)
               maxCe90 = ce90;
            if (le90 > maxLe90)
               maxLe90 = le90;

            sumCe90 += ce90;
            sumLe90 += le90;
         }

         double meanCe90 = sumCe90/count;
         double meanLe90 = sumLe90/count;

         cout << "\n Mean CE90: " << meanCe90<<"\n";
         cout << "  Max CE90: " << maxCe90<<"\n";
         cout << "\n Mean LE90: " << meanLe90<<"\n";
         cout << "  Max LE90: " << maxLe90<<"\n"<<endl;
      }

   }
   catch(const ossimException& e)
   {
      ossimNotify(ossimNotifyLevel_WARN) << e.what() << std::endl;
      returnCode = 1;
   }
   catch( ... )
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << "ossim-foo caught unhandled exception!" << std::endl;
      returnCode = 1;
   }
   
   return returnCode;
}
