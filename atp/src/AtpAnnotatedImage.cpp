//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#include "AtpAnnotatedImage.h"
#include "AtpConfig.h"
#include "../AtpCommon.h"
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageDataFactory.h>
#include <ossim/imaging/ossimBandSelector.h>
#include <ossim/imaging/ossimImageRenderer.h>
#include <ossim/imaging/ossimCacheTileSource.h>
#include <ossim/imaging/ossimTiffWriter.h>
#include <ossim/imaging/ossimIndexToRgbLutFilter.h>
#include <ossim/imaging/ossimVertexExtractor.h>
#include <ossim/elevation/ossimElevManager.h>
#include <ossim/imaging/ossimLinearStretchRemapper.h>
#include <ossim/imaging/ossimAnnotationLineObject.h>
#include <ossim/imaging/ossimAnnotationFontObject.h>

using namespace std;

#define DEBUG_ATP

namespace ATP
{
AtpAnnotatedImage::AtpAnnotatedImage(ossimRefPtr<ossimImageChain>& sourceChain,
                                     ossimRefPtr<ossimImageViewProjectionTransform>& ivt,
                                     const ossimDrect& aoi)
:   m_annFilename("annotated.tif")
{
   const char* MODULE = "AtpGeneratorBase::setupAnnotatedChain()  ";

   // Combine inputs into a two-color multiview
//   ossimRefPtr<ossimTwoColorView> mosaic (new ossimTwoColorView);
//   mosaic->connectMyInputTo(m_refChain.get());
//   mosaic->connectMyInputTo(m_cmpChain.get());
//   m_annChain->add(mosaic.get());
   add(sourceChain.get()); // just use ref image.

   // Insure there is a renderer to output to a common view space:
   ossimTypeNameVisitor vref ("ossimImageRenderer");
   sourceChain->accept(vref);
   ossimImageRenderer* r = vref.getObjectAs<ossimImageRenderer>();
   if (!r)
   {
      auto renderer = new ossimImageRenderer;
      add(renderer);
      renderer->setImageViewTransform(ivt.get());

      // Add cache after resampler:
      auto * cache = new ossimCacheTileSource();
      add(cache);
   }

   // The ref chain is necessarily single band. Need to map to RGB:
   ossimRefPtr<ossimBandSelector> band_selector = new ossimBandSelector;
   add(band_selector.get());
   vector<ossim_uint32> bandList;
   bandList.push_back(0);
   bandList.push_back(0);
   bandList.push_back(0);
   band_selector->setOutputBandList(bandList, true);

   // Add the graphics annotation source:
   m_annSource = new ossimAnnotationSource;
   add(m_annSource.get());

   initialize();

   // Keep track of the view geometry's image size to scale output when writing annotated chain:
   int annSize = AtpConfig::instance().getParameter("annotatedImageSize").asInt();
   if (annSize <= 0)
      m_annScale = ossimDpt(1, 1);
   else
      m_annScale = ossimDpt(aoi.width()/annSize, aoi.height()/annSize);

   if (m_annScale.y > m_annScale.x)
      m_annScale.x = m_annScale.y;
   else
      m_annScale.y = m_annScale.x;
   if (m_annScale.x < 1.0)
      m_annScale = ossimDpt(1.0, 1.0);
   m_annScaleInverse = ossimDpt(1.0/m_annScale.x, 1.0/m_annScale.y);

   // Establish the AOI in scaled space:
   m_annScaledAoi = aoi * m_annScaleInverse.x;
}

AtpAnnotatedImage::~AtpAnnotatedImage()
{

}

void AtpAnnotatedImage::annotateResiduals(AtpList& atps, int r, int g, int b)
{
   if (!m_annSource)
      return;

   int halfWidth = AtpConfig::instance().getParameter("corrWindowSize").asUint() * m_annScaleInverse.x / 2 ;
   if (halfWidth < 3)
      halfWidth = 3;

   AtpConfig& config = AtpConfig::instance();
   double residualMultiplier = config.getParameter("residualMultiplier").asFloat();

   ossimDpt refPt, cmpPt, residual;
   for (auto &atp : atps)
   {
      atp->getRefViewPoint(refPt);
      refPt *= m_annScaleInverse.x;

      // Draw box around feature:
      ossimIrect box;
      box.set_ul (ossimIpt(refPt.x - halfWidth, refPt.y - halfWidth));
      box.set_ur (ossimIpt(refPt.x + halfWidth, refPt.y - halfWidth));
      box.set_lr (ossimIpt(refPt.x + halfWidth, refPt.y + halfWidth));
      box.set_ll (ossimIpt(refPt.x - halfWidth, refPt.y + halfWidth));
      drawBox(box, r, g, 0);

      // Draw residual vector:
      atp->getVectorResidual(residual);
      cmpPt.x = refPt.x + residualMultiplier*residual.x;
      cmpPt.y = refPt.y + residualMultiplier*residual.y;
      ossimRefPtr<ossimAnnotationLineObject> l = new ossimAnnotationLineObject(refPt,cmpPt,r,g,b);
      m_annSource->addObject(l.get());
   }
}
void AtpAnnotatedImage::annotateTPIDs(AtpList& atps, int r, int g, int b)
{
   if (!m_annSource)
      return;

   ossimDpt refPt, cmpPt, residual;
   for (auto &atp : atps)
   {
      atp->getRefViewPoint(refPt);
      ossimRefPtr<ossimAnnotationFontObject> f =
            new ossimAnnotationFontObject(refPt, atp->getTiePointId());
      m_annSource->addObject(f.get());
   }
}

void AtpAnnotatedImage::annotateFeatures(vector<ossimDpt>& features, int r, int g, int b)
{
   if (!m_annSource)
      return;

   AtpConfig& config = AtpConfig::instance();
   int d = 1 ;

   for (auto &p : features)
   {
      // For each correlation, draw +, scaling to the size of the annotated image:
      p *= m_annScaleInverse.x;
      ossimDpt p1 (p.x, p.y-d);
      ossimDpt p2 (p.x, p.y+d);
      ossimDpt p3 (p.x-d, p.y);
      ossimDpt p4 (p.x+1, p.y);

      ossimAnnotationLineObject* line1 = new ossimAnnotationLineObject(p1, p2, r, g, b);
      ossimAnnotationLineObject* line2 = new ossimAnnotationLineObject(p3, p4, r, g, b);

      m_annSource->addObject(line1);
      m_annSource->addObject(line2);
   }
}

void AtpAnnotatedImage::annotateCorrelations(AtpList& atpList, int r, int g, int b)
{
   if (!m_annSource)
      return;

   AtpConfig& config = AtpConfig::instance();
   int halfWidth = config.getParameter("corrWindowSize").asUint() * m_annScaleInverse.x / 2 ;
   if (halfWidth < 3)
      halfWidth = 3;

   double residualMultiplier = config.getParameter("residualMultiplier").asFloat();
   if (residualMultiplier < 1)
      residualMultiplier = 1.0;

   ossimDpt refPt, cmpPt, residual;
   for (auto &atp : atpList)
   {
      // For each correlation, draw a box, scaling to the size of the annotated image:
      atp->getCmpViewPoint(cmpPt);
      cmpPt *= m_annScaleInverse.x;

      ossimIrect box;
      box.set_ul (ossimIpt(cmpPt.x - halfWidth, cmpPt.y - halfWidth));
      box.set_ur (ossimIpt(cmpPt.x + halfWidth, cmpPt.y - halfWidth));
      box.set_lr (ossimIpt(cmpPt.x + halfWidth, cmpPt.y + halfWidth));
      box.set_ll (ossimIpt(cmpPt.x - halfWidth, cmpPt.y + halfWidth));
      drawBox(box, r, g, b);

      // Draw residual vector:
      atp->getVectorResidual(residual);
      refPt.x = cmpPt.x - residualMultiplier*residual.x;
      refPt.y = cmpPt.y - residualMultiplier*residual.y;
      ossimRefPtr<ossimAnnotationLineObject> l = new ossimAnnotationLineObject(cmpPt,refPt,r,g,b);
      m_annSource->addObject(l.get());
   }
}

void AtpAnnotatedImage::annotateOverlap(ossimPolygon& poly)
{
   ossimDpt ptA, ptB;
   const vector<ossimDpt>& vlist = poly.getVertexList();

   for (size_t i=0; i<vlist.size(); ++i)
   {
      ptA = vlist[i];
      if (i==0)
         ptB = vlist.back();
      else
         ptB = vlist[i-1];

      ptA *= m_annScaleInverse.x;
      ptB *= m_annScaleInverse.x;

      ossimAnnotationLineObject* line = new ossimAnnotationLineObject(ptA, ptB, 0,255,255);
      m_annSource->addObject(line);
   }
}

void AtpAnnotatedImage::annotateFeatureSearchTiles(std::vector<ossimIrect>& searchTileRects)
{
   const char* MODULE = "AtpAnnotatedImage::annotateFeatureSearchTiles()  ";

   if (!m_annSource)
      return;

   if (searchTileRects.empty())
   {
      CINFO<<MODULE<<"No feature search tiles were established in the overlap."<<endl;
      return;
   }

   ossimDpt pt;

   for (auto &searchTileRect : searchTileRects)
   {
      ossimIrect box;

      // For each search rect, draw a box:
      pt.x = searchTileRect.ul().x;
      pt.y = searchTileRect.ul().y;
      box.set_ul(pt * m_annScaleInverse.x);

      pt.x = searchTileRect.ur().x;
      pt.y = searchTileRect.ur().y;
      box.set_ur(pt * m_annScaleInverse.x);

      pt.x = searchTileRect.lr().x;
      pt.y = searchTileRect.lr().y;
      box.set_lr(pt * m_annScaleInverse.x);

      pt.x = searchTileRect.ll().x;
      pt.y = searchTileRect.ll().y;
      box.set_ll(pt * m_annScaleInverse.x);

      drawBox(box, 200, 255, 100);
   }

}

void AtpAnnotatedImage::annotateCorrelationSearchTile(const ossimIrect& tileRect)
{
   if (!m_annSource)
      return;

   ossimIrect scaledRect = tileRect * m_annScaleInverse.x;
   drawBox(scaledRect, 255, 200, 100);
}

bool AtpAnnotatedImage::write()
{
   ossimRefPtr<ossimTiffWriter> writer (new ossimTiffWriter);
   writer->setFilename(m_annFilename);
   writer->connectMyInputTo(this);
   writer->setAreaOfInterest(m_annScaledAoi);
   writer->initialize();
   bool status = writer->execute();
   writer->close();

   if (status)
      CINFO<<"\nAtpAnnotatedImage::write() -- Wrote "<<m_annFilename<<endl;

   return status;
}

void AtpAnnotatedImage::drawBox(const ossimIrect& box, int r, int g, int b)
{
   ossimAnnotationLineObject* line1 = new ossimAnnotationLineObject(box.ul(), box.ur(), r, g, b);
   ossimAnnotationLineObject* line2 = new ossimAnnotationLineObject(box.ur(), box.lr(), r, g, b);
   ossimAnnotationLineObject* line3 = new ossimAnnotationLineObject(box.lr(), box.ll(), r, g, b);
   ossimAnnotationLineObject* line4 = new ossimAnnotationLineObject(box.ll(), box.ul(), r, g, b);

   m_annSource->addObject(line1);
   m_annSource->addObject(line2);
   m_annSource->addObject(line3);
   m_annSource->addObject(line4);
}

}
