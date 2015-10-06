//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file.
//
// Author:  David Hicks
//
// Description: Automatic tie measurement extraction.
//
//----------------------------------------------------------------------------
// $Id$

#include <ossim/base/ossimString.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimConstants.h>
#include <ossim/elevation/ossimElevManager.h>
#include <ossim/imaging/ossimImageData.h>
#include <ossim/imaging/ossimImageSource.h>

#include "ossimTieMeasurementGenerator.h"
#include "ossimIvtGeomXformVisitor.h"

#include <opencv/highgui.h>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/flann/flann.hpp>
#include <opencv2/legacy/legacy.hpp>
// Note: These are purposely commented out to indicate non-use.
// #include <opencv2/nonfree/nonfree.hpp>
// #include <opencv2/nonfree/features2d.hpp>
// Note: These are purposely commented out to indicate non-use.

#include <vector>
#include <iostream>

static ossimTrace traceExec  ("ossimTieMeasurementGenerator:exec");
static ossimTrace traceDebug ("ossimTieMeasurementGenerator:debug");


//*****************************************************************************
//  METHOD: ossimTieMeasurementGenerator::ossimTieMeasurementGenerator()
//  
//  Constructor.
//*****************************************************************************
ossimTieMeasurementGenerator::ossimTieMeasurementGenerator()
   :
   m_src(0),
   m_igxA(0),
   m_igxB(0),
   m_imgA(),
   m_imgB(),
   m_numMeasurements(0),
   m_maxMatches(5),
   m_spIndexA(0),
   m_spIndexB(1),
   m_patchSizeA(),
   m_patchSizeB(),
   m_validBox(false),   
   m_useGrid(false),
   m_showCvWindow(false),
   m_patchRefA(),
   m_patchRefB(),
   m_measA(),
   m_measB(),
   m_distEditFactor(0),
   m_detectorName("ORB"),
   m_detector(),
   m_extractorName("FREAK"),
   m_extractor(),
   m_matcherName("BruteForce-Hamming"),
   m_matcher(),
   m_rep(0),
   m_maxCvWindowDim(500),
   m_cvWindowName("Correlation Patch")
{
   if (traceExec())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "\nDEBUG: ...ossimTieMeasurementGenerator::constructor" << std::endl;
   }
}


//*****************************************************************************
//  METHOD: ossimTieMeasurementGenerator::ossimTieMeasurementGenerator()
//  
//  Initializer.
//*****************************************************************************
bool ossimTieMeasurementGenerator::init(std::ostream& report)
{
   if (traceExec())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "DEBUG: ...ossimTieMeasurementGenerator::init" << std::endl;
   }

   m_initOK = true;
   ossimString ts;
   ossim::getFormattedTime("%a %m.%d.%y %H:%M:%S", false, ts);

   m_rep = &report;


   // Initial report output
   *m_rep << "\nossimTieMeasurementGenerator Report     ";
   *m_rep << ts;
   *m_rep << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~";
   *m_rep << std::endl;
 
   // Set default grid size
   setGridSize(ossimIpt(1,1));

   try{
   // Set default detector, extractor, matcher
      setFeatureDetector(m_detectorName);
      setDescriptorExtractor(m_extractorName);
      setDescriptorMatcher(m_matcherName);

   }
   catch(...)
   {
      m_initOK = false;
   }

   return m_initOK;
}


//*****************************************************************************
//  DESTRUCTOR: ~ossimTieMeasurementGenerator()
//  
//*****************************************************************************
ossimTieMeasurementGenerator::~ossimTieMeasurementGenerator()
{
   if (traceExec())  ossimNotify(ossimNotifyLevel_DEBUG)
      << "DEBUG: ~ossimTieMeasurementGenerator(): returning..." << std::endl;
}


//*****************************************************************************
//  METHOD: ossimTieMeasurementGenerator::refreshCollectionTraits()
//  
//  Collection trait initializer.
//*****************************************************************************
bool ossimTieMeasurementGenerator::refreshCollectionTraits()
{
   bool initOK = true;

   if (m_useGrid)
   {
      int gridRows = m_gridSize.y;
      int gridCols = m_gridSize.x;
      cv::Ptr<cv::FeatureDetector> detector = m_detector;
      m_detector = new cv::GridAdaptedFeatureDetector(detector, m_maxMatches, gridRows, gridCols);
   }

   return initOK;
}


//*****************************************************************************
//  METHOD: ossimTieMeasurementGenerator::run()
//  
//  
//*****************************************************************************
bool ossimTieMeasurementGenerator::run()
{
   bool runOK = true;

   // Refresh collection traits before run
   runOK = refreshCollectionTraits();

   if (runOK)
   {
      // ossimIrect constructor center-based constructor
      //    -> center view coordinates: m_patchRefA,m_patchRefB
      ossimIrect rectA(m_patchRefA, m_patchSizeA.x, m_patchSizeA.y);
      ossimIrect rectB(m_patchRefB, m_patchSizeB.x, m_patchSizeB.y);
      
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "DEBUG: ...ossimTieMeasurementGenerator::run" << std::endl;
         ossimNotify(ossimNotifyLevel_DEBUG)<<" m_patchRefA: "<<m_patchRefA<<" size: "<<m_patchSizeA<<std::endl; 
         ossimNotify(ossimNotifyLevel_DEBUG)<<" m_patchRefB: "<<m_patchRefB<<" size: "<<m_patchSizeB<<std::endl; 
         ossimNotify(ossimNotifyLevel_DEBUG)<<" rectA = "<<rectA<<endl;
         ossimNotify(ossimNotifyLevel_DEBUG)<<" rectB = "<<rectB<<endl;
         ossimNotify(ossimNotifyLevel_DEBUG)<<" m_src A ossimScalarType = "<<m_src[m_spIndexA]->getOutputScalarType()<<endl;
         ossimNotify(ossimNotifyLevel_DEBUG)<<" m_src B ossimScalarType = "<<m_src[m_spIndexB]->getOutputScalarType()<<endl;
      }

      // Get the patches
      ossimRefPtr<ossimImageData> idA = m_src[m_spIndexA]->getTile(rectA);   
      ossimRefPtr<ossimImageData> idB = m_src[m_spIndexB]->getTile(rectB);

      // Create the OpenCV images
      m_imgA.create(cv::Size(rectA.width(), rectA.height()), CV_8UC1);
      m_imgB.create(cv::Size(rectB.width(), rectB.height()), CV_8UC1);

      memcpy(m_imgA.ptr(), (void*) idA->getBuf(), rectA.area());
      memcpy(m_imgB.ptr(), (void*) idB->getBuf(), rectB.area());

      if(m_imgA.empty())
      {
         *m_rep << "cv::Mat A creation failed..."<<std::endl;;
         runOK = false;
      }
      else if(m_imgB.empty())
      {
         *m_rep << "cv::Mat B creation failed..."<<std::endl;;
         runOK = false;
      }
      else
      {
         // Detector
         vector<cv::KeyPoint> keypointsA;
         vector<cv::KeyPoint> keypointsB;
         m_detector->detect(m_imgA, keypointsA);
         m_detector->detect(m_imgB, keypointsB);
         
         // Extractor
         //    32 byte uchar arrays   CV_8U (0) ..........
         //    cout << desc.rows << " " << desc.cols << " " << desc.type() << std::endl;
         //    uchar d = desc.at<uchar>(row,col);
         cv::Mat descriptorsA;
         cv::Mat descriptorsB;
         m_extractor->compute(m_imgA, keypointsA, descriptorsA);
         m_extractor->compute(m_imgB, keypointsB, descriptorsB);
         
         // Execute matcher
         //  TODO add cross-check process here?
         //----------
         // BruteForceMatcher<L2<float> > descriptorMatcher;
         // vector<DMatch> filteredMatches12, matches12, matches21;
         // descriptorMatcher.match( descriptors1, descriptors2, matches12 );
         // descriptorMatcher.match( descriptors2, descriptors1, matches21 );
         // for( size_t i = 0; i < matches12.size(); i++ )
         // {
         //     DMatch forward = matches12[i];
         //     DMatch backward = matches21[forward.trainIdx];
         //     if( backward.trainIdx == forward.queryIdx )
         //         filteredMatches12.push_back( forward );
         // }
         //----------
         std::vector<cv::DMatch> matches;
         std::vector<cv::DMatch> matchesAB;
         m_matcher->match(descriptorsA, descriptorsB, matchesAB);
         
         matches = matchesAB;

         // Symmetry
         // void symmetryTest(const std::vector<cv::DMatch> &matches1,const std::vector<cv::DMatch> &matches2,std::vector<cv::DMatch>& symMatches)
         // {
         //     symMatches.clear();
         //     for (vector<DMatch>::const_iterator matchIterator1= matches1.begin();matchIterator1!= matches1.end(); ++matchIterator1)
         //     {
         //         for (vector<DMatch>::const_iterator matchIterator2= matches2.begin();matchIterator2!= matches2.end();++matchIterator2)
         //         {
         //             if ((*matchIterator1).queryIdx ==(*matchIterator2).trainIdx &&(*matchIterator2).queryIdx ==(*matchIterator1).trainIdx)
         //             {
         //                 symMatches.push_back(DMatch((*matchIterator1).queryIdx,(*matchIterator1).trainIdx,(*matchIterator1).distance));
         //                 break;
         //             }
         //         }
         //     }
         // }

         *m_rep<<" Match resulted in "<<descriptorsA.rows<<" points..."<<std::endl;

         //-- Calculate max and min distances between keypoints
         double maxDist = 0;
         double minDist = 500;
         int maxRows = matches.size();
         if(maxRows > descriptorsA.rows) maxRows = descriptorsA.rows;
         for( int i = 0; i < maxRows; i++ )
         {
            double dist = matches[i].distance;
            if( dist < minDist ) minDist = dist;
            if( dist > maxDist ) maxDist = dist;
         }

         //-- Check for "good" matches (i.e. whose distance is less than 2*minDist )
         //-- TODO -> radiusMatch can also be used here?
         m_distEditFactor = 3; //TODO
         std::vector<cv::DMatch> goodMatches;
         for( int i = 0; i < maxRows; i++ )
         {
            if( matches[i].distance < m_distEditFactor*minDist )
            {
               goodMatches.push_back( matches[i]);
            }
         }
         *m_rep<<" Distance filter ("<<std::setw(1)<<m_distEditFactor<<"X min) resulted in "<<goodMatches.size()<<" points..."<<std::endl;
         *m_rep<<"  -- Max dist : "<<maxDist<<std::endl;
         *m_rep<<"  -- Min dist : "<<minDist<<std::endl;
      
         if (m_maxMatches<(int)goodMatches.size())
         {
            nth_element(goodMatches.begin(),goodMatches.begin()+m_maxMatches,goodMatches.end());
            goodMatches.erase(goodMatches.begin()+m_maxMatches,goodMatches.end());
         }
         
         // Load measurements
         *m_rep<<"\n Selected top "<<goodMatches.size()<<"..."<<std::endl;
         *m_rep<<"   n queryIdx trainIdx imgIdx distance            A            B"<<std::endl;
         *m_rep<<"---- -------- -------- ------ -------- ------------ ------------"<<std::endl;
         
         for( ossim_uint32 i = 0; i < goodMatches.size(); ++i )
         {
            double xA = keypointsA[goodMatches[i].queryIdx].pt.x;
            double yA = keypointsA[goodMatches[i].queryIdx].pt.y;
            double xB = keypointsB[goodMatches[i].trainIdx].pt.x;
            double yB = keypointsB[goodMatches[i].trainIdx].pt.y;

            *m_rep<<std::setw(4)<<i+1<<" "
               <<std::setw(8)<<goodMatches[i].queryIdx<<" "
               <<std::setw(8)<<goodMatches[i].trainIdx<<" "
               <<std::setw(6)<<goodMatches[i].imgIdx<<" "
               <<std::fixed<<std::setprecision(1)<<std::setw(8)
               <<goodMatches[i].distance<<" ("
               <<std::fixed<<std::setprecision(0)
               <<std::setw(4)<<xA << ", " 
               <<std::setw(4)<<yA <<") ("
               <<std::setw(4)<<xB << ", " 
               <<std::setw(4)<<yB <<") "
               <<std::endl;
         
            // View coordinates
            ossimDpt vptA(xA, yA);
            ossimDpt vptB(xB, yB);
            vptA += m_patchRefA - m_patchSizeA/2;
            vptB += m_patchRefB - m_patchSizeB/2;

            // Image coordinates
            ossimDpt iptA;
            ossimDpt iptB;
            m_igxA->viewToImage(vptA, iptA);
            m_igxB->viewToImage(vptB, iptB);

            m_measA.push_back(iptA);
            m_measB.push_back(iptB);
            ++m_numMeasurements;
         }
         

         // Pop up the results window
         if (m_showCvWindow)
         {
            showCvResultsWindow(keypointsA, keypointsB, goodMatches);
         }
      }

   }
   else
   {
      *m_rep << "An error occurred in collection trait initialization..."<<std::endl;
      *m_rep << "Measurement collection could not be executed."<<std::endl;
   }

   summarizeRun();

   return runOK;
}


//*****************************************************************************
//  METHOD: ossimTieMeasurementGenerator::setFeatureDetector()
//   Set the feature detector
//  
// The following detector types are supported:
//     "FAST" – FastFeatureDetector
//     "STAR" – StarFeatureDetector
//     "SIFT" – SIFT (nonfree module)
//     "SURF" – SURF (nonfree module)
//     "ORB" – ORB
//     "BRISK" – BRISK
//     "MSER" – MSER
//     "GFTT" – GoodFeaturesToTrackDetector
//     "HARRIS" – GoodFeaturesToTrackDetector with Harris detector enabled
//     "Dense" – DenseFeatureDetector
//     "SimpleBlob" – SimpleBlobDetector
// Also a combined format is supported: feature detector adapter name
// ( "Grid" – GridAdaptedFeatureDetector, "Pyramid" – PyramidAdaptedFeatureDetector )
//+ feature detector name (see above), for example: "GridFAST", "PyramidSTAR".
//*****************************************************************************
bool ossimTieMeasurementGenerator::setFeatureDetector(const ossimString& name)
{
   bool createOK = false;
   m_detectorName = name;
   m_detector  = cv::FeatureDetector::create(m_detectorName);

   if( m_detector != 0 )
   {
      createOK = true;
      std::vector<std::string> parameters;
      m_detector->getParams(parameters);
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "DEBUG: ...detector..." << std::endl;
         for (int i = 0; i < (int) parameters.size(); i++)
            ossimNotify(ossimNotifyLevel_DEBUG)<<"  "<<parameters[i]<<std::endl;
      }
   }

   return createOK;
}

//*****************************************************************************
//  METHOD: ossimTieMeasurementGenerator::setDescriptorExtractor()
//   Set the descriptor-extractor
//  
// The current implementation supports the following types of a descriptor extractor:
// Not Used "SIFT" – SIFT (nonfree module)
// Not Used "SURF" – SURF (nonfree module)
//          "ORB" – ORB
//          "BRISK" – BRISK
//          "BRIEF" – BriefDescriptorExtractor
// A combined format is also supported: descriptor extractor adapter name
//( "Opponent" – OpponentColorDescriptorExtractor ) + descriptor extractor name
//(see above), for example: "OpponentSIFT" .
//
// cv::Algorithm::set, cv::Algorithm::getParams
//featureDetector->set("someParam", someValue)
//*****************************************************************************
bool ossimTieMeasurementGenerator::setDescriptorExtractor(const ossimString& name)
{
   bool createOK = false;
   m_extractorName = name;
   m_extractor = cv::DescriptorExtractor::create(m_extractorName);

   if( m_extractor != 0 )
   {
      createOK = true;
      std::vector<std::string> parameters;
      m_extractor->getParams(parameters);
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "DEBUG: ...extractor..." << std::endl;
         for (int i = 0; i < (int) parameters.size(); i++)
            ossimNotify(ossimNotifyLevel_DEBUG)<<"  "<<parameters[i]<<std::endl;
      }
   }

   return createOK;
}

//*****************************************************************************
//  METHOD: ossimTieMeasurementGenerator::getDescriptorMatcher()
//   Set the descriptor-matcher
//  
// Descriptor matcher type. Now the following matcher types are supported:
//     BruteForce-Hamming
//     BruteForce-HammingLUT
//     FlannBased
//
//  The current decriptor-matcher compatibility table (updated for this note) is the following:
//                 BruteForce BruteForce-L1 FlannBased BruteForce-Hamming BruteForce-HammingLUT
//  Not Used SURF  YES        YES           YES        NO                 NO
//  Not Used SIFT  YES        YES           YES        NO                 NO
//           ORB   NO         NO            YES(Lsh)   YES                YES
//           BRIEF NO         NO            YES(Lsh)   YES                YES
//           FREAK NO         NO            YES(Lsh)   YES                YES
// The ORB and BRIEF descriptors and BruteForce-Hamming and BruteForce-HammingLUT
// matching schemes generate or accept binary-string descriptors (CV_8U), while
// the other descriptors and matching schemes generate or accept vectors of floating
// point values (CV_32F). It is possible to mix and match the descriptors more by
// converting the data format of the descriptor matrix prior to matching, but this can
// sometimes lead to very poor matching results, so attempt it with caution.
//*****************************************************************************
bool ossimTieMeasurementGenerator::setDescriptorMatcher(const ossimString& name)
{
   bool createOK = false;
   m_matcherName = name;

   if (m_matcherName == "FlannBased")
   {
      // Set LshIndexParams(int table_number, int key_size, int multi_probe_level)
      m_matcher = new cv::FlannBasedMatcher(new cv::flann::LshIndexParams(20,10,2));
   }
   else
   {
      m_matcher = cv::DescriptorMatcher::create(m_matcherName);
   }

   if( m_matcher != 0 )
   {
      createOK = true;
      std::vector<std::string> parameters;
      m_matcher->getParams(parameters);
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "DEBUG: ...matcher..." << std::endl;
         for (int i = 0; i < (int) parameters.size(); i++)
            ossimNotify(ossimNotifyLevel_DEBUG)<<"  "<<parameters[i]<<std::endl;
      }
   }

   // TODO: Garrett: this was causing core dumps so I commented it out.  Also, it appears that in the docs 
   //          the default for crossCheck argument in the base constructor is false.
      // Set crossCheck
      //m_matcher->set("crossCheck", false);


   return createOK;
}


//*****************************************************************************
//  METHOD: ossimTieMeasurementGenerator::setGridSize()
//  
//*****************************************************************************
bool ossimTieMeasurementGenerator::setGridSize(const ossimIpt& gridDimensions)
{
   bool setOK = false;

   if (gridDimensions.x>0 && gridDimensions.y>0)
   {
      m_gridSize = gridDimensions;
      setOK = true;
   }

   return setOK;
}


//*****************************************************************************
//  METHOD: ossimTieMeasurementGenerator::setBox()
//
//  Delineate the bounding collection boxes
//*****************************************************************************
bool ossimTieMeasurementGenerator::setBox(std::vector<ossimIrect> roi,
                                          const ossim_uint32& index,
                                          std::vector<ossimImageSource*> src)
{

   m_validBox = false;

   // Load the image source pointers
   //    Note: The source pointer vector may contain more than
   //          two images, but only the first two are used
   //          in the auto measurement process.
   m_src.clear();
   for (ossim_uint32 n=0; n<src.size(); ++n)
      m_src.push_back(src[n]);


   // Set the patch indices
   if (index<2)
   {
      m_spIndexA = index;
      if (m_spIndexA == 0)
         m_spIndexB = 1;
      else
         m_spIndexB = 0;

      // Save reference points (patch centers)
      roi[m_spIndexA].getCenter(m_patchRefA);
      roi[m_spIndexB].getCenter(m_patchRefB);

      // Save dimensions (patch sizes)
      m_patchSizeA.x = roi[m_spIndexA].width();
      m_patchSizeA.y = roi[m_spIndexA].height();
      m_patchSizeB.x = roi[m_spIndexB].width();
      m_patchSizeB.y = roi[m_spIndexB].height();

      // Set the IvtGeometryXforms
      ossimIvtGeomXformVisitor visitorA;
      m_src[m_spIndexA]->accept(visitorA);
      if (visitorA.getTransformList().size() == 1)
      {
         m_igxA = visitorA.getTransformList()[0].get();
      }
      ossimIvtGeomXformVisitor visitorB;
      m_src[m_spIndexB]->accept(visitorB);
      if (visitorB.getTransformList().size() == 1)
      {
         m_igxB = visitorB.getTransformList()[0].get();
      }

      m_validBox = true;

      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "DEBUG: ...ossimTieMeasurementGenerator::setBox" << std::endl;
         ossimNotify(ossimNotifyLevel_DEBUG)<<" m_spIndexA  = "<<m_spIndexA<<std::endl;
         ossimNotify(ossimNotifyLevel_DEBUG)<<" m_spIndexB  = "<<m_spIndexB<<std::endl;
         ossimNotify(ossimNotifyLevel_DEBUG)<<" m_patchRefA = "<<m_patchRefA<<std::endl;
         ossimNotify(ossimNotifyLevel_DEBUG)<<" m_patchRefB = "<<m_patchRefB<<std::endl;
         ossimNotify(ossimNotifyLevel_DEBUG)<<" m_patchSizeA = "<<m_patchSizeA<<std::endl;
         ossimNotify(ossimNotifyLevel_DEBUG)<<" m_patchSizeB = "<<m_patchSizeB<<std::endl;
      }
   }

   return m_validBox;
}


//*****************************************************************************
//  METHOD: ossimTieMeasurementGenerator::setMaxMatches()
//  
//*****************************************************************************
bool ossimTieMeasurementGenerator::setMaxMatches(const int& maxMatches)
{
   bool setOK = false;

   if (maxMatches > 0)
   {
      m_maxMatches = maxMatches;
      setOK = true;
   }

   return setOK;
}


//*****************************************************************************
//  METHOD: ossimTieMeasurementGenerator::pointIndexedAt()
//  
//*****************************************************************************
ossimDpt ossimTieMeasurementGenerator::pointIndexedAt
   (const ossim_uint32 imgIdx, const ossim_uint32 measIdx)
{
   if ((int)measIdx<m_numMeasurements && imgIdx<2)
   {
      if (imgIdx == 0)
      {
         if (m_spIndexA == 0)
            return m_measA[measIdx];
         else
            return m_measB[measIdx];
      }
      else
      {
         if (m_spIndexA == 1)
            return m_measA[measIdx];
         else
            return m_measB[measIdx];
      }
   }
   else
   {
      ossimDpt bad;
      bad.makeNan();
      return bad;
   }
}


//*****************************************************************************
//  METHOD: ossimTieMeasurementGenerator::summarizeRun()
//  
//*****************************************************************************
void ossimTieMeasurementGenerator::summarizeRun() const
{
   *m_rep<<"\n Configuration..."<<std::endl;
   *m_rep<<"  Detector:   "<<getFeatureDetector()<<std::endl;
   *m_rep<<"  Descriptor: "<<getDescriptorExtractor()<<std::endl;
   *m_rep<<"  Matcher:    "<<getDescriptorMatcher()<<std::endl;
   *m_rep<<"  Patch size: "<<m_patchSizeA<<std::endl;
   *m_rep<<"  Grid size:  "<<m_gridSize<<std::endl;

   ossimString ts;
   ossim::getFormattedTime("%a %m.%d.%y %H:%M:%S", false, ts);
   *m_rep << "\n";
   *m_rep << "\n" << ts;
   *m_rep << "\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~";
   *m_rep << std::endl;
}


//*****************************************************************************
//  METHOD: ossimTieMeasurementGenerator::showCvResultsWindow()
//  
//*****************************************************************************
void ossimTieMeasurementGenerator::showCvResultsWindow(
   std::vector<cv::KeyPoint> keypointsA,
   std::vector<cv::KeyPoint> keypointsB,
   std::vector<cv::DMatch> goodMatches) 
{
   //-- Draw only "good" matches
   // DEFAULT
   // DRAW_OVER_OUTIMG
   // NOT_DRAW_SINGLE_POINTS
   // DRAW_RICH_KEYPOINTS
   cv::namedWindow(m_cvWindowName);
   cv::Mat imgMatch;
   cv::drawMatches(m_imgA, keypointsA, m_imgB, keypointsB, goodMatches, imgMatch,
       cv::Scalar::all(-1), cv::Scalar::all(-1), vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);      

   // Scale down the results window if necessary using a somewhat arbitrary factor for now
   double rRatio = m_maxCvWindowDim / std::max(m_patchSizeA.y, m_patchSizeB.y);
   double cRatio = m_maxCvWindowDim / std::max(m_patchSizeA.x, m_patchSizeB.x);
   double sFac = 1.0;
   if (rRatio <= cRatio)
   {
      if (rRatio < 1.0)
         sFac = rRatio;
   }
   else
   {
      if (cRatio< 1.0)
         sFac = cRatio;
   }
   cv::Mat imgMatchOut;
   cv::resize(imgMatch, imgMatchOut, cv::Size(), sFac, sFac, cv::INTER_AREA);
   cv::imshow(m_cvWindowName, imgMatchOut);
}


//*****************************************************************************
//  METHOD: ossimTieMeasurementGenerator::closeCvWindow()
//  
//*****************************************************************************
void ossimTieMeasurementGenerator::closeCvWindow(const bool waitKeyPress) 
{
   if (waitKeyPress)
   {
      cv::waitKey();
   }

   cv::destroyWindow(m_cvWindowName);
}
