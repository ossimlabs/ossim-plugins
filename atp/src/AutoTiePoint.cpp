//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#include "AutoTiePoint.h"
#include "AtpConfig.h"
#include "AtpGenerator.h"
#include "../AtpCommon.h"
#include <ossim/imaging/ossimImageRenderer.h>

using namespace std;
using namespace ossim;

namespace ATP
{
AutoTiePoint::AutoTiePoint()
: m_relativeError(0),
  m_generator(0)
{
}

AutoTiePoint::AutoTiePoint (const AutoTiePoint& copy)
: TiePoint (copy),
  m_generator(copy.m_generator),
  m_refViewPt(copy.m_refViewPt),
  m_cmpViewPt(copy.m_cmpViewPt),
  m_residual(copy.m_residual),
  m_relativeError(copy.m_relativeError)
{
   for (size_t i=0; i<copy.m_matchPoints.size(); ++i)
      m_matchPoints.push_back(copy.m_matchPoints[i]);
}

AutoTiePoint::AutoTiePoint(AtpGenerator* overlap, const std::string& id)
:  m_generator(overlap),
   m_relativeError(0)
{
   m_type = TiePoint::AUTO;
   setTiePointId(id);
   if (!m_generator)
   {
      CFATAL << "AutoTiePoint Constructor ERROR: Null overlap chain passed." << endl;
      return;
   }

   // Because the Image IDs are not unique, use the image filename as the ID:
   shared_ptr<Image> ref (new Image(m_generator->getRefImageID(), m_generator->getRefFilename()));
   shared_ptr<Image> cmp (new Image(m_generator->getCmpImageID(), m_generator->getCmpFilename()));
   m_images.push_back(ref);
   m_images.push_back(cmp);
}

AutoTiePoint::AutoTiePoint(const Json::Value& tp_json_node)
:  TiePoint(tp_json_node),
   m_relativeError(0),
   m_generator(0)
{
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
   m_generator->getRefIVT()->imageToView(imagePt, m_refViewPt);
}

void AutoTiePoint::setRefViewPt(const ossimDpt& viewPt)
{
   m_refViewPt = viewPt;
   ossimDpt imagePt;
   m_generator->getRefIVT()->viewToImage(viewPt, imagePt);
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

   auto geom = m_generator->getRefIVT()->getViewGeometry();
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

bool AutoTiePoint::bumpActiveMatch()
{
   m_matchPoints.erase(m_matchPoints.begin());
   if (m_matchPoints.empty())
      return false;

   recomputeResidual();
   return true;
}

bool AutoTiePoint::hasValidMatch() const
{
   return !m_matchPoints.empty();
}

void AutoTiePoint::addViewMatch(const ossimDpt& cmpViewPt, double confidenceValue)
{
   // Need to compute what the CMP image point is for the peak:
   ossimDpt cmpImagePt;
   m_generator->getCmpIVT()->viewToImage(cmpViewPt, cmpImagePt);

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
   if (!m_generator)
      m_residual.makeNan();

   if (m_imagePoints.size() < 2)
      return;

   // Since the REF input projection may have changed, recompute the REF view pt data member:
   m_generator->getRefIVT()->imageToView(m_imagePoints[0], m_refViewPt);

   // Establish the view-space coordinates of the peak given the peak's image point (in REF imae)
   // in order to compute residual:
   m_generator->getCmpIVT()->imageToView(m_imagePoints[1], m_cmpViewPt);

   m_residual = m_cmpViewPt - m_refViewPt;
}

bool AutoTiePoint::getRefGroundPoint(ossimGpt& gpt) const
{
   gpt.makeNan();
   if (m_refViewPt.isNan())
      return false;

   m_generator->getRefIVT()->getViewGeometry()->localToWorld(m_refViewPt, gpt);
   return true;
}


}

