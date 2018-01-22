//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#ifndef ossimDescriptorSource_HEADER
#define ossimDescriptorSource_HEADER

#include "../AutoTiePoint.h"
#include "../AtpConfig.h"
#include "../AtpTileSource.h"
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
//! DescriptorTiePoint objects, each representing a set of correlation peaks for each feature.
//*************************************************************************************************
class OSSIMDLLEXPORT ossimDescriptorSource : public AtpTileSource
{
public:
   ossimDescriptorSource();
   ossimDescriptorSource(ossimConnectableObject::ConnectableObjectList& inputSources);
   ossimDescriptorSource(AtpGeneratorBase* generator);

   virtual ~ossimDescriptorSource();

   virtual void initialize();

   virtual ossimRefPtr<ossimImageData> getTile(const ossimIrect& origin, ossim_uint32 rLevel=0);

};
}
#endif /* #ifndef ossimDescriptorSource_HEADER */
