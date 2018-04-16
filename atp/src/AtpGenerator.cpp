//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#include "AtpGenerator.h"
#include "AtpConfig.h"
#include "AtpTileSource.h"
#include "../AtpCommon.h"
#include "ossimCorrelationSource.h"
#include "ossimDescriptorSource.h"
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageDataFactory.h>
#include <ossim/imaging/ossimBandSelector.h>
#include <ossim/imaging/ossimImageRenderer.h>
#include <ossim/imaging/ossimScalarRemapper.h>
#include <ossim/imaging/ossimCacheTileSource.h>
#include <ossim/imaging/ossimTiffWriter.h>
#include <ossim/imaging/ossimIndexToRgbLutFilter.h>
#include <ossim/imaging/ossimVertexExtractor.h>
#include <ossim/projection/ossimUtmProjection.h>
#include <ossim/elevation/ossimElevManager.h>
#include <ossim/imaging/ossimLinearStretchRemapper.h>
#include <ossim/imaging/ossimAnnotationFontObject.h>
#include <ossim/imaging/ossimHistogramRemapper.h>
#include <ossim/base/ossimStringProperty.h>

using namespace std;
using namespace ossim;

#define DEBUG_ATP

namespace ATP
{
std::shared_ptr<AutoTiePoint> AtpGenerator::s_referenceATP;


AtpGenerator::AtpGenerator(Algorithm algo)
:   m_algorithm (algo),
    m_refEllipHgt(0)
{
}


AtpGenerator::~AtpGenerator()
{
}


void AtpGenerator::setRefImage(shared_ptr<Image> ref_image)
{
   m_refImage = ref_image;
}


void AtpGenerator::setCmpImage(shared_ptr<Image> cmp_image)
{
   m_cmpImage = cmp_image;
}


string  AtpGenerator::getRefImageID()
{
   ossimImageHandler* handler = getImageHandler(m_refChain);
   if (!handler)
      return "";
   return handler->getImageID().string();
}

string AtpGenerator::getCmpImageID()
{
   ossimImageHandler* handler = getImageHandler(m_cmpChain);
   if (!handler)
      return "";
   return handler->getImageID().string();
}

string  AtpGenerator::getRefFilename()
{
   ossimImageHandler* handler = getImageHandler(m_refChain);
   if (!handler)
      return "";
   return handler->getFilename().string();
}

string  AtpGenerator::getCmpFilename()
{
   ossimImageHandler* handler = getImageHandler(m_cmpChain);
   if (!handler)
      return "";
   return handler->getFilename().string();
}

ossimImageHandler* AtpGenerator::getImageHandler(ossimRefPtr<ossimImageChain>& chain)
{
   if (!chain)
      return 0;

   ossimTypeNameVisitor visitor ("ossimImageHandler");
   chain->accept(visitor);
   ossimImageHandler* handler = (ossimImageHandler*) visitor.getObjectAs<ossimImageHandler>();

   return handler;
}


void AtpGenerator::initialize()
{
   static const char* MODULE=" AtpGenerator::initialize()  ";
   AtpConfig &config = AtpConfig::instance();

   // Initialize the reference ATP containing the static data members used by all ATPs:
   s_referenceATP.reset(new AutoTiePoint());
   m_viewGeom = 0;

   // Establish image processing chains:
   vector<ossimDpt> refVertices, cmpVertices;
   m_refChain = constructChain(m_refImage, m_refIVT);
   m_cmpChain = constructChain(m_cmpImage, m_cmpIVT);
   if (!m_refChain || !m_cmpChain)
   {
      CFATAL<<MODULE<<"Null input chain(s)."<<endl;
      return;
   }

   if (config.diagnosticLevel(1))
   {
      CINFO<<"\n"<<MODULE<<"Initializing image pair with:"
            <<"\n  REF_ID: "<<m_refImage->getImageId()
            <<"\n  CMP_ID: "<<m_cmpImage->getImageId()<<endl;
   }

   // Establish the actual overlap polygon and set AOI as its bounding rect:
   getValidVertices(m_refChain, m_refIVT, refVertices);
   getValidVertices(m_cmpChain, m_cmpIVT, cmpVertices);
   ossimPolyArea2d refPoly (refVertices);
   ossimPolyArea2d cmpPoly (cmpVertices);
   ossimPolyArea2d overlapPoly (refPoly & cmpPoly);
   vector<ossimPolygon> intersectPolys;
   overlapPoly.getVisiblePolygons(intersectPolys);
   if (intersectPolys.empty())
   {
      CINFO<<MODULE<<"No intersect polygon found."<<endl;
      return;
   }
   ossimPolygon intersectPoly (intersectPolys[0]);

   ossimDrect viewDrect;
   overlapPoly.getBoundingRect(viewDrect);
   m_aoiView = viewDrect;
   m_viewGeom->setImageSize(m_aoiView.size());

   if (config.diagnosticLevel(3))
   {
      ossimString basename (config.getParameter("annotatedImageBaseName").asString());
      m_annotatedRefImage = new AtpAnnotatedImage(m_refChain, m_refIVT, m_aoiView);
      m_annotatedRefImage->setImageName(basename+"Ref.tif");
      m_annotatedCmpImage = new AtpAnnotatedImage(m_cmpChain, m_cmpIVT, m_aoiView);
      m_annotatedCmpImage->setImageName(basename+"Cmp.tif");
   }

   // Establish the feature search tile locations:
   layoutSearchTileRects(intersectPoly);

   // Establish the nominal height above ellipsoid for MSL at this location:
   ossimRefPtr<ossimImageGeometry> refGeom = m_refChain->getImageGeometry();
   if (!refGeom)
   {
      CFATAL<<MODULE<<"Null ref image geometry."<<endl;
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

   // Now need the algorithm-specific tile source that does the pixel pulling:
   switch (m_algorithm)
   {
   case CROSSCORR:
      m_atpTileSource = new ossimCorrelationSource(this);
      break;
   case DESCRIPTOR:
      m_atpTileSource = new ossimDescriptorSource(this);
      break;
   case NASA:
   case ALGO_UNASSIGNED:
   default:
      CFATAL<<MODULE<<"Unhandled algorithm type."<<endl;
      return;
   }

   // Adjust AOI for half-width of correlation window -- this really should only apply to crosscorr,
   // but descriptors also have a sampling window (just not readily known):
   int patch_center = (config.getParameter("corrWindowSize").asUint() + 1) / 2;
   ossimIpt first_pos(m_aoiView.ul().x + patch_center, m_aoiView.ul().y + patch_center);
   ossimIpt last_pos(m_aoiView.lr().x - patch_center, m_aoiView.lr().y - patch_center);
   m_aoiView.set_ul(first_pos);
   m_aoiView.set_lr(last_pos);

   m_atpTileSource->initialize();
}


ossimRefPtr<ossimImageChain>
AtpGenerator::constructChain(shared_ptr<Image> image,
                                 ossimRefPtr<ossimImageViewProjectionTransform>& ivt)
{
   static const char *MODULE = "AtpGenerator::constructChain()  ";
   AtpConfig &config = AtpConfig::instance();

   ossimImageHandlerRegistry *factory = ossimImageHandlerRegistry::instance();
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

   if (config.diagnosticLevel(5))
   {
      ossimIrect vuRect;
      handler->getImageGeometry()->getBoundingRect(vuRect);
      CINFO << MODULE << "m_aoiView: " << vuRect << endl;;
   }

   ossimRefPtr<ossimImageChain> chain = new ossimImageChain;
   chain->add(handler.get());

   // Add histogram (experimental)
   bool doHistogramStretch = config.getParameter("doHistogramStretch").asBool();
   if (doHistogramStretch)
   {
      ossimRefPtr<ossimHistogramRemapper> histo_remapper = new ossimHistogramRemapper();
      chain->add(histo_remapper.get());
      histo_remapper->setStretchMode(ossimHistogramRemapper::LINEAR_AUTO_MIN_MAX);
      ossimRefPtr<ossimMultiResLevelHistogram> histogram = handler->getImageHistogram();
      if (!histogram)
      {
         handler->buildHistogram();
         histogram = handler->getImageHistogram();
      }
      histo_remapper->setHistogram(histogram);

      // Need to select a band if multispectral:
      unsigned int numBands = handler->getNumberOfOutputBands();
      if (numBands > 1)
      {
         unsigned int band = image->getActiveBand();
         if ((band > numBands) || (band <= 0))
         {
            CINFO << MODULE << "Specified band (" << band << ") is outside allowed range (1 to "
                  << numBands
                  << "). Using default band 1 which may not be ideal." << endl;
            band = 1;
         }
         band--; // shift to zero-based
         ossimRefPtr<ossimBandSelector> band_selector = new ossimBandSelector();
         chain->add(band_selector.get());
         vector<ossim_uint32> bandList;
         bandList.push_back(band);
         band_selector->setOutputBandList(bandList);
      }
   }

   // Can only work in 8-bit radiometry:
   if (chain->getOutputScalarType() != OSSIM_UINT8)
   {
      ossimRefPtr<ossimScalarRemapper> remapper = new ossimScalarRemapper();
      chain->add(remapper.get());
      remapper->setOutputScalarType(OSSIM_UINT8);
   }

   // Finally, add a cache to the input chain:
   ossimRefPtr<ossimCacheTileSource> cache = new ossimCacheTileSource();
   chain->add(cache.get());

   // Check for specified GSD in config parameters (experimental):
   double viewGsd = config.getParameter("viewGSD").asFloat();

   // Set up a common view geometry, even if derived class works exclusively in image space.
   ossimRefPtr<ossimImageGeometry> image_geom = handler->getImageGeometry();
   if (!m_viewGeom)
   {
      ossimDpt gsd;
      if (!ossim::isnan(viewGsd) && (viewGsd != 0.0))
         gsd = ossimDpt(viewGsd, viewGsd);
      else
      {
         gsd = image_geom->getMetersPerPixel();
         if (gsd.x > gsd.y)
            gsd.y = gsd.x;
         else
            gsd.x = gsd.y;
      }
      ossimGpt proj_origin;
      image_geom->getTiePoint(proj_origin, false);
      ossimUtmProjection *proj = new ossimUtmProjection();
      proj->setOrigin(proj_origin);
      proj->setMetersPerPixel(gsd);
      proj->setUlTiePoints(proj_origin);
      m_viewGeom = new ossimImageGeometry(NULL, proj);
   }
   else
   {
      // View geometry was already set up by first image of the pair (REF). Only need to check the
      // GSD of the CMP image and adjust the view GSD to match if larger:
      ossimDpt cmpGsd(image_geom->getMetersPerPixel());
      if (cmpGsd.x > cmpGsd.y)
         cmpGsd.y = cmpGsd.x;
      else
         cmpGsd.x = cmpGsd.y;
      ossimDpt refGsd(m_viewGeom->getMetersPerPixel());
      if (cmpGsd.x > refGsd.x)
      {
         refGsd.x = cmpGsd.x;
         refGsd.y = cmpGsd.y;
         m_viewGeom->getAsMapProjection()->setMetersPerPixel(refGsd);
      }
   }

   ivt = new ossimImageViewProjectionTransform(image_geom.get(), m_viewGeom.get());

   if (m_algorithm == CROSSCORR)
   {
      // Need to add a renderer since correlations are done in view-space:
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
      ossimRefPtr<ossimCacheTileSource> cache = new ossimCacheTileSource();
      chain->add(cache.get());

      if (AtpConfig::instance().diagnosticLevel(3))
      {
         ossimDpt imagePt(1,1);
         ossimDpt viewPt, rtIpt;
         ivt->imageToView(imagePt, viewPt);
         ivt->viewToImage(viewPt, rtIpt);
         CINFO<<MODULE<<"\n  imagePt: "<<imagePt<<
              "\n  viewPt: "<<viewPt<<"\n  rtIpt: "<<rtIpt<<endl;
      }

   }
   return chain;
}

bool AtpGenerator::getValidVertices(ossimRefPtr<ossimImageChain> chain,
                                        ossimRefPtr<ossimImageViewProjectionTransform>& ivt,
                                        std::vector<ossimDpt>& validVertices)
{
   validVertices.clear();

   // Need to establish the valid image vertices for establishing overlap polygon. Compute in
   // image space then transform the vertices to view space:
   if (!chain)
      return false;
   ossimImageHandler *handler = dynamic_cast<ossimImageHandler *>(chain->getLastSource());
   if (!handler)
      return false;

   // Check if there is already a vertices file alongside the image:
   if (handler->openValidVertices())
   {
      vector<ossimIpt> iverts;
      handler->getValidImageVertices(iverts);
      for (const auto &vert : iverts)
         validVertices.emplace_back(vert);
   }

   if (validVertices.empty())
   {
      // If the projection is a sensor model, then assume that the four image-space coordinates are
      // valid image coordinates.
      ossimRefPtr<ossimImageGeometry> geom = handler->getImageGeometry();
      if (geom)
      {
         if (dynamic_cast<ossimSensorModel*>(geom->getProjection()))
         {
            ossimIrect rect (handler->getBoundingRect());
            validVertices.emplace_back(rect.ul());
            validVertices.emplace_back(rect.ur());
            validVertices.emplace_back(rect.lr());
            validVertices.emplace_back(rect.ll());
         }
      }

      if (validVertices.empty())
      {
         ossimFilename verticesFile(handler->createDefaultValidVerticesFilename());
         ossimRefPtr<ossimVertexExtractor> ve = new ossimVertexExtractor(handler);
         ve->setOutputName(verticesFile);
         ve->setAreaOfInterest(handler->getBoundingRect(0));
         ve->execute();
         const vector<ossimIpt> &iverts = ve->getVertices();
         for (const auto &vert : iverts)
            validVertices.emplace_back(vert);
      }
   }

   // The image vertices are necessarily in image space coordinates. Need to transform to a common
   // "view" space before determining the overlap polygon:
   ossimDpt vpt;
   for (auto &vertex : validVertices)
   {
      ivt->imageToView(vertex, vpt);
      vertex = vpt;
   }

   return true;
}

void AtpGenerator::writeTiePointList(ostream& out, const AtpList& tpList)
{
   if (tpList.empty())
   {
      out<<"\nTiepoint list is empty.\n"<<endl;
   }
   else
   {
      for (size_t i=0; i<tpList.size(); ++i)
      {
         out<<"\ntiepoint["<<i<<"]: "<<tpList[i]<<endl;
      }
   }
}

bool AtpGenerator::generateTiePointList(TiePointList& atpList)
{
   const char* MODULE = "AtpGenerator::generateTiePointList()  ";
   initialize();

   if (!m_refChain || !m_cmpChain || !m_atpTileSource)
      return false;

   AtpConfig& config = AtpConfig::instance();

   // Need to search for features in the REF tile first, then consider only those locations
   // for correlating. The search tiles were previously established in the base class initialization
   ossimRefPtr<ossimImageData> ref_tile = 0;
   ossim_uint32 tileId = 0;
   for (auto &searchTileRect : m_searchTileRects)
   {
      ref_tile = m_atpTileSource->getTile(searchTileRect);

      AtpList &tileATPs = m_atpTileSource->getTiePoints();
      if (tileATPs.empty() && config.diagnosticLevel(1))
      {
         CINFO << "\n" << MODULE << "No TPs found in tile." << endl;
         ++tileId;
         continue;
      }

      // Add remaining tile ATPs to master ATP list:
      if (!tileATPs.empty())
         atpList.insert(atpList.end(), tileATPs.begin(), tileATPs.end());

      ++tileId;
   }

   if (config.diagnosticLevel(1))
      CINFO<<"\n"<<MODULE<<"Total number of TPs found in pair = "<<atpList.size()<<endl;

   if (config.diagnosticLevel(3))
   {
      m_annotatedRefImage->write();
      if (config.diagnosticLevel(4))
         m_annotatedCmpImage->write();
   }
   return true;
}

void AtpGenerator::layoutSearchTileRects(ossimPolygon& intersectPoly)
{
   const char* MODULE = "AtpGenerator::layoutSearchTiles()  ";
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
      CINFO<<MODULE<<"Intersect polygon num vertices is less than 3! This should never happen."<<endl;
      return;
   }
   if (intersectPoly[0] == intersectPoly[lastVertex])
   {
      intersectPoly.removeVertex(lastVertex);
      if (lastVertex < 3)
      {
         CWARN<<MODULE<<"Intersect polygon num vertices is less than 3! This should never happen."<<endl;
         return;
      }
   }

   double intersectArea = fabs(intersectPoly.area());
   if (intersectArea < min_area)
   {
      CINFO<<MODULE<<"Intersect area too small"<<endl;
      return;
   }

   if (config.diagnosticLevel(3) && m_annotatedRefImage)
      m_annotatedRefImage->annotateOverlap(intersectPoly);

   // Prepare to layout evenly-spaced search tiles across overlap bounding rect:
   ossimIrect aoiRect;
   intersectPoly.getBoundingRect(aoiRect);
   int side_length = (int) config.getParameter("featureSearchTileSize").asUint();
   int dx, dy; // tile width and height
   int aoiWidth = aoiRect.width();
   int aoiHeight = aoiRect.height();

   // Determine optimal X and Y-direction space between tiles (actually between corresponding
   // UL corners)
   int gridSize = (int) config.getParameter("searchGridSize").asUint();
   if (gridSize == 0)
   {
      // Check for overlap requirement to account for sampling kernel width:
      int marginSize = (int) config.getParameter("featureSearchTileMargin").asUint();

      // No gridsize indicates contiguous tiles (full coverage, usually for DEM generation ATPs:
      dx = side_length-marginSize;
      dy = side_length-marginSize;
   }
   else
   {
      for (int nx=gridSize; nx>1; nx--)
      {
         dx = (int) floor(aoiWidth/nx);
         if (dx >= side_length)
            break;
      }
      for (int ny=gridSize; ny>1; ny--)
      {
         dy = (int) floor(aoiHeight/ny);
         if (dy >= side_length)
            break;
      }
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
      if(!intersectPoly.isRectWithin(*b))
         b = m_searchTileRects.erase(b);
      else
         b++;
   }

   // Don't bother with less than N boxes, just make the entire intersection a search patch:
   if (gridSize && (m_searchTileRects.size() < 4))
   {
      m_searchTileRects.clear();
      m_searchTileRects.push_back(m_aoiView);
   }

   if (config.diagnosticLevel(2))
   {
      for (int tileIdx=0; tileIdx<m_searchTileRects.size(); ++tileIdx)
         CINFO<<MODULE<<"m_searchTileRects["<<tileIdx<<"] = "<<m_searchTileRects[tileIdx]<<endl;

      if (config.diagnosticLevel(3) && m_annotatedRefImage)
         m_annotatedRefImage->annotateFeatureSearchTiles(m_searchTileRects);
   }
}


}
