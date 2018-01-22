//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************
#ifndef AtpAnnotatedImage_H_
#define AtpAnnotatedImage_H_

#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimPolyArea2d.h>
#include <ossim/imaging/ossimImageChain.h>
#include <ossim/imaging/ossimImageGeometry.h>
#include <ossim/imaging/ossimAnnotationSource.h>
#include <ossim/projection/ossimImageViewProjectionTransform.h>
#include "AutoTiePoint.h"
#include "Image.h"
#include <vector>
#include <memory>

namespace ATP
{

/**
 * For engineering use in ATP.
 */
class AtpAnnotatedImage : public ossimImageChain

{
public:
   AtpAnnotatedImage(ossimRefPtr<ossimImageChain>& sourceChain,
                     ossimRefPtr<ossimImageViewProjectionTransform>& ivt,
                     const ossimDrect& aoi);
   ~AtpAnnotatedImage();

   void setImageName(const std::string& name) { m_annFilename = name; }

   void annotateFeatureSearchTiles(std::vector<ossimIrect>& tileRects);
   void annotateCorrelations(AtpList& atpList, int r, int g, int b);
   void annotateCorrelationSearchTile(const ossimIrect& tileRect);
   void annotateOverlap(ossimPolygon& poly);
   void annotateResiduals(AtpList& atps, int r, int g, int b);
   void annotateTPIDs(AtpList& atps, int r, int g, int b);
   void annotateCorrBoxes(AtpList& atpList, int r);

   bool write();

protected:
   void drawBox(const ossimIrect& rect, int r, int g, int b);

   ossimRefPtr<ossimAnnotationSource> m_annSource; // For engineering use
   ossimDpt m_annScale; // For engineering use
   ossimDpt m_annScaleInverse; // For engineering use
   ossimIrect m_annScaledAoi;
   ossimFilename m_annFilename;
};
}
#endif /* AtpAnnotatedImage_H_ */
