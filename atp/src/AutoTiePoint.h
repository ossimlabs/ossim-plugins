//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#pragma once

#include <vector>
#include <memory>
#include <string>
#include <ossim/base/ossimDpt.h>
#include <ossim/imaging/ossimImageSource.h>
#include <ossim/imaging/ossimImageGeometry.h>
#include <ossim/base/ossimIrect.h>
#include <TiePoint.h>
#include <MatchPoint.h>


namespace ATP
{
class AtpTileSource;

/**
 * Base class for all automatic tiepoints. These differ from tiepoint base class in that it is
 * guarantied to represent a single image pair. Therefore, residuals between the pair can
 * be computed without referencing image ids, etc. TODO: Eventually need to generalize to permit
 * N-way auto-tiepoints, at which point it should merge with TiePoint base class.
 */
class AutoTiePoint : public TiePoint
{
public:
   AutoTiePoint ();

   AutoTiePoint (const AutoTiePoint& copy_me);

   /**
    * Constructs given two images and a point on the REF image. The matching is still to be
    * established.
    * @param overlap The combiner source representing the overlap between REF and CMP images.
    * @id Tie point ID
    */
   AutoTiePoint(AtpTileSource* overlap, const std::string& id);

   /**
   * Creates new tiepoint from JSON object formatted as:
   * {
   *    "id": <unsigned int>, // may be ignored if not valid
   *    "type": "M"|"A"|"G"  For “manual”, “auto”, or “GCP-associated”
   *    "imagePoints: [
   *        {
   *           "imageId": <string>,
   *           "x": <double>,
   *           "y": <double>,
   *           "covariance": [ cxx, cyy, cxy ],
   *           "gcpId": <string> Only if associated with a GCP
   *        }
   *    ]
   * }
   */
   AutoTiePoint(const Json::Value& tp_json_node);

   virtual ~AutoTiePoint();

   /** Sets the reference image's image point (in image space) for this tiepoint */
   void setRefImagePt(const ossimDpt& imagePt);

   /** Sets the reference image's view point (in view space) for this tiepoint */
   void setRefViewPt(const ossimDpt& viewPt);

   //! Returns the vector residual (CMP-REF) of current active peak (in view space) via argument. 
   //! Returns true if valid.
   bool getVectorResidual(ossimDpt& residual) const;

   //! Returns the center location of the reference patch (the feature) in image-space. Returns
   //! false if point not yet defined. Note that there not be any valid peak.
   bool getRefImagePoint(ossimDpt& image_pt) const;

   //! Fetches the comparison image point for current best peak. Returns false if no valid peak
   bool getCmpImagePoint(ossimDpt& image_pt) const;

   //! Stream output dump of object.
   virtual std::ostream& print(std::ostream& out) const;

   //! Access to peaks list for GUI debug display.
   std::vector<MatchPoint>& matchPoints() { return m_matchPoints; }

   //! Inserts match in image space by confidence in descending order.
   void addImageMatch(const ossimDpt& cmpPt, double confidenceValue=1.0);

   //! Inserts match in image space by confidence in descending order.
   void addViewMatch(const ossimDpt& cmpViewPt, double confidenceValue=1.0);

   //! Returns the total number of peaks in the list.
   unsigned int getNumMatches() const { return (unsigned int) m_matchPoints.size(); }

   //! Returns true if there is a valid peak in the list.
   bool hasValidMatch() const;

   //! Returns the correlation value of current active peak via argument. Returns true if valid.
   bool getConfidenceMeasure(double &confidence) const;

   //! The peaks are reviewed for inconsistencies. If the current peak is determined inconsistent,
   //! the next best peak is enabled. Returns true if peak was changed.
   bool findBestConsistentMatch(std::vector< std::shared_ptr<AutoTiePoint> >& neighborhoodAtpList);

   //! Returns the center location of the reference patch (the feature) in view-space. Returns
   //! false if point not yet defined. Note that there may not be any valid peak.
   bool getRefViewPoint (ossimDpt& view_pt)  const;

   //! Returns the center location of the reference patch (the feature) in geographic. Returns
   //! false if point not yet defined. Note that there may not be any valid peak.
   bool getRefGroundPoint(ossimGpt& gpt) const;

   //! Fetches the comparison view point for current best peak, or for the peak indicated if provided.
   //! Returns false if no valid peak.
   bool getCmpViewPoint (ossimDpt& view_pt, unsigned int peak_idx=0)  const;

   /*
   * Refer to <a href="https://docs.google.com/document/d/1DXekmYm7wyo-uveM7mEu80Q7hQv40fYbtwZq-g0uKBs/edit?usp=sharing">3DISA API document</a>
   * for JSON format used.
   */
   virtual void saveJSON(Json::Value& json) const;

protected:
   //! Compares the residual against existing neighbors. Typically this will include all candidate
   //! tiepoints in the same tile as this (and can include this). TODO: The consistency check should
   //! consider distance to neighbor, and not just tile co-location. There may be TPs close by but
   //! in a different tile. The difficulty is that for large overlaps, TP searches are done on a
   //! tile basis, so TPs from other tiles are not known.
   //! @param neighborList List of tiepoints considered neighbors. The list can contain THIS
   //!   tiepoint -- it will be skipped.
   //! @return TRUE if consistent.
   bool checkConsistency(const std::vector< std::shared_ptr<AutoTiePoint> >& neighborList);

   //! Compares this TP's residual against the given neighbor's residual and returns TRUE if
   //! deemed consistent.
   bool residualsAreConsistent(const ossimDpt& R1);

   //! Recomputes active peak's residual after a model adjustment or change in peak.
   //! Residuals are in view space for this type.
   virtual void recomputeResidual();

   //! Costly processing for ATP common settings done only once for all TPs.
   void initializeStaticMembers();

   AtpTileSource*     m_overlap;
   std::vector<MatchPoint> m_matchPoints;
   ossimDpt           m_refViewPt;
   ossimDpt           m_cmpViewPt;
   ossimDpt           m_residual;
   double             m_relativeError;    //!< Uncertainty in position of feature in CMP image.

   static bool        s_initialized;
   static double      s_maxDiffRatio;
   static double      s_cosMaxAngleDiff;
   static double      s_minVectorResDiff;
};

typedef std::vector< std::shared_ptr<AutoTiePoint> > AtpList;

}
