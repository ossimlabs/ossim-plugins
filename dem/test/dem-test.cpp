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
#include <dem/src/ossimDemTool.h>

using namespace std;

int main(int argc, char *argv[])
{
   int returnCode = 0;
   
   ossimArgumentParser ap(&argc, argv);
   ossimInit::instance()->addOptions(ap);
   ossimInit::instance()->initialize(ap);

   if (argc < 3)
   {
      cout<<"\nUsage: "<<argv[0]<<"<image-A> <image-B>\n"<<endl;
      exit(0);
   }

   try
   {
      ossimFilename imageA (argv[1]);
      ossimFilename imageB (argv[2]);

      Json::Value rootJson;
      rootJson["algorithm"] = "asp";
      rootJson["method"] = "generate";
      rootJson["filename"] = "dem-test-output.tif";

      Json::Value imageAJson;
      imageAJson["filename"] = imageA.string();
      Json::Value imageBJson;
      imageBJson["filename"] = imageB.string();
      Json::Value imagesJson;
      imagesJson[0] = imageAJson;
      imagesJson[1] = imageBJson;

      Json::Value pbJson;
      pbJson["images"] = imagesJson;
      rootJson["photoblock"] = pbJson;

      cout<<"Input JSON:\n"<<rootJson<<endl;

      ossimDemTool demTool;
      demTool.loadJSON(rootJson);

      demTool.execute();
   }
   catch(const ossimException& e)
   {
      ossimNotify(ossimNotifyLevel_WARN) << e.what() << std::endl;
      returnCode = 1;
   }
   catch( ... )
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << argv[0] <<" caught unhandled exception!" << std::endl;
      returnCode = 1;
   }
   
   return returnCode;
}
