//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#ifndef AtpTileSource_HEADER
#define AtpTileSource_HEADER

#include "AtpConfig.h"
#include "AutoTiePoint.h"
#include <ossim/base/ossimIrect.h>
#include <ossim/imaging/ossimImageCombiner.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/projection/ossimImageViewProjectionTransform.h>
#include <memory>
#include <vector>

namespace ATP
{
class AtpGenerator;
/**
 * Base class for tile sources performing auto tie point extraction. Implemented as a combiner that
 * establishes the overlap between input sources, and computes tie-points in the getTile() call.
 */
class OSSIMDLLEXPORT AtpTileSource : public ossimImageCombiner
{
public:
   AtpTileSource(AtpGenerator* generator);

   virtual ~AtpTileSource() {}

   virtual void initialize();

   /**
    * Derived classes implement their particular tiepoint matching algorithms for the requested
    * tile. It is not clear what the output tile needs to be, since the product of this filter
    * is the list of tiepoints (available through the getTiePoints() method). Recommend simply
    * passing through the corresponding REF image tile.
    */
   virtual ossimRefPtr<ossimImageData> getTile(const ossimIrect& origin, ossim_uint32 rLevel=0) = 0;

   /** This list resets with every getTile() call , so make sure tiepoints from previous getTile()
    *  calls are being accumulated */
   AtpList& getTiePoints() { return m_tiePoints; }

   virtual ossimScalarType getOutputScalarType() const { return OSSIM_DOUBLE; }
   virtual ossim_uint32 getNumberOfOutputBands() const { return 2; }

   void filterPoints();

protected:
   //! Convenience struct for use between filterWithParallax( and computeParallaxStatistics()
   struct ParallaxStatistics
   {
      double dx_dH;
      double dy_dH;
      double parallaxSlope;
      double parallaxOffset;
      double denom;
      double maxDistance;
      std::map<std::string, double> distanceMap;
   };

   AtpTileSource();
   AtpTileSource(ossimConnectableObject::ConnectableObjectList& inputSources);

   virtual void allocate();

   void filterWithParallax(ParallaxStatistics& paxstats);
   void computeParallaxStatistics(ParallaxStatistics& paxstats);

   void filterWithoutParallax();

   //! Caps the max number of TPs given the list, which is the list of filtered TPs for the tile.
   void pruneList();

   void initializeStaticMembers();

   std::shared_ptr<AtpGenerator> m_generator;
   AtpList m_tiePoints;
   ossimRefPtr<ossimImageData> m_tile; // Used only for raster mode (rare)

   // Statics assigned once for speed
   static double s_minVectorResDiff;
   static double s_maxDiffRatio;
   static double s_cosMaxAngleDiff;
   static double s_maxPaxDev;
   static double s_maxPaxPix;
   static unsigned int s_minNumConsistent;
   static unsigned int s_percentConsistent;
   static unsigned int s_numTpsPerTile;
   static bool s_considerParallax;
   static bool s_useRasterMode;
   static bool s_initialized;
   static unsigned int s_numFilterIterations;

};
}
#endif /* #ifndef AtpTileSource_HEADER */
