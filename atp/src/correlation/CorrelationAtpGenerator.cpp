//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#include "CorrelationAtpGenerator.h"
#include "../AtpConfig.h"
#include <ossim/imaging/ossimImageRenderer.h>
#include <ossim/imaging/ossimCacheTileSource.h>
#include <ossim/base/ossimConnectableObject.h>
#include <ossim/base/ossimStringProperty.h>

using namespace std;

#define DEBUG_ATP

namespace ATP
{
CorrelationAtpGenerator::CorrelationAtpGenerator()
{
}

void CorrelationAtpGenerator::initialize()
{
   AtpGeneratorBase::initialize();

   // Add the CorrelationSource filter to the REF chain, then add the CMP image to the correlator:
   m_atpTileSource = new ossimCorrelationSource(this);
   //m_atpTileSource->setViewGeom(m_viewGeom.get());

   // Adjust AOI for half-width of correlation window:
   AtpConfig& config = AtpConfig::instance();
   int patch_center = (config.getParameter("corrWindowSize").asUint() + 1) / 2;
   ossimIpt first_pos(m_aoiView.ul().x + patch_center, m_aoiView.ul().y + patch_center);
   ossimIpt last_pos(m_aoiView.lr().x - patch_center, m_aoiView.lr().y - patch_center);
   m_aoiView.set_ul(first_pos);
   m_aoiView.set_lr(last_pos);
}

CorrelationAtpGenerator::~CorrelationAtpGenerator()
{
}

ossimRefPtr<ossimImageChain>
CorrelationAtpGenerator::constructChain(shared_ptr<Image> image,
                                        ossimRefPtr<ossimImageViewProjectionTransform>& ivt,
                                        vector<ossimDpt>& validVertices)
{
   static const char* MODULE="CorrelationAtpGenerator::constructChain()  ";

   ossimRefPtr<ossimImageChain> chain = AtpGeneratorBase::constructChain(image, ivt, validVertices);
   if (!chain)
      return NULL;

   // Renderer. The view geometry is set to the tie-point and GSD of the first (REF) image:
   ossimImageRenderer* renderer = new ossimImageRenderer;
   chain->add(renderer);
   renderer->setImageViewTransform(ivt.get());

   // Set the appropriate resampling filter for ATP. TODO: Need experimentation.
   ossimString filterType = AtpConfig::instance().getParameter("resamplingMethod").asString();
   if (filterType.empty())
      filterType = "bilinear";
   ossimRefPtr<ossimProperty> p = new ossimStringProperty("filter_type", filterType);
   renderer->setProperty(p);

   // Add cache after resampler:
   ossimCacheTileSource* cache = new ossimCacheTileSource();
   chain->add(cache);

   if (AtpConfig::instance().diagnosticLevel(3))
   {
      ossimDpt imagePt(1,1);
      ossimDpt viewPt, rtIpt;
      ivt->imageToView(imagePt, viewPt);
      ivt->viewToImage(viewPt, rtIpt);
      clog<<MODULE<<"\n  imagePt: "<<imagePt<<
            "\n  viewPt: "<<viewPt<<"\n  rtIpt: "<<rtIpt<<endl;
   }
   return chain;
}

#if 0
bool CorrelationAtpGenerator::renderTiePointDataAsImages(const ossimFilename& outFile,
                                                         const ossimFilename& lutFile)
{
   ossimImageSource* chainHead = 0;
   initialize();
   AtpConfig& corrInfo = AtpConfig::instance();

   // Tile source of correlations and residuals:
   ossimRefPtr<ossimCorrelationSource> correlationSource = new ossimCorrelationSource;
   correlationSource->connectMyInputTo(m_refChain.get());
   correlationSource->connectMyInputTo(m_cmpChain.get());
   correlationSource->setRefEllipsoidHgt(m_refEllipHgt);
   chainHead = correlationSource.get();
   chainHead->initialize();

   // Smoothing:
   ossimRefPtr<ossimMeanMedianFilter> mmf2 = new ossimMeanMedianFilter();
   mmf2->connectMyInputTo(chainHead);
   mmf2->setFilterType(ossimMeanMedianFilter::OSSIM_MEAN_FILL_NULLS);
   mmf2->setWindowSize(5);
   chainHead = mmf2.get();
   chainHead->initialize();

   // Write a two-band image containing correlations and residuals
   ossimFilename tmpFile(outFile.fileNoExtension() + "_TMP.tif");
   writeImage(chainHead, m_aoiView, tmpFile, "");

   // Open the 2-band product image:
   ossimRefPtr<ossimImageHandler> handler =
         ossimImageHandlerRegistry::instance()->open(tmpFile, true, false);
   if (!handler)
      return false;
   chainHead = handler.get(); // Start of new chain here, no resampler needed

   // Establish the output rect:
   ossimRefPtr<ossimImageGeometry> geom (handler->getImageGeometry().get());
   if (!geom)
      return false;
   ossimDrect bounding_rect;
   geom->getBoundingRect(bounding_rect);
   m_aoiView = ossimIrect(bounding_rect);

   ossimRefPtr<ossimBandSelector> bandSelector (new ossimBandSelector());
   bandSelector->connectMyInputTo(chainHead);
   vector<ossim_uint32> bandId(1);
   bandSelector->enableSource();
   chainHead = bandSelector.get();
   chainHead->initialize();

   // Need to scale results:
   ossimRefPtr<ossimLinearStretchRemapper> remapper (new ossimLinearStretchRemapper);
   remapper->connectMyInputTo(chainHead);
   double peakThreshold = AtpConfig::instance().peakThreshold();
   double maxResidual = correlationSource->getMaxResidualMagnitude();
   chainHead = remapper.get();
   chainHead->initialize();

   // Scale to 8-bit:
   ossimRefPtr<ossimScalarRemapper> scalarRemap (new ossimScalarRemapper);
   scalarRemap->connectMyInputTo(chainHead);
   scalarRemap->setOutputScalarType(OSSIM_UINT8);
   chainHead = scalarRemap.get();
   chainHead->initialize();

   if (!lutFile.empty())
   {
      ossimRefPtr<ossimIndexToRgbLutFilter> lut (new ossimIndexToRgbLutFilter());
      lut->connectMyInputTo(chainHead);
      lut->setLut(lutFile);
      chainHead = lut.get();
      chainHead->initialize();
   }

   // Correlation file output:
   bandId[0] = 0;
   bandSelector->setOutputBandList(bandId);
   remapper->setMinPixelValue(peakThreshold, 0);
   remapper->setMaxPixelValue(1.0, 0);
   ossimFilename corrFile(outFile.fileNoExtension() + "_COR.tif");
   writeImage(chainHead, m_aoiView, corrFile, lutFile);

   // Residual file output:
   bandId[0] = 1;
   bandSelector->setOutputBandList(bandId);
   remapper->setMinPixelValue(0.0, 0);
   remapper->setMaxPixelValue(maxResidual, 0);
   ossimFilename resFile(outFile.fileNoExtension() + "_RES.tif");
   writeImage(chainHead,  m_aoiView, resFile, lutFile);

   // Remove the intermediate 2-band file
   ossimFilename::remove(tmpFile);
   return true;
}
#endif

}
