//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#ifndef AtpGeneratorBase_H_
#define AtpGeneratorBase_H_

#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimPolyArea2d.h>
#include <ossim/imaging/ossimImageChain.h>
#include <ossim/imaging/ossimImageGeometry.h>
#include <ossim/imaging/ossimAnnotationSource.h>
#include <AutoTiePoint.h>
#include <AtpAnnotatedImage.h>
#include <Image.h>
#include <vector>
#include <memory>

namespace ATP
{

/**
 * Base class for OSSIM-based ATP generators.
 * TODO: Currently, ATP generation only works with pair-wise matching. Eventually it should
 * support N-way matching with an arbitrary number of input images. The multiple image case is
 * presently handled by the AutoTiePointService.
 */
class AtpGeneratorBase : public std::enable_shared_from_this<AtpGeneratorBase>

{
public:
   AtpGeneratorBase();

   virtual ~AtpGeneratorBase();

   void setRefImage(std::shared_ptr<Image> ref_image);
   void setCmpImage(std::shared_ptr<Image> cmp_image);

   /**
    * When the input images are multiband, the bands must be combined into a single-band image.
    * @param weights Vector of normalized weights (0-1) applied to the input bands for combining.
    */
   void setBandWeights(const std::vector<double> weights) { m_bandWeights = weights; }

   /**
    * This is the main workhorse method. It launches the auto-tie-point generation process given
    * the REF and CMP images.
    * TODO: Currently, ATP generation only works with pair-wise matching. Eventually it should
    * support N-way matching with an arbitrary number of input images.
    */
   virtual bool generateTiePointList(AtpList& tpList);

   /**
    * For engineering use. Renders relevant tiepoint data to disk file(s). Optionally
    * accepts an LUT file for raster-based remapping.
    * @param outBaseName This is the base name of possible multiple outputs, as needed by derived
    * generator.
    * @param lutFile Optional LUT file formatted as specified in ossimIndexToRgbLutFilter.
    * @return True if successful.
    */
//   virtual bool renderTiePointDataAsImages(const ossimFilename& outBaseName,
//                                           const ossimFilename& lutFile=ossimFilename("")) = 0;

   /**
    * For engineering use. Convenience method for outputting tiepoint list to output stream.
    */
   static void writeTiePointList(ostream& out, const AtpList& tpList);

protected:
   /**
    * Initializes the specific generator. Should be overriden by derived class as needed, though it
    * still needs to be called from the derived initialize() as it initializes the overlap polygon
    * and other common quantities
    * */
   virtual void initialize();

   /**
    * Constructs the processing chain for the input image according to the needs of the generator.
    */
   virtual ossimRefPtr<ossimImageChain> constructChain(std::shared_ptr<Image> image,
                                                       std::vector<ossimDpt>& validVertices);

   //! Finds optimum layout of patches within the intersect area for feature search.
   void layoutSearchTileRects(ossimPolygon& overlapPoly);

   //! Removes inconsistent residuals
   void filterBadMatches(AtpList& tpList);

   //! Caps the max number of TPs given the list, which is the list of filtered TPs for the tile.
   void pruneList(AtpList& tpList);


   std::shared_ptr<Image> m_refImage;
   std::shared_ptr<Image> m_cmpImage;
   ossimRefPtr<AtpTileSource> m_atpTileSource;
   ossimRefPtr<ossimImageChain> m_refChain;
   ossimRefPtr<ossimImageChain> m_cmpChain;
   ossimRefPtr<ossimImageGeometry> m_viewGeom;
   ossimIrect m_aoiView;
   std::vector<double> m_bandWeights;
   double m_refEllipHgt;
   std::vector<ossimIrect> m_searchTileRects;
   static std::shared_ptr<AutoTiePoint> s_referenceATP;

   ossimRefPtr<AtpAnnotatedImage> m_annotatedRefImage; // For engineering use
   ossimRefPtr<AtpAnnotatedImage> m_annotatedCmpImage; // For engineering use
};
}
#endif /* AtpGeneratorBase_H_ */
