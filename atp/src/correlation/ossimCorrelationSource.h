//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#ifndef ossimCorrelationSource_HEADER
#define ossimCorrelationSource_HEADER

#include <AutoTiePoint.h>
#include <AtpConfig.h>
#include <AtpTileSource.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimRefPtr.h>
#include <memory>
namespace ATP
{
//*************************************************************************************************
//  CLASS DESCRIPTION:
//! A tile source that manages the feature correlation between two images (hence a combiner).
//! This class establishes correlations and corresponding residuals for the requested tile rect.
//! NOTE: All references to pixel coordinates in this class are assumed to be in
//! output projection (view) space. The primary product of this class is a list of
//! CorrelationTiePoint objects, each representing a set of correlation peaks for each feature.
//*************************************************************************************************
class OSSIMDLLEXPORT ossimCorrelationSource : public AtpTileSource
{
public:
   ossimCorrelationSource();
   ossimCorrelationSource(ossimConnectableObject::ConnectableObjectList& inputSources);

   virtual ~ossimCorrelationSource();

   virtual void initialize();

   virtual ossimRefPtr<ossimImageData> getTile(const ossimIrect& origin, ossim_uint32 rLevel=0);

   /** For engineering use in annotating CMP image */
   unsigned int getNominalCmpPatchSize() const { return m_nominalCmpPatchSize; }

protected:
   //! Given an image tile, locates prominent features in that tile. Use
   void findFeatures(const ossimImageData* data, std::vector<ossimIpt>& featurePts);

   bool correlate(std::shared_ptr<AutoTiePoint> atp);

   //! Called by correlate(), correlates two image patches using OpenCV imaging functions.
   //! Returns true if correlation was successful (this doesn't mean there were peaks, just that it
   //! worked). Returns false on failure.
   bool OpenCVCorrelation(std::shared_ptr<AutoTiePoint> atp,
                          const ossimImageData*  refpatch,
                          const ossimImageData*  cmppatch);

   unsigned int m_nominalCmpPatchSize;

};
}
#endif /* #ifndef ossimCorrelationSource_HEADER */
