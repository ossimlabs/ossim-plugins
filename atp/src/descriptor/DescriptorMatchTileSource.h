//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#ifndef DescriptorMatchTileSource_HEADER
#define DescriptorMatchTileSource_HEADER

#include <AtpTileSource.h>
#include <AtpConfig.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/imaging/ossimImageCombiner.h>
#include <memory>
namespace ATP
{
//*************************************************************************************************
//  CLASS DESCRIPTION:
//! A tile source that manages the descriptor-based feature matching between two images
//! This class establishes correlations and corresponding residuals for the requested tile rect.
//! NOTE: All references to pixel coordinates in this class are assumed to be in image space.
//! The primary product of this class is a list of
//! AutoTiePoint objects, each representing a set of matches for each feature.
//*************************************************************************************************
class OSSIMDLLEXPORT DescriptorMatchTileSource : public AtpTileSource
{
public:
   DescriptorMatchTileSource();
   DescriptorMatchTileSource(ossimConnectableObject::ConnectableObjectList& inputSources);

   virtual ~DescriptorMatchTileSource();

   virtual void initialize();

   virtual ossimRefPtr<ossimImageData> getTile(const ossimIrect& origin,
                                               ossim_uint32 resLevel=0);

protected:
	ossimRefPtr<ossimImageViewProjectionTransform> m_A2BXform;
};
}
#endif /* #ifndef DescriptorMatchTileSource_HEADER */
