//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#include "AutoTiePoint.h"
#include "AtpConfig.h"
#include "AtpTileSource.h"
#include "../AtpCommon.h"
#include <ossim/imaging/ossimImageRenderer.h>

using namespace std;
using namespace ossim;

namespace ATP
{
bool   AutoTiePoint::s_initialized = false;
double AutoTiePoint::s_maxDiffRatio = 1.0;
double AutoTiePoint::s_cosMaxAngleDiff = 0;
double AutoTiePoint::s_minVectorResDiff = 0;
unsigned int AutoTiePoint::s_minNumConsistent = 3;

AutoTiePoint::AutoTiePoint()
: m_relativeError(0),
  m_overlap(0)
{
   if (!s_initialized)
      initializeStaticMembers();
}

AutoTiePoint::AutoTiePoint (const AutoTiePoint& copy)
: TiePoint (copy),
  m_overlap(copy.m_overlap),
  m_refViewPt(copy.m_refViewPt),
  m_cmpViewPt(copy.m_cmpViewPt),
  m_residual(copy.m_residual),
  m_relativeError(copy.m_relativeError)
{
   for (size_t i=0; i<copy.m_matchPoints.size(); ++i)
      m_matchPoints.push_back(copy.m_matchPoints[i]);
}

AutoTiePoint::AutoTiePoint(AtpTileSource* overlap, const std::string& id)
:  m_overlap(overlap),
   m_relativeError(0)
{
   if (!s_initialized)
      initializeStaticMembers();

   m_type = TiePoint::AUTO;
   setTiePointId(id);
   if (!m_overlap)
   {
      CFATAL << "AutoTiePoint Constructor ERROR: Null overlap chain passed." << endl;
      return;
   }

   // Because the Image IDs are not unique, use the image filename as the ID:
   shared_ptr<Image> ref (new Image(m_overlap->getRefImageID(), m_overlap->getRefFilename()));
   shared_ptr<Image> cmp (new Image(m_overlap->getCmpImageID(), m_overlap->getCmpFilename()));
   m_images.push_back(ref);
   m_images.push_back(cmp);
}

AutoTiePoint::AutoTiePoint(const Json::Value& tp_json_node)
:  TiePoint(tp_json_node),
   m_relativeError(0),
   m_overlap(0)
{
   if (!s_initialized)
      initializeStaticMembers();
}

AutoTiePoint::~AutoTiePoint()
{
   m_matchPoints.clear();
}

bool AutoTiePoint::getVectorResidual(ossimDpt& residual) const
{
   residual = m_residual;
   return true;
}

bool AutoTiePoint::getCmpImagePoint(ossimDpt& image_point) const
{
   if ((m_imagePoints.size() < 2) || m_imagePoints[1].isNan())
      return false;
   image_point = m_imagePoints[1];
   return true;
}

bool AutoTiePoint::getRefImagePoint(ossimDpt& image_point) const
{
   if (m_imagePoints.empty() || m_imagePoints[0].isNan())
      return false;
   image_point = m_imagePoints[0];
   return true;
}

void AutoTiePoint::setRefImagePt(const ossimDpt& imagePt)
{
   if (m_imagePoints.empty())
      m_imagePoints.push_back(imagePt);
   else
      m_imagePoints[0] = imagePt;
   m_overlap->getRefIVT()->imageToView(imagePt, m_refViewPt);
}

void AutoTiePoint::setRefViewPt(const ossimDpt& viewPt)
{
   m_refViewPt = viewPt;
   ossimDpt imagePt;
   m_overlap->getRefIVT()->viewToImage(viewPt, imagePt);
   if (m_imagePoints.empty())
      m_imagePoints.push_back(imagePt);
   else
      m_imagePoints[0] = imagePt;
}

void AutoTiePoint::saveJSON(Json::Value& json_node) const
{
   TiePoint::saveJSON(json_node);// ID
   if (m_matchPoints.empty())
      return;
   json_node["confidence"] = m_matchPoints[0].getConfidenceMeasure();

   auto geom = m_overlap->getRefIVT()->getViewGeometry();
   if (geom)
   {
      ossimGpt refGpt, cmpGpt;
      geom->localToWorld(m_refViewPt, refGpt);
      geom->localToWorld(m_cmpViewPt, cmpGpt);
      Json::Value residual;
      residual["range"] = refGpt.distanceTo(cmpGpt);
      residual["azimuth"] = refGpt.azimuthTo(cmpGpt);
      json_node["residual"] = residual;
   }
}

ostream& AutoTiePoint::print(std::ostream& out) const
{
   double azimuth = atan2(m_residual.x, - m_residual.y)*DEG_PER_RAD;
   if (azimuth < 0.0)
      azimuth += 360.0;

   out << "\nDump of AutoTiePoint ID = " << m_tiePointId;
   out << "\n  refImgPt    = " << m_imagePoints[0];
   out << "\n  residual (p) = " << m_residual;
   out << "\n  azimuth deg  = " << azimuth;
   TiePoint::print(out);
   if (m_matchPoints.size() == 0)
   {
      out<<"\n  No matches.";
   }
   else
   {
      out<<"\n  Matches:";
      for (size_t i=0; i<m_matchPoints.size(); ++i)
         out<<"\n    "<<m_matchPoints[i];
   }

   return out;
}

void AutoTiePoint::addImageMatch(const ossimDpt& cmpImagePt, double confidenceValue)
{
   // Check if we will be inserting in first position. If so, need to set m_cmpViewPt and
   // m_residual data  members to this (active) active peak info:
   if (m_matchPoints.size() == 0)
   {
      if (m_imagePoints.empty())
      {
         ossimDpt nanRefPt;
         nanRefPt.makeNan();
         m_imagePoints.push_back(nanRefPt);
      }
      if (m_imagePoints.size() == 1)
         m_imagePoints.push_back(cmpImagePt);
      else
         m_imagePoints[1] = cmpImagePt;

      recomputeResidual();
   }

   // Instantiate a new peak. Insert this peak into the map, where it is ordered by peak value:
   m_matchPoints.emplace_back(MatchPoint(cmpImagePt, confidenceValue, m_residual));
}

bool AutoTiePoint::checkConsistency(const AtpList& atpList)
{
   AtpConfig& config = AtpConfig::instance();
   ossimDpt R1;
   unsigned int num_consistent_neighbors = 0;

   if (config.diagnosticLevel(5))
   {
      CINFO<<"AutoTiePoint::checkConsistency() -- Processing TP "<<m_tiePointId
      <<" at REF image pt: ("<<m_imagePoints[0].x<<", "<<m_imagePoints[0].y<<")"<<endl;
   }

   // Loop over all of the TPs neighbors:
   AtpList::const_iterator neighbor = atpList.begin();
   AutoTiePoint* atp = 0;
   for(;neighbor!=atpList.end();++neighbor)
   {
      atp = neighbor->get();
      if (!atp || (atp == this))
         continue;

      // APC is not enabled, so simply check against neighbor's active (primary) peak:
      R1 = atp->m_residual;
      if (residualsAreConsistent(R1))
         num_consistent_neighbors++;
   }

   // Decide if this peak was good or out there:
   unsigned int percentConsistentThreshold =
         config.getParameter("percentConsistentThreshold").asUint();
   unsigned int p = (unsigned int)
         ceil(percentConsistentThreshold*(atpList.size()-1)/100);
   if (p == 0)
      p = 1;
   unsigned int min_num_consistent_neighbors = (s_minNumConsistent > p ? s_minNumConsistent : p);
   if (num_consistent_neighbors < min_num_consistent_neighbors)
      return false;

   return true;
}

bool AutoTiePoint::getConfidenceMeasure(double &confidence) const
{
   // Check for empty list or if we are at the end of the list (no points remaining):
   if ((!hasValidMatch()))
   {
      confidence = 0;
      return false;
   }

   // The first peak in the list is always the best peak:
   confidence = m_matchPoints.begin()->getConfidenceMeasure();
   return true;
}

bool AutoTiePoint::findBestConsistentMatch(AtpList& atpList)
{
   bool changed = false;

   int num_neighbors = atpList.size()-1;
   if (num_neighbors <= 0)
   {
      m_matchPoints.clear();
      return true;
   }

   while (m_matchPoints.size())
   {
      // Check if this TP peak is consistent with at least one nearby TP:
      // All the work is done by checkConsistency(). Returns TRUE if consistent:
      if (checkConsistency(atpList))
         break;

      // Bad peak detected, remove it from the list so that next one becomes active:
      m_matchPoints.erase(m_matchPoints.begin());
      changed=true;
      if (m_matchPoints.size() != 0)
         recomputeResidual();
   }

   // We are required to use all neighbors available for consistency check, even those considered
   // invalid by prior consistency checks. This prevents lone wildpoints from surviving the prune
   // due to surrounding wildpoints being eliminated first. However, now that we are done referencing
   // these neighbors, we can prune the list down to consider only active TPs in the next consistency
   // check for this TP:
//   AtpList::iterator neighbor = atpList.begin();
//   AutoTiePoint* atp = 0;
//   while (neighbor != ctpList.end())
//   {
//      atp = neighbor->get();
//      if (atp && (atp != this) && !atp->hasValidMatch())
//         neighbor = atpList.erase(neighbor);
//      else
//         ++neighbor;
//   }

   return changed;
}

bool AutoTiePoint::hasValidMatch() const
{
   return !m_matchPoints.empty();
}

bool AutoTiePoint::residualsAreConsistent(const ossimDpt& R1)
{
   // Check for simple minimum vector difference:
   double diff = (m_residual-R1).length();
   if (diff < s_minVectorResDiff)
      return true;

   double cos_theta=1.0, mag_ratio=0;

   // Compute angle and magnitude ratio between residuals. Note no check for valid peak:
   double r0 = m_residual.length();
   double r1 = R1.length();
   mag_ratio = 2.0*fabs(r1-r0)/(r0+r1);
   if ((r1 > 0.0) && (r0 > 0.0))
      cos_theta = (m_residual.x*R1.x + m_residual.y*R1.y)/(r0*r1);

   // Test for consistency with this neighbor's peak:
   if ((mag_ratio < s_maxDiffRatio) && (cos_theta > s_cosMaxAngleDiff))
      return true;

   return false;
}

void AutoTiePoint::initializeStaticMembers()
{
   AtpConfig& config = AtpConfig::instance();
   s_maxDiffRatio = config.getParameter("maxResMagDiffRatio").asFloat();
   if (ossim::isnan(s_maxDiffRatio) || (s_maxDiffRatio == 0))
   {
      CWARN<<"AutoTiePoint::residualsAreConsistent() -- Bad or missing parameter in config: "
            "maxResMagDiffRatio = "<<s_maxDiffRatio<<". Defaulting to 0.1."<<endl;
      s_maxDiffRatio = 0.1;
   }

   double theta = config.getParameter("maxResAngleDiff").asFloat();
   if (ossim::isnan(theta) || (theta <= 0))
   {
      CWARN<<"AutoTiePoint::residualsAreConsistent() -- Bad or missing parameter in config: "
            "maxAngleDiff = "<<theta<<". Defaulting to 10 deg."<<endl;
      theta = 10.0;
   }
   s_cosMaxAngleDiff = ossim::cosd(theta);

   s_minVectorResDiff = config.getParameter("minVectorResDiff").asFloat();
   if (ossim::isnan(s_minVectorResDiff) || (s_minVectorResDiff < 1.0))
   {
      CWARN<<"AutoTiePoint::residualsAreConsistent() -- Bad or missing parameter in config: "
            "minVectorResDiff = "<<s_minVectorResDiff<<". Defaulting to 1.0."<<endl;
      s_minVectorResDiff = 1.0;
   }

   s_minNumConsistent = config.getParameter("minNumConsistentNeighbors").asUint();
   if (s_minNumConsistent == 0)
   {
      CWARN<<"AutoTiePoint::residualsAreConsistent() -- Bad or missing parameter in config: "
              "minNumConsistentNeighbors = "<<s_minNumConsistent<<". Defaulting to 3.0."<<endl;
      s_minNumConsistent = 3.0;
   }

   s_initialized = true;
}

void AutoTiePoint::addViewMatch(const ossimDpt& cmpViewPt, double confidenceValue)
{
   // Need to compute what the CMP image point is for the peak:
   ossimDpt cmpImagePt;
   m_overlap->getCmpIVT()->viewToImage(cmpViewPt, cmpImagePt);

   // Check if we will be inserting in first position. If so, need to set m_cmpViewPt and
   // m_residual data  members to this (active) active peak info:
   if (m_matchPoints.size() == 0)
      m_cmpViewPt = cmpViewPt;

   addImageMatch(cmpImagePt, confidenceValue);
}

bool AutoTiePoint::getRefViewPoint(ossimDpt& view_point) const
{
   view_point = m_refViewPt;
   if (m_refViewPt.isNan())
      return false;
   return true;
}

bool AutoTiePoint::getCmpViewPoint(ossimDpt& view_point, unsigned int peak_idx) const
{
   if (peak_idx == 0)
   {
      view_point = m_cmpViewPt; // last known point, may be invalid.
      return true;
   }

   return false;
}

void AutoTiePoint::recomputeResidual()
{
   if (!m_overlap)
      m_residual.makeNan();

   if (m_imagePoints.size() < 2)
      return;

   // Since the REF input projection may have changed, recompute the REF view pt data member:
   m_overlap->getRefIVT()->imageToView(m_imagePoints[0], m_refViewPt);

   // Establish the view-space coordinates of the peak given the peak's image point (in REF imae)
   // in order to compute residual:
   m_overlap->getCmpIVT()->imageToView(m_imagePoints[1], m_cmpViewPt);

   m_residual = m_cmpViewPt - m_refViewPt;
}

bool AutoTiePoint::getRefGroundPoint(ossimGpt& gpt) const
{
   gpt.makeNan();
   if (m_refViewPt.isNan())
      return false;

   m_overlap->getRefIVT()->getViewGeometry()->localToWorld(m_refViewPt, gpt);
   return true;
}


}

