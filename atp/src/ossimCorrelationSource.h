//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#ifndef ossimCorrelationSource_HEADER
#define ossimCorrelationSource_HEADER

#include "AutoTiePoint.h"
#include "AtpConfig.h"
#include "AtpTileSource.h"
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimRefPtr.h>
#include <memory>
namespace ATP
{
/**
 * Finds auto-tie-points using the corss-correlation-based matching algorithm
 */
class OSSIMDLLEXPORT ossimCorrelationSource : public AtpTileSource
{
public:
   ossimCorrelationSource();
   ossimCorrelationSource(ossimConnectableObject::ConnectableObjectList& inputSources);
   ossimCorrelationSource(AtpGenerator* generator);

   virtual ~ossimCorrelationSource();

   virtual void initialize();

   virtual ossimRefPtr<ossimImageData> getTile(const ossimIrect& origin, ossim_uint32 rLevel=0);

   /** For engineering use in annotating CMP image */
   unsigned int getNominalCmpPatchSize() const { return m_nominalCmpPatchSize; }

protected:
   //! Given an image tile, locates prominent features in that tile. Use
   void findFeatures(const ossimImageData* data, std::vector<ossimIpt>& featurePts);

   bool correlate(std::shared_ptr<AutoTiePoint> atp);

   bool OpenCVCorrelation(std::shared_ptr<AutoTiePoint> atp,
                          const ossimImageData*  refpatch,
                          const ossimImageData*  cmppatch);

   unsigned int m_nominalCmpPatchSize;

};
}
#endif /* #ifndef ossimCorrelationSource_HEADER */
