//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file.
//
// Author:  David Hicks
//
//
// Description: Automatic tie measurement extraction.
//----------------------------------------------------------------------------
#ifndef ossimTieMeasurementGenerator_HEADER
#define ossimTieMeasurementGenerator_HEADER 1

#include <ossim/base/ossimObject.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimTieMeasurementGeneratorInterface.h>
#include "ossimIvtGeomXform.h"

#include <opencv/cv.h>

#include <ctime>
#include <vector>
#include <iostream>

typedef std::vector<ossimDpt> DptVec_t;

class ossimImageSource;


class OSSIM_DLL ossimTieMeasurementGenerator : 
   public ossimTieMeasurementGeneratorInterface, public ossimObject
{
public:

   // Constructor/initializer
   ossimTieMeasurementGenerator();
   bool init(std::ostream& report = cout);

   // Define collection ROI
   bool setBox(std::vector<ossimIrect> roi,
               const ossim_uint32& index,
               std::vector<ossimImageSource*> src);
   bool isValidCollectionBox() const {return m_validBox;}   

   // Measurement collection
   bool run();

   // Report run parameters
   void summarizeRun() const;

   // Destructor
   ~ossimTieMeasurementGenerator();

   // Patch grid configuration accessors
   void setUseGrid(const bool useGrid) {m_useGrid = useGrid;}
   bool getUseGrid() const {return m_useGrid;}
   bool setGridSize(const ossimIpt& gridDimensions);
   ossimIpt getGridSize() const {return m_gridSize;}

   // Max matches in patch accessors
   bool setMaxMatches(const int& maxMatches);
   int getMaxMatches() const {return m_maxMatches;}

   // Set the feature detector
   bool setFeatureDetector(const ossimString& name);
   ossimString getFeatureDetector() const {return m_detectorName;}

   // Set the descriptor-extractor
   bool setDescriptorExtractor(const ossimString& name);
   ossimString getDescriptorExtractor() const {return m_extractorName;}

   // Set the matcher
   bool setDescriptorMatcher(const ossimString& name);
   ossimString getDescriptorMatcher() const {return m_matcherName;}

   // Measured point accessors
   int numMeasurements() const { return m_numMeasurements; }
   ossimDpt pointIndexedAt(const ossim_uint32 imgIdx, const ossim_uint32 measIdx);

   // OpenCV drawMatches window
   void closeCvWindow(const bool waitKeyPress = false);
   void setShowCvWindow(const bool showCvWindow) {m_showCvWindow = showCvWindow;}
   bool getShowCvWindow() const {return m_showCvWindow;}

protected:

   bool m_initOK;

   // Initialize patch reference positions
   bool refreshCollectionTraits();

   // Image-related members
   std::vector<ossimImageSource*> m_src;
   ossimRefPtr<ossimIvtGeomXform> m_igxA;
   ossimRefPtr<ossimIvtGeomXform> m_igxB;
   cv::Mat m_imgA;
   cv::Mat m_imgB;

   // Measurement count parameters
   int m_numMeasurements;
   int m_maxMatches;

   // Patch traits
   ossim_uint32 m_spIndexA;
   ossim_uint32 m_spIndexB;
   ossimIpt m_patchSizeA;
   ossimIpt m_patchSizeB;
   bool m_validBox;

   // Grid size for matcher
   bool m_useGrid;
   ossimIpt m_gridSize;

   // Patch reference point (center)
   ossimDpt m_patchRefA;
   ossimDpt m_patchRefB;

   // Measurement containers
   DptVec_t m_measA;
   DptVec_t m_measB;

   // Editing parameters
   int m_distEditFactor;

   // Pointer to detector
   ossimString m_detectorName;
   cv::Ptr<cv::FeatureDetector> m_detector;

   // Pointer to descriptor extractor
   ossimString m_extractorName;
   cv::Ptr<cv::DescriptorExtractor> m_extractor;

   // Pointer to matcher
   ossimString m_matcherName;
   cv::Ptr<cv::DescriptorMatcher> m_matcher;

   // Measurement run report
   std::ostream* m_rep;

   // Results window
   void showCvResultsWindow(
      std::vector<cv::KeyPoint> keypointsA,
      std::vector<cv::KeyPoint> keypointsB,
      std::vector<cv::DMatch> goodMatches);
   double m_maxCvWindowDim;
   ossimString m_cvWindowName;
   bool m_showCvWindow;
};
#endif // #ifndef ossimTieMeasurementGenerator_HEADER
