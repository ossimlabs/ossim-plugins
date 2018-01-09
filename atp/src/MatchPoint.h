//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#ifndef MatchPoint_HEADER
#define MatchPoint_HEADER

#include <ossim/base/ossimDpt.h>

namespace ATP
{
//*************************************************************************************************
//  CLASS DESCRIPTION:
//! Maintains quantities associated with single matchpoint peak. This consists of the image point
//! on the query/CMP image that corresponds to the feature in the REF image, and the strength
//! (confidence level) of the correlation (0.0 - 1.0). There will be more than one match (one
//! instance of this class) per REF image feature maintained by the AutoTiePoint. The ATP
//! will decide, based on match confidence and consistency with neighboring ATPs, whether a match
//! should be considered as the correct match point.
//!
//! Wish List:
//! May eventually want to consider a 2x2 (2-D) covariance matrix to accompany the peak value.
//*************************************************************************************************
class MatchPoint
{
public:

   //! Constructor for creating new instance with data members provided.
   MatchPoint(const ossimDpt& cmpImagePt,
                   double corr_value,
                   const ossimDpt& residual=ossimDpt(0,0))
      :  m_confidence(corr_value),
         m_cmpImagePt       (cmpImagePt),
         m_residual         (residual)  {}

   //! Copy constructor.
   MatchPoint(const MatchPoint& copy)
      :  m_confidence(copy.m_confidence),
         m_cmpImagePt      (copy.m_cmpImagePt),
         m_residual (copy.m_residual) {}
      
   //! Destructor does nothing.
   ~MatchPoint() { }

   //! Const access methods.
   const double&   getConfidenceMeasure() const { return m_confidence; }

   //! Returns the image point corresponding to the correlated feature on the CMP image
   const ossimDpt& getImagePoint () const { return m_cmpImagePt; }

   //! Sets/gets the residual associated with this peak (for APC functionality in consistency check)
   void setResidual(const ossimDpt& residual) { m_residual = residual; }
   const ossimDpt& getResidual() const        { return m_residual; }

   friend std::ostream& operator << (std::ostream& out, const MatchPoint& cp)
   {
      out<<cp.m_cmpImagePt<<", "<<cp.m_confidence;
      return out;
   }

private:
   double   m_confidence;
   ossimDpt m_cmpImagePt;
   ossimDpt m_residual;
};

}
#endif
