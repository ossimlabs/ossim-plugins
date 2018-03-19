//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#ifndef ossimDescriptorSource_HEADER
#define ossimDescriptorSource_HEADER

#include "AutoTiePoint.h"
#include "AtpConfig.h"
#include "AtpTileSource.h"
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimRefPtr.h>
#include <memory>
#include <opencv2/opencv.hpp>

namespace ATP
{
/**
 * Finds auto-tie-points using the descriptor-based matching algorithm
 */
class OSSIMDLLEXPORT ossimDescriptorSource : public AtpTileSource
{
public:
   ossimDescriptorSource(AtpGenerator* generator);

   virtual ~ossimDescriptorSource();

   virtual void initialize();

   virtual ossimRefPtr<ossimImageData> getTile(const ossimIrect& origin, ossim_uint32 rLevel=0);

private:
   struct SortFunc
   {
      bool operator()(cv::KeyPoint lhs, cv::KeyPoint rhs)
      { return (lhs.response > rhs.response); };
   } sortFunc;

   ossimDescriptorSource();
   ossimDescriptorSource(ossimConnectableObject::ConnectableObjectList& inputSources);

   unsigned int m_cmpPatchInflation;
};
}
#endif /* #ifndef ossimDescriptorSource_HEADER */
