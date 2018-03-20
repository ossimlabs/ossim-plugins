//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "AtpTileSource.h"
#include "../AtpCommon.h"
#include "AtpGenerator.h"
#include <ossim/imaging/ossimImageDataFactory.h>
#include <ossim/base/ossimVisitor.h>
#include <ossim/imaging/ossimImageRenderer.h>
#include <ossim/elevation/ossimElevManager.h>

using namespace std;

namespace ATP
{
double AtpTileSource::s_minVectorResDiff = 0;
double AtpTileSource::s_maxDiffRatio = 1.0;
double AtpTileSource::s_cosMaxAngleDiff = 0;
double AtpTileSource::s_maxPaxDev = 1.0;
double AtpTileSource::s_maxPaxPix = 2.0;
unsigned int AtpTileSource::s_minNumConsistent = 3;
unsigned int AtpTileSource::s_percentConsistent = 50;
unsigned int AtpTileSource::s_numTpsPerTile = 2;
unsigned int AtpTileSource::s_numFilterIterations = 0;
bool   AtpTileSource::s_useRasterMode = false;
bool   AtpTileSource::s_initialized = false;

AtpTileSource::AtpTileSource()
:  m_considerParallax (true)
{
}

AtpTileSource::AtpTileSource(ossimConnectableObject::ConnectableObjectList& inputSources)
:  ossimImageCombiner(inputSources),
   m_considerParallax (true)

{
}

AtpTileSource::AtpTileSource(AtpGenerator* generator)
:  m_generator(generator),
   m_considerParallax (true)
{
   theComputeFullResBoundsFlag = true;
   if (m_generator)
   {
      connectMyInputTo(0, m_generator->getRefChain().get());
      connectMyInputTo(1, m_generator->getCmpChain().get());
   }
   addListener((ossimConnectableObjectListener*)this);
   initialize();
}

void AtpTileSource::initialize()
{
   const char* MODULE = "AtpTileSource::initialize() -- ";
   if (!s_initialized)
      initializeStaticMembers();

   ossimImageCombiner::initialize();
   m_tiePoints.clear();
   if ( getNumberOfInputs() < 2)
      return;
}

void AtpTileSource::allocate()
{
   // The AtpTileSources cannot be relied on to return valid pixel data, since their main product
   // are the tie points, not pixels. However, the tile is used to communicate scalar type, rect,
   // and other tile metadata, so is still needed:
   m_tile = NULL;
   if ( getNumberOfInputs() > 1)
   {
      ossimImageSource* firstSource = dynamic_cast<ossimImageSource*>(theInputObjectList[0].get());
      m_tile = ossimImageDataFactory::instance()->create(this, firstSource);
      m_tile->initialize();
   }
}

void AtpTileSource::filterPoints()
{
   const char* MODULE = "AtpTileSource::filterPoints() -- ";
   AtpConfig& config = AtpConfig::instance();

   // First remove points with no matches:
   auto tiePoint = m_tiePoints.begin();
   while (tiePoint != m_tiePoints.end())
   {
      if ((*tiePoint) && (*tiePoint)->hasValidMatch())
         ++tiePoint;
      else
         tiePoint = m_tiePoints.erase(tiePoint);
   }

   // No filtering done for raster mode:
   if (s_useRasterMode)
      return;

   if (config.diagnosticLevel(3))
   {
      // Annotate red before filtering:
      m_generator->m_annotatedRefImage->annotateResiduals(m_tiePoints, 255, 0, 0);
      m_generator->m_annotatedCmpImage->annotateCorrelations(m_tiePoints, 255, 0, 0);
   }

   // Check for consistency check override
   if (s_minNumConsistent == 0)
      return;

   // Don't accept any if there are an insufficient number of neighbors:
   if (m_tiePoints.size() < s_minNumConsistent)
   {
      m_tiePoints.clear();
      return;
   }

   if (config.paramExists("considerParallax"))
      m_considerParallax = config.getParameter("considerParallax").asBool();
   if (m_considerParallax)
   {
      // If the perspective differences are small, fall back to non-parallax check:
      computeParallax();
      if ((fabs(m_paxInfo.dx_dH) < 1.0) && (fabs(m_paxInfo.dy_dH) < 1.0))
      {
         if (config.diagnosticLevel(5))
            CINFO << MODULE << "Insignificant parallax, falling back to non-parallax filtering."
                  << endl;
         filterWithoutParallax();
      }
      else
      {
         s_numFilterIterations = 0;
         filterWithParallax();
      }
   }
   else
      filterWithoutParallax();

   if (config.diagnosticLevel(3))
   {
      // Annotate yellow before pruning:
      m_generator->m_annotatedRefImage->annotateResiduals(m_tiePoints, 255, 255, 0);
      m_generator->m_annotatedCmpImage->annotateCorrelations(m_tiePoints, 255, 255, 0);
   }

   pruneList();

   if (config.diagnosticLevel(2))
   {
      CINFO<<MODULE<<"After filtering & pruning: num TPs in tile = "<<m_tiePoints.size()<<endl;
      if (config.diagnosticLevel(3))
      {
         // Annotate Green after accepting TP:
         m_generator->m_annotatedRefImage->annotateResiduals(m_tiePoints, 0, 255, 0);
         m_generator->m_annotatedCmpImage->annotateCorrelations(m_tiePoints, 0, 255, 0);
         //m_annotatedRefImage->annotateTPIDs(m_tiePoints, 180, 180, 0);
      }
   }
}

void AtpTileSource::filterWithParallax()
{
   // May be called recursively, limited by s_numFilterIterations < 5

   const char* MODULE = "AtpTileSource::filterWithParallax()  ";
   AtpConfig& config = AtpConfig::instance();
   if (m_tiePoints.size() < 2)
   {
      m_tiePoints.clear();
      return;
   }

   computeParallaxStatistics();

   // This is where recursion is stopped:
   if ((s_numFilterIterations == 5) && (m_paxInfo.maxDistance > s_maxPaxPix))
   {
      m_tiePoints.clear();
      if (config.diagnosticLevel(5))
      {
         CINFO << MODULE << "Could not establish good parallax statistics for this set. "
               << "Removing all points for this tile." << endl;
      }
      return;
   }

   // Loop to eliminate bad peaks and inconsistent ATPs in tile:
   ossimDpt residual;
   bool statsChanged = false;
   auto tiePoint = m_tiePoints.begin();
   while (tiePoint != m_tiePoints.end())
   {
      // Check point's active peak against the set of residuals along parallax:
      auto entry = m_paxInfo.distanceMap.find((*tiePoint)->getTiePointId());
      if (entry == m_paxInfo.distanceMap.end())
      {  // Should never happen
         tiePoint = m_tiePoints.erase(tiePoint);
         continue;
      }
      double distance = entry->second;

      // Loop to find a match in this ATP that is consistent.
      while (distance > m_paxInfo.maxDistance)
      {
         // This residual is outside the acceptable limit, try the next match in the tiepoint or
         // remove it if it has no more peaks.
         statsChanged = true;
         if (!(*tiePoint)->bumpActiveMatch())
         {
            if (config.diagnosticLevel(5))
            {
               CINFO << MODULE << "Removing TP " << (*tiePoint)->getTiePointId() << endl;
               CINFO << "   distance: " << distance << ",  maxDistance: " << m_paxInfo.maxDistance << endl;
            }
            tiePoint = m_tiePoints.erase(tiePoint);
            break;
         }
         else if (config.diagnosticLevel(5))
         {
            CINFO << MODULE << "Bumping active match for TP " << (*tiePoint)->getTiePointId()
                  << endl;
            CINFO << "   distance: " << distance << ",  maxDistance: " << m_paxInfo.maxDistance << endl;
         }

         // A new peak is being used, will need to recompute statistics later.
         (*tiePoint)->getVectorResidual(residual);
         if (m_paxInfo.dx_dH == 0.0)
            distance = residual.x;
         else if (m_paxInfo.dx_dH == 0.0)
            distance = residual.y;
         else
         {
            distance = fabs(m_paxInfo.parallaxOffset + m_paxInfo.parallaxSlope * residual.x -
                            residual.y) / m_paxInfo.denom;
         }
      }
      if (distance <= m_paxInfo.maxDistance)
      {
         // The match was deemed consistent. proceed to next point:
         ++tiePoint;
      }
   }

   if (statsChanged)
   {
      // Recursive call to recompute statistics and eliminate suspect points
      s_numFilterIterations++;
      if (config.diagnosticLevel(5))
      {
         CINFO << MODULE << "Iterating filterWithParallax after ATP list change (N = "
               << s_numFilterIterations << ")."<< endl;
      }
      filterWithParallax();
   }
}

void AtpTileSource::computeParallax()
{
   // Use the point at tile center to establish reference ground point at two different elevations:
   static const double DELTA_HEIGHT = 500.0;
   ossimDpt vpt, ipt0, ipt1;
   ossimGpt gpt0, gpt1;

   // Possible debug mode will not have a tile assigned:
   if (!m_tile || (m_tile->getDataObjectStatus() == OSSIM_STATUS_UNKNOWN))
       m_tiePoints[0]->getRefViewPoint(vpt);
   else
      m_tile->getImageRectangle().getCenter(vpt);
   ossimImageGeometry *geom = m_generator->m_refIVT->getViewGeometry();
   geom->localToWorld(vpt, gpt0);
   if (gpt0.isHgtNan())
      gpt0.hgt = ossimElevManager::instance()->getHeightAboveEllipsoid(gpt0);
   gpt1 = gpt0;
   gpt1.hgt += DELTA_HEIGHT;

   // Determine dx/dh and dy/dh for ref image:
   geom = m_generator->m_refIVT->getImageGeometry();
   geom->worldToLocal(gpt0, ipt0);
   geom->worldToLocal(gpt1, ipt1);
   double dx_ref = (ipt1.x - ipt0.x);
   double dy_ref = (ipt1.y - ipt0.y);

   // Determine dx/dh and dy/dh for cmp image:
   geom = m_generator->m_cmpIVT->getImageGeometry();
   geom->worldToLocal(gpt0, ipt0);
   geom->worldToLocal(gpt1, ipt1);
   double dx_cmp = (ipt1.x - ipt0.x);
   double dy_cmp = (ipt1.y - ipt0.y);

   // Compute parallax direction REF-->CMP
   m_paxInfo.dx_dH = dx_cmp - dx_ref;
   m_paxInfo.dy_dH = dy_cmp - dy_ref;
   m_paxInfo.parallaxSlope = m_paxInfo.dy_dH / m_paxInfo.dx_dH;
   m_paxInfo.denom = sqrt(m_paxInfo.parallaxSlope*m_paxInfo.parallaxSlope + 1.0);

   CINFO << "### dx_dH: " << m_paxInfo.dx_dH << endl; // TODO REMOVE
   CINFO << "### dy_dH: " << m_paxInfo.dy_dH << endl; // TODO REMOVE
   CINFO << "### parallaxSlope: " << m_paxInfo.parallaxSlope << endl; // TODO REMOVE
}


void AtpTileSource::computeParallaxStatistics()
{
   // Determining the parallax direction (slope) is easy. It is the offset (y-intercept) that
   // depends on the collection of residuals.
   // Use the point at tile center to establish dx/dH and dy/dH in view-space:
   // Compute parallax slope and intercept:
   ossimDpt residual;
   if (m_paxInfo.dx_dH != 0.0)
   {
      // Compute parallax intercept using collection of all primary matches:
      double sum_c_i = 0;
      for (auto &tiePoint : m_tiePoints)
      {
         tiePoint->getVectorResidual(residual);
         sum_c_i += residual.y - m_paxInfo.parallaxSlope * residual.x;
      }
      m_paxInfo.parallaxOffset = sum_c_i/m_tiePoints.size();
   }

   // Compute statistics of residual vector distribution, saving individual distance samples:
   m_paxInfo.distanceMap.clear();
   double distance, sumDistance2=0.0;
   for (auto &tiePoint : m_tiePoints)
   {
      tiePoint->getVectorResidual(residual);
      if (m_paxInfo.dx_dH == 0)
         distance = residual.x;
      else if (m_paxInfo.dy_dH == 0)
         distance = residual.y;
      else
      {
         distance = fabs(m_paxInfo.parallaxOffset + m_paxInfo.parallaxSlope*residual.x - residual.y)/m_paxInfo.denom;
      }
      sumDistance2 += distance*distance;
      m_paxInfo.distanceMap.emplace(tiePoint->getTiePointId(), distance);
      CINFO << "### TP: "<<tiePoint->getTiePointId()<<"  distance: " << distance << endl; // TODO REMOVE

   }
   double sigmaDistance = sqrt(sumDistance2/(m_tiePoints.size()-1));
   m_paxInfo.maxDistance = s_maxPaxDev*sigmaDistance;
   if (m_paxInfo.maxDistance < s_maxPaxPix)
      m_paxInfo.maxDistance = s_maxPaxPix;

   CINFO << "### sigmaDistance: " << sigmaDistance << endl; // TODO REMOVE
   CINFO << "### maxDistance: " << m_paxInfo.maxDistance << endl; // TODO REMOVE
}

void AtpTileSource::filterWithoutParallax()
{
   const char* MODULE = "AtpTileSource::filterWithoutParallax()  ";
   AtpConfig& config = AtpConfig::instance();

   // Loop to eliminate bad peaks and inconsistent ATPs in tile:
   auto tiePoint = m_tiePoints.begin();
   while (tiePoint != m_tiePoints.end())
   {
      // The "tiepoint" may not have any matches -- it may be just a feature:
      ossimDpt r_atp, r_nbr;
      (*tiePoint)->getVectorResidual(r_atp);

      if (config.diagnosticLevel(5))
      {
         ossimDpt refPt;
         (*tiePoint)->getRefImagePoint(refPt);
         CINFO<<"AutoTiePoint::checkConsistency() -- Processing TP "<<(*tiePoint)->getTiePointId()
              <<" at REF image pt: ("<<refPt.x<<", "<<refPt.y<<")"<<endl;
      }

      // Loop over all of the TPs neighbors to count the number that are consistent:
      unsigned int num_consistent_neighbors = 0;
      for(auto &neighbor : m_tiePoints)
      {
         if ((*tiePoint)->getTiePointId() == neighbor->getTiePointId())
            continue;

         // Check for simple minimum vector difference:
         neighbor->getVectorResidual(r_nbr);
         double diff = (r_nbr - r_atp).length();
         if (diff < s_minVectorResDiff)
         {
            num_consistent_neighbors++;;
            continue;
         }

         // Compute angle and magnitude ratio between residuals. Note no check for valid peak:
         double cos_theta=1.0, mag_ratio=0;
         double r0 = r_atp.length();
         double r1 = r_nbr.length();
         mag_ratio = 2.0*fabs(r1-r0)/(r0+r1);
         if ((r1 > 0.0) && (r0 > 0.0))
            cos_theta = (r_atp.x*r_nbr.x + r_atp.y*r_nbr.y)/(r0*r1);

         // Test for consistency with this neighbor's peak:
         if ((mag_ratio < s_maxDiffRatio) && (cos_theta > s_cosMaxAngleDiff))
            num_consistent_neighbors++;;
      }

      // Decide if this peak was good or out there:
      int minConsistent = (int) ceil(s_percentConsistent*(m_tiePoints.size()-1)/100);
      if (minConsistent < (int)s_minNumConsistent)
         minConsistent = s_minNumConsistent;
      if (num_consistent_neighbors < minConsistent)
      {
         // Peak was bad, fetch next strongest match or exit loop if no more available:
         if (!(*tiePoint)->bumpActiveMatch())
         {
            if (config.diagnosticLevel(5))
               CINFO << MODULE << "Removing TP " << (*tiePoint)->getTiePointId() << endl;
            tiePoint = m_tiePoints.erase(tiePoint);
            if (m_tiePoints.empty())
               break;
         }
      }
      else
      {
         // Match is consistent, keep the tiepoint in the list an advance to the next tiepoint
         tiePoint++;
      }
   }
}


void AtpTileSource::pruneList()
{
   const char* MODULE = "AtpTileSource::pruneList()  ";
   AtpConfig &config =  AtpConfig::instance();

   // Use a map to sort by confidence measure:
   multimap<double, shared_ptr<AutoTiePoint> > atpMap;
   auto atp = m_tiePoints.begin();
   double confidence;
   while (atp != m_tiePoints.end())
   {
      if (!(*atp)) // Should never happen
      {
         CWARN<<MODULE<<"WARNING: Null AutoTiePoint encountred in list. Skipping point."<<endl;
         ++atp;
         continue;
      }

      // The "tiepoint" may not have any matches -- it may be just a feature:
      bool hasValidMatch = (*atp)->getConfidenceMeasure(confidence);
      if (hasValidMatch)
         atpMap.emplace(1.0/confidence, *atp);

      atp++;
   }

   // Skim off the top best:
   m_tiePoints.clear();
   multimap<double, shared_ptr<AutoTiePoint> >::iterator tp = atpMap.begin();
   while (tp != atpMap.end())
   {
      m_tiePoints.push_back(tp->second);
      if (m_tiePoints.size() == s_numTpsPerTile)
         break;
      tp++;
   }
}

void AtpTileSource::initializeStaticMembers()
{
   static const char* MODULE = "AtpTileSource::initializeStaticMembers() -- ";

   AtpConfig& config = AtpConfig::instance();
   s_maxDiffRatio = config.getParameter("maxResMagDiffRatio").asFloat();
   if (ossim::isnan(s_maxDiffRatio) || (s_maxDiffRatio == 0))
   {
      CWARN<<MODULE<<"Bad or missing parameter in config: "
              "maxResMagDiffRatio = "<<s_maxDiffRatio<<". Defaulting to 0.1."<<endl;
      s_maxDiffRatio = 0.1;
   }

   double theta = config.getParameter("maxResAngleDiff").asFloat();
   if (ossim::isnan(theta) || (theta <= 0))
   {
      CWARN<<MODULE<<"Bad or missing parameter in config: "
              "maxAngleDiff = "<<theta<<". Defaulting to 10 deg."<<endl;
      theta = 10.0;
   }
   s_cosMaxAngleDiff = ossim::cosd(theta);

   s_minVectorResDiff = config.getParameter("minVectorResDiff").asFloat();
   if (ossim::isnan(s_minVectorResDiff) || (s_minVectorResDiff < 1.0))
   {
      CWARN<<MODULE<<"Bad or missing parameter in config: "
              "minVectorResDiff = "<<s_minVectorResDiff<<". Defaulting to 1.0."<<endl;
      s_minVectorResDiff = 1.0;
   }

   s_minNumConsistent = config.getParameter("minNumConsistentNeighbors").asUint();
   if (s_minNumConsistent == 0)
   {
      CWARN<<MODULE<<"Bad or missing parameter in config: "
              "minNumConsistentNeighbors = "<<s_minNumConsistent<<". Defaulting to 3."<<endl;
      s_minNumConsistent = 3.0;
   }

   s_percentConsistent = config.getParameter("percentConsistentThreshold").asUint();
   if (s_percentConsistent == 0)
   {
      CWARN<<MODULE<<"Bad or missing parameter in config: "
              "percentConsistentThreshold = "<<s_percentConsistent<<". Defaulting to 50."<<endl;
      s_percentConsistent = 50;
   }

   s_numTpsPerTile = config.getParameter("numTiePointsPerTile").asUint();
   if (s_numTpsPerTile == 0)
   {
      CWARN<<MODULE<<"Bad or missing parameter in config: "
              "numTiePointsPerTile = "<<s_numTpsPerTile<<". Defaulting to 2."<<endl;
      s_numTpsPerTile = 2;
   }

   s_maxPaxDev = config.getParameter("maxResParallaxDev").asFloat();
   if (ossim::isnan(s_maxPaxDev) || (s_maxPaxDev <= 0))
   {
      CWARN<<MODULE<<"Bad or missing parameter in config: "
              "maxResParallaxDev = "<<s_maxPaxDev<<". Defaulting to 1.0."<<endl;
      s_maxPaxDev = 1.0;
   }

   s_maxPaxPix = (double) config.getParameter("maxResParallaxPix").asUint();
   if (ossim::isnan(s_maxPaxPix) || (s_maxPaxPix <= 0))
   {
      CWARN<<MODULE<<"Bad or missing parameter in config: "
              "maxResParallaxPix = "<<s_maxPaxPix<<". Defaulting to 1.0."<<endl;
      s_maxPaxPix = 2.0;
   }

   if (config.paramExists("useRasterMode"))
      s_useRasterMode = config.getParameter("useRasterMode").asBool();

   s_initialized = true;
}

void AtpTileSource::setTiePoints(ossim::TiePointList& tpList)
{
   // Engineering code for manually specifying TPs to test consistency checking
   string id;
   ossimDpt rip, cip;
   NEWMAT::SymmetricMatrix cov;
   m_tiePoints.clear();
   for (auto &tp : tpList)
   {
      shared_ptr<AutoTiePoint> atp (new AutoTiePoint(m_generator, tp->getTiePointId()));
      tp->getImagePoint(0,id,rip,cov);
      tp->getImagePoint(1,id,cip,cov);
      atp->setRefImagePt(rip);
      atp->addImageMatch(cip);
      m_tiePoints.emplace_back(atp);
   }
}

}

