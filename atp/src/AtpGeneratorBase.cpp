//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#include "AtpGeneratorBase.h"
#include "AutoTiePoint.h"
#include "AtpConfig.h"
#include "AtpTileSource.h"
#include <ossim/base/ossimPolygon.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageDataFactory.h>
#include <ossim/imaging/ossimBandSelector.h>
#include <ossim/imaging/ossimImageRenderer.h>
#include <ossim/imaging/ossimScalarRemapper.h>
#include <ossim/imaging/ossimCacheTileSource.h>
#include <ossim/imaging/ossimTiffWriter.h>
#include <ossim/imaging/ossimMeanMedianFilter.h>
#include <ossim/imaging/ossimIndexToRgbLutFilter.h>
#include <ossim/imaging/ossimVertexExtractor.h>
#include <ossim/projection/ossimUtmProjection.h>
#include <ossim/projection/ossimImageViewProjectionTransform.h>
#include <ossim/elevation/ossimElevManager.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/imaging/ossimLinearStretchRemapper.h>
#include <ossim/imaging/ossimImageSourceSequencer.h>
#include <ossim/imaging/ossimAnnotationLineObject.h>
#include <ossim/imaging/ossimAnnotationFontObject.h>
#include <ossim/imaging/ossimTwoColorView.h>
#include <cmath>
#include <map>

using namespace std;

#define DEBUG_ATP

namespace ATP
{
std::shared_ptr<AutoTiePoint> AtpGeneratorBase::s_referenceATP;

AtpGeneratorBase::AtpGeneratorBase()
:   m_refEllipHgt(0)
{
}

AtpGeneratorBase::~AtpGeneratorBase()
{

}

void AtpGeneratorBase::setRefImage(shared_ptr<Image> ref_image)
{
   m_refImage = ref_image;
}

void AtpGeneratorBase::setCmpImage(shared_ptr<Image> cmp_image)
{
   m_cmpImage = cmp_image;
}

void AtpGeneratorBase::initialize()
{
   static const char* MODULE=" AtpGeneratorBase::initialize()  ";

   // Initialize the reference ATP containing the static data members used by all ATPs:
   s_referenceATP.reset(new AutoTiePoint());
   m_viewGeom = 0;

   // Establish image processing chains:
   vector<ossimDpt> refVertices, cmpVertices;
   m_refChain = constructChain(m_refImage, m_refIVT, refVertices);
   m_cmpChain = constructChain(m_cmpImage, m_cmpIVT, cmpVertices);
   if (!m_refChain || !m_cmpChain)
   {
      ossimNotify(ossimNotifyLevel_FATAL)<<MODULE<<"Null input chain(s)."<<endl;
      return;
   }


   if (AtpConfig::instance().diagnosticLevel(1))
   {
      clog<<"\n"<<MODULE<<"Initializing image pair with:"
            <<"\n  REF_ID: "<<m_refImage->getImageId()
            <<"\n  CMP_ID: "<<m_cmpImage->getImageId()<<endl;
   }

   // Establish the actual overlap polygon and set AOI as its bounding rect:
   ossimPolyArea2d refPoly (refVertices);
   ossimPolyArea2d cmpPoly (cmpVertices);
   ossimPolyArea2d overlapPoly (refPoly & cmpPoly);
   vector<ossimPolygon> intersectPolys;
   overlapPoly.getVisiblePolygons(intersectPolys);
   if (intersectPolys.empty())
   {
      clog<<MODULE<<"No intersect polygon found."<<endl;
      return;
   }
   ossimPolygon intersectPoly (intersectPolys[0]);

   ossimDrect viewDrect;
   overlapPoly.getBoundingRect(viewDrect);
   m_aoiView = viewDrect;
   m_viewGeom->setImageSize(m_aoiView.size());

   if (AtpConfig::instance().diagnosticLevel(3))
   {
      m_annotatedRefImage = new AtpAnnotatedImage(m_refChain, m_refIVT, m_aoiView);
      m_annotatedRefImage->setImageName("annotatedRef.tif");
      m_annotatedCmpImage = new AtpAnnotatedImage(m_cmpChain, m_cmpIVT, m_aoiView);
      m_annotatedCmpImage->setImageName("annotatedCmp.tif");
   }

   // Establish the feature search tile locations:
   layoutSearchTileRects(intersectPoly);

   // Establish the nominal height above ellipsoid for MSL at this location:
   ossimRefPtr<ossimImageGeometry> refGeom = m_refChain->getImageGeometry();
   if (!refGeom)
   {
      ossimNotify(ossimNotifyLevel_FATAL)<<MODULE<<"Null ref image geometry."<<endl;
      return;
   }
   ossimDpt viewCenter;
   ossimGpt geoCenter;
   m_aoiView.getCenter(viewCenter);
   refGeom->localToWorld(viewCenter, geoCenter);
   m_refEllipHgt = ossimElevManager::instance()->getHeightAboveEllipsoid(geoCenter);
   geoCenter.height(m_refEllipHgt); // Sea level
   ossimElevManager::instance()->setDefaultHeightAboveEllipsoid(m_refEllipHgt);
   ossimElevManager::instance()->setUseGeoidIfNullFlag(true);
}

ossimRefPtr<ossimImageChain>
AtpGeneratorBase::constructChain(shared_ptr<Image> image,
                                 ossimRefPtr<ossimImageViewProjectionTransform>& ivt,
                                 vector<ossimDpt>& validVertices)
{
   static const char* MODULE="AtpGeneratorBase::constructChain()  ";
   AtpConfig& config = AtpConfig::instance();

   ossimImageHandlerRegistry* factory = ossimImageHandlerRegistry::instance();
   ossimRefPtr<ossimImageHandler> handler = factory->open(image->getFilename());
   if (!handler)
      return NULL;
   handler->setCurrentEntry(image->getEntryIndex());

   // Check if an imageID was provided in job JSON. otherwise, check if image handler can get it:
   ossimString imageId = image->getImageId();
   if (imageId.empty())
   {
      imageId = handler->getImageID();
      if (imageId.empty())
         imageId = image->getFilename();
   }
   handler->setImageID(imageId);
   image->setImageId(imageId);

   if (config.diagnosticLevel(3))
   {
      ossimIrect vuRect;
      handler->getImageGeometry()->getBoundingRect(vuRect);
      clog<<MODULE<<"m_aoiView: "<<vuRect<<endl;;
   }

   ossimRefPtr<ossimImageChain> chain = new ossimImageChain;
   chain->add(handler.get());

   // Need to select a band if multispectral:
   unsigned int numBands = handler->getNumberOfOutputBands();
   if (numBands > 1)
   {
      unsigned int band = image->getActiveBand();
      if ((band > numBands) || (band <= 0))
      {
         clog<<MODULE<<"Specified band ("<<band<<") is outside allowed range (1 to "<<numBands
               <<"). Using default band 1 which may not be ideal."<<endl;
         band = 1;
      }
      band--; // shift to zero-based
      ossimRefPtr<ossimBandSelector> band_selector = new ossimBandSelector();
      chain->add(band_selector.get());
      vector<ossim_uint32> bandList;
      bandList.push_back(band);
      band_selector->setOutputBandList(bandList);
   }

   // Can only work in 8-bit radiometry:
   if (chain->getOutputScalarType() != OSSIM_UINT8)
   {
      ossimRefPtr<ossimScalarRemapper> remapper = new ossimScalarRemapper();
      chain->add(remapper.get());
      remapper->setOutputScalarType(OSSIM_UINT8);
   }

   // Set up a common view geometry, even if derived class works exclusively in image space.
   ossimRefPtr<ossimImageGeometry> image_geom = handler->getImageGeometry();
   if (!m_viewGeom)
   {
      ossimDpt gsd(image_geom->getMetersPerPixel());
      if (gsd.x > gsd.y)
         gsd.y = gsd.x;
      else
         gsd.x = gsd.y;
      ossimGpt proj_origin;
      image_geom->getTiePoint(proj_origin, false);
      ossimUtmProjection* proj = new ossimUtmProjection();
      proj->setOrigin(proj_origin);
      proj->setMetersPerPixel(gsd);
      proj->setUlTiePoints(proj_origin);
      m_viewGeom = new ossimImageGeometry(NULL, proj);
   }
   else
   {
      // View geometry was already set up by first image of the pair (REF). Only need to check the
      // GSD of the CMP image and adjust the view GSD to match if larger:
      ossimDpt cmpGsd (image_geom->getMetersPerPixel());
      if (cmpGsd.x > cmpGsd.y)
         cmpGsd.y = cmpGsd.x;
      else
         cmpGsd.x = cmpGsd.y;
      ossimDpt refGsd (m_viewGeom->getMetersPerPixel());
      if (cmpGsd.x > refGsd.x)
      {
         refGsd.x = cmpGsd.x;
         refGsd.y = cmpGsd.y;
         m_viewGeom->getAsMapProjection()->setMetersPerPixel(refGsd);
      }
   }

   // Need to establish the valid image vertices for establishing overlap polygon. Compute in
   // image space then transform the vertices to view space:
   if (handler->openValidVertices())
   {
      vector<ossimIpt> iverts;
      handler->getValidImageVertices(iverts);
      for (int i=0; i<iverts.size(); ++i)
         validVertices.push_back(iverts[i]);
   }
   else
   {
      ossimFilename verticesFile (handler->createDefaultValidVerticesFilename());
      ossimRefPtr<ossimVertexExtractor> ve = new ossimVertexExtractor(handler.get());
      ve->setOutputName(verticesFile);
      ve->setAreaOfInterest(handler->getBoundingRect(0));
      ve->execute();
      const vector<ossimIpt>& iverts = ve->getVertices();
      for (int i=0; i<iverts.size(); ++i)
         validVertices.push_back(iverts[i]);
   }

   // The image vertices are necessarily in image space coordinates. Need to transform to a common
   // "view" space before determining the overlap polygon:
   ivt = new ossimImageViewProjectionTransform(image_geom.get(), m_viewGeom.get());
   ossimDpt ipt, vpt;
   for (size_t i=0; i<validVertices.size(); ++i)
   {
      ipt = validVertices[i];
      ivt->imageToView(ipt, vpt);
      validVertices[i] = vpt;
   }

   // Create a renderer even if it won't be used, just
   return chain;
}

void AtpGeneratorBase::writeTiePointList(ostream& out, const AtpList& tpList)
{
   if (tpList.empty())
   {
      out<<"\nTiepoint list is empty.\n"<<endl;
   }
   else
   {
      for (size_t i=0; i<tpList.size(); ++i)
      {
         out<<"\ntiepoint["<<i<<"]: "<<*tpList[i]<<endl;
      }
   }
}

bool AtpGeneratorBase::generateTiePointList(AtpList& atpList)
{
   const char* MODULE = "AtpGeneratorBase::generateTiePointList()  ";
   initialize();

   if (!m_refChain || !m_cmpChain || !m_atpTileSource)
      return false;

   AtpConfig& config = AtpConfig::instance();

   // Need to search for features in the REF tile first, then consider only those locations
   // for correlating. The search tiles were previously established in the base class initialization
   ossimRefPtr<ossimImageData> ref_tile = 0;
   ossim_int64 tileId = 0;
   for (int tileIdx=0; tileIdx<m_searchTileRects.size(); ++tileIdx)
   {
      ref_tile = m_atpTileSource->getTile(m_searchTileRects[tileIdx]);
      if (ref_tile->getDataObjectStatus() == OSSIM_EMPTY)
      {
         if (config.diagnosticLevel(3))
            clog<<MODULE<<"WARNING: Empty search tile returned from CorrelationSource."<<endl;
         continue;
      }

      AtpList& tileATPs = m_atpTileSource->getTiePoints();

      if (config.diagnosticLevel(3))
      {
         // Annotate red before filtering:
         m_annotatedRefImage->annotateResiduals(tileATPs, 255, 0, 0);
         m_annotatedCmpImage->annotateCorrelations(tileATPs, 255, 0, 0);
      }

      filterBadMatches(tileATPs);
      if (config.diagnosticLevel(3))
      {
         // Annotate yellow before pruning:
         m_annotatedRefImage->annotateResiduals(tileATPs, 255, 255, 0);
         m_annotatedCmpImage->annotateCorrelations(tileATPs, 255, 255, 0);
      }

      pruneList(tileATPs);

      if (config.diagnosticLevel(2))
         clog<<MODULE<<"After filtering: num TPs in tile = "<<tileATPs.size()<<endl;

      // Add remaining tile CTPs to master CTP list:
      if (tileATPs.size())
         atpList.insert(atpList.end(), tileATPs.begin(), tileATPs.end());

      if (config.diagnosticLevel(3))
      {
         // Annotate Green after accepting TP:
         m_annotatedRefImage->annotateResiduals(tileATPs, 0, 255, 0);
         m_annotatedCmpImage->annotateCorrelations(tileATPs, 0, 255, 0);
         //m_annotatedRefImage->annotateTPIDs(tileATPs, 180, 180, 0);
      }
   }

   if (config.diagnosticLevel(1))
      clog<<"\n"<<MODULE<<"Total number of TPs found in pair = "<<atpList.size()<<endl;

   if (config.diagnosticLevel(3))
   {
      m_annotatedRefImage->write();
      m_annotatedCmpImage->write();
   }
   return true;
}

void AtpGeneratorBase::layoutSearchTileRects(ossimPolygon& intersectPoly)
{
   const char* MODULE = "AtpGeneratorBase::layoutSearchTiles()  ";
   AtpConfig& config = AtpConfig::instance();

   m_searchTileRects.clear();

   // Note this seems to be specific to correlation-based matching, but should be generalized:
   unsigned int featureSideSize = config.getParameter("corrWindowSize").asUint();

   // ### Critical value set here such that we can fit at least 1/2 the number of desired TPs:
   unsigned int numTpsPerTile = config.getParameter("numFeaturesPerTile").asUint();
   double min_area = numTpsPerTile*featureSideSize*featureSideSize/2.0;

   // Found issue with last vertex = first vertex. Remove last if so:
   int lastVertex = intersectPoly.getNumberOfVertices() - 1;
   if (lastVertex < 2)
   {
      clog<<MODULE<<"Intersect polygon num vertices is less than 3! This should never happen."<<endl;
      return;
   }
   if (intersectPoly[0] == intersectPoly[lastVertex])
   {
      intersectPoly.removeVertex(lastVertex);
      if (lastVertex < 3)
      {
         clog<<MODULE<<"Intersect polygon num vertices is less than 3! This should never happen."<<endl;
         return;
      }
   }

   double intersectArea = fabs(intersectPoly.area());
   if (intersectArea < min_area)
   {
      clog<<MODULE<<"Intersect area too small"<<endl;
      return;
   }

   if (config.diagnosticLevel(3) && m_annotatedRefImage)
      m_annotatedRefImage->annotateOverlap(intersectPoly);

   // Prepare to layout evenly-spaced search tiles across overlap bounding rect:
   ossimIrect aoiRect;
   intersectPoly.getBoundingRect(aoiRect);
   int side_length = (int) config.getParameter("featureSearchTileSize").asUint();
   int nx, ny; // Number of tiles in x and y directions
   int dx, dy; // tile width and height
   int aoiWidth = aoiRect.width();
   int aoiHeight = aoiRect.height();

   // Determine optimal X-direction space between tiles (actually between corresponding UL corners)
   int gridSize = (int) config.getParameter("searchGridSize").asUint();
   for (int nx=gridSize; nx>1; nx--)
   {
      dx = (int) floor(aoiWidth/nx);
      if (dx >= side_length)
         break;
   }

   // Determine optimal Y-direction space between tiles:
   for (int ny=gridSize; ny>1; ny--)
   {
      dy = (int) floor(aoiHeight/ny);
      if (dy >= side_length)
         break;
   }

   // Create tile rects now that the intervals are known:
   for (int y=aoiRect.ul().y; y<=aoiRect.lr().y-side_length; y+=dy)
   {
      for (int x=aoiRect.ul().x; x<=aoiRect.lr().x-side_length; x+=dx)
      {
         m_searchTileRects.push_back(ossimIrect (x, y, x+side_length-1, y+side_length-1));
      }
   }

   // Keep only those rects that are inside the overlap polygon. The layout was done over the
   // overlap's bounding rect, so some may spill outside the overlap:
   vector<ossimIrect>::iterator b = m_searchTileRects.begin();
   while (b != m_searchTileRects.end())
   {
      if(!intersectPoly.rectIntersects(*b))
         b = m_searchTileRects.erase(b);
      else
         b++;
   }

   // Don't bother with less than N boxes, just make the entire intersection a search patch:
   if (m_searchTileRects.size() < 4)
   {
      m_searchTileRects.clear();
      m_searchTileRects.push_back(m_aoiView);
   }

   if (config.diagnosticLevel(2))
   {
      for (int tileIdx=0; tileIdx<m_searchTileRects.size(); ++tileIdx)
         clog<<MODULE<<"m_searchTileRects["<<tileIdx<<"] = "<<m_searchTileRects[tileIdx]<<endl;

      if (config.diagnosticLevel(3) && m_annotatedRefImage)
         m_annotatedRefImage->annotateFeatureSearchTiles(m_searchTileRects);
   }
}

void AtpGeneratorBase::filterBadMatches(AtpList& tpList)
{
   const char* MODULE = "AtpGeneratorBase::filterBadPeaks()  ";

   // Can't filter without neighbors, so remove all just in case they are bad:
   AtpConfig& config = AtpConfig::instance();
   if (tpList.size() < config.getParameter("minNumConsistentNeighbors").asUint())
   {
      tpList.clear();
      return;
   }

   // Loop to eliminate bad peaks and inconsistent CTPs in tile:
   AtpList::iterator iter = tpList.begin();
   while (iter != tpList.end())
   {
      if (!(*iter)) // Should never happen
      {
         clog<<MODULE<<"WARNING: Null AutoTiePoint encountred in list. Skipping point."<<endl;
         ++iter;
         continue;
      }

      // The "tiepoint" may not have any matches -- it may be just a feature:
      if ((*iter)->hasValidMatch())
         (*iter)->findBestConsistentMatch(tpList);

      if (!(*iter)->hasValidMatch())
      {
         // Remove this TP from our list as soon as it is discovered that there are no valid
         // peaks. All TPs in theTpList must have valid peaks:
         if (config.diagnosticLevel(4))
            clog<<MODULE<<"Removing TP "<<(*iter)->getTiePointId()<<endl;
         iter = tpList.erase(iter);
         if (tpList.empty())
            break;
      }
      else
         iter++;
   }
}

void AtpGeneratorBase::pruneList(AtpList& tpList)
{
   const char* MODULE = "AtpGeneratorBase::pruneList()  ";
   multimap<double, shared_ptr<AutoTiePoint> > atpMap;

   // Use a map to sort by confidence measure:
   AtpList::iterator iter = tpList.begin();
   double confidence;
   while (iter != tpList.end())
   {
      if (!(*iter)) // Should never happen
      {
         clog<<MODULE<<"WARNING: Null AutoTiePoint encountred in list. Skipping point."<<endl;
         ++iter;
         continue;
      }

      // The "tiepoint" may not have any matches -- it may be just a feature:
      bool hasValidMatch = (*iter)->getConfidenceMeasure(confidence);
      if (hasValidMatch)
         atpMap.emplace(1.0/confidence, *iter);

      iter++;
   }

   // Skim off the top best:
   unsigned int N =  AtpConfig::instance().getParameter("numTiePointsPerTile").asUint();
   tpList.clear();
   multimap<double, shared_ptr<AutoTiePoint> >::iterator tp = atpMap.begin();
   while (tp != atpMap.end())
   {
      tpList.push_back(tp->second);
      if (tpList.size() == N)
         break;
      tp++;
   }
}

}
