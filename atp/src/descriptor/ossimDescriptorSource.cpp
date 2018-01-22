//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "ossimDescriptorSource.h"
#include "../AtpOpenCV.h"
#include <ossim/imaging/ossimImageData.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimConstants.h>
#include <ossim/imaging/ossimImageDataFactory.h>
#include <ossim/projection/ossimSensorModel.h>

#include <opencv2/opencv.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/xfeatures2d.hpp>

namespace ATP
{
static const int DEFAULT_CMP_PATCH_SIZING_FACTOR = 2;
static const double DEFAULT_NOMINAL_POSITION_ERROR = 50; // meters

ossimDescriptorSource::ossimDescriptorSource()
{
}

ossimDescriptorSource::ossimDescriptorSource(ossimConnectableObject::ConnectableObjectList& inputSources)
: AtpTileSource(inputSources)
{
   initialize();
}

ossimDescriptorSource::ossimDescriptorSource(AtpGeneratorBase* generator)
: AtpTileSource(generator)
{
}

ossimDescriptorSource::~ossimDescriptorSource()
{
}

void ossimDescriptorSource::initialize()
{
   // Base class initializes ref and cmp chain datat members and associated IVTs. Subsequent call
   // to base class setViewGeom() will initialize the associated IVTs:
   AtpTileSource::initialize();
}

ossimRefPtr<ossimImageData> ossimDescriptorSource::getTile(const ossimIrect& tileRect,
                                                            ossim_uint32 resLevel)
{
   static const char* MODULE = "ossimDescriptorSource::getTile()  ";
   AtpConfig& config = AtpConfig::instance();

   if (config.diagnosticLevel(2))
      clog<<"\n\n"<< MODULE << " -- tileRect: "<<tileRect<<endl;
   m_tiePoints.clear();

   if(!m_tile.get())
   {
      // try to initialize
      initialize();
      if(!m_tile.get()){
         clog << MODULE << " ERROR: could not initialize tile\n";
         return m_tile;
      }
   }

   // Make sure there are at least two images as input.
   if(getNumberOfInputs() < 2)
   {
      clog << MODULE << " ERROR: wrong number of inputs " << getNumberOfInputs() << " when expecting at least 2 \n";
      return 0;
   }


   // The tileRect passed in is in a common map-projected view space. Need to transform the rect
   // into image space for both ref and cmp images:
   ossimIrect refRect (m_refIVT->getViewToImageBounds(tileRect));
   ossimIrect cmpRect (m_cmpIVT->getViewToImageBounds(tileRect));

   // Retrieve both the ref and cmp image data
   ossimRefPtr<ossimImageData> ref_tile = m_refChain->getTile(refRect, resLevel);
   if (ref_tile->getDataObjectStatus() == OSSIM_EMPTY)
   {
      clog << MODULE << " ERROR: could not get REF tile with rect " << refRect << "\n";
      return m_tile;
   }
   if (config.diagnosticLevel(5))
      ref_tile->write("REF_TILE.RAS");


   ossimRefPtr<ossimImageData> cmp_tile = m_cmpChain->getTile(cmpRect, resLevel);
   if (cmp_tile->getDataObjectStatus() == OSSIM_EMPTY)
   {
      clog << MODULE << " ERROR: could not get CMP tile with rect " << cmpRect << "\n";
      return m_tile;
   }
   if (config.diagnosticLevel(5))
      cmp_tile->write("CMP_TILE.RAS");

   m_tile = ref_tile;

   // Convert both into OpenCV Mat objects using the Ipl image.
   IplImage* refImage = convertToIpl32(ref_tile.get());
   IplImage* cmpImage = convertToIpl32(cmp_tile.get());
   cv::Mat queryImg = cv::cvarrToMat(refImage);
   cv::Mat trainImg = cv::cvarrToMat(cmpImage);

   // Get the KeyPoints using appropriate descriptor.
   std::vector<cv::KeyPoint> kpA;
   std::vector<cv::KeyPoint> kpB;
   cv::Mat desA;
   cv::Mat desB;

   std::string descriptorType = config.getParameter("descriptor").asString();
   transform(descriptorType.begin(), descriptorType.end(), descriptorType.begin(),::toupper);
   if(descriptorType == "AKAZE")
   {
      cv::Ptr<cv::AKAZE> detector = cv::AKAZE::create();
      detector->detectAndCompute(queryImg, cv::noArray(), kpA, desA);
      detector->detectAndCompute(trainImg, cv::noArray(), kpB, desB);
   }
   else if(descriptorType == "KAZE")
   {
      cv::Ptr<cv::KAZE> detector = cv::KAZE::create();
      detector->detectAndCompute(queryImg, cv::noArray(), kpA, desA);
      detector->detectAndCompute(trainImg, cv::noArray(), kpB, desB);
   }
   else if(descriptorType == "ORB")
   {
      // For some reason orb wants multiple channel mats so fake it.
      if (queryImg.channels() == 1)
      {
         std::vector<cv::Mat> channels;
         channels.push_back(queryImg);
         channels.push_back(queryImg);
         channels.push_back(queryImg);
         cv::merge(channels, queryImg);
      }

      if (trainImg.channels() == 1)
      {
         std::vector<cv::Mat> channels;
         channels.push_back(trainImg);
         channels.push_back(trainImg);
         channels.push_back(trainImg);
         cv::merge(channels, trainImg);
      }

      cv::Ptr<cv::ORB> detector = cv::ORB::create();
      detector->detectAndCompute(queryImg, cv::noArray(), kpA, desA);
      detector->detectAndCompute(trainImg, cv::noArray(), kpB, desB);
   }
   else if(descriptorType == "SURF")
   {
      queryImg.convertTo(queryImg, CV_8U);
      trainImg.convertTo(trainImg, CV_8U);

      cv::Ptr<cv::xfeatures2d::SURF> detector = cv::xfeatures2d::SURF::create();
      detector->detectAndCompute(queryImg, cv::noArray(), kpA, desA);
      detector->detectAndCompute(trainImg, cv::noArray(), kpB, desB);
   }
   else if(descriptorType == "SIFT")
   {
      queryImg.convertTo(queryImg, CV_8U);
      trainImg.convertTo(trainImg, CV_8U);

      cv::Ptr<cv::xfeatures2d::SIFT> detector = cv::xfeatures2d::SIFT::create();
      detector->detectAndCompute(queryImg, cv::noArray(), kpA, desA);
      detector->detectAndCompute(trainImg, cv::noArray(), kpB, desB);
   }
   else
   {
      std::clog << MODULE << " WARNING: No such descriptor as " << descriptorType << "\n";
      return m_tile;
   }

   // Get the DPoints using the appropriate matcher.
   std::vector< std::vector<cv::DMatch> > weakMatchs;
   if(!(desA.empty() || desB.empty()))
   {
      std::string matcherType = config.getParameter("matcher").asString();
      uint k = config.getParameter("k").asUint();
      if (kpA.size() < k)
         k = kpA.size();
      transform(matcherType.begin(), matcherType.end(), matcherType.begin(),::toupper);

      if(matcherType == "FLANN")
      {
         if(desA.type()!=CV_32F)
            desA.convertTo(desA, CV_32F);
         if(desB.type()!=CV_32F)
            desB.convertTo(desB, CV_32F);
         cv::FlannBasedMatcher matcher;
         matcher.knnMatch(desA, desB, weakMatchs, k);
      }
      else if(matcherType == "BF")
      {
         std::clog << MODULE << " WARNING: BF NOT YET IMPLEMENTED\n";
         return m_tile;
      }
      else
      {
         std::clog << MODULE << " WARNING: No such matcher as " << matcherType << "\n";
         return m_tile;
      }
   }

   int leastDistance = INT_MAX;
   int maxDistance = 0;

   // Find the highest distance to compute the relative confidence of each match
   for (size_t i = 0; i < weakMatchs.size(); ++i)
   {
      for (size_t j = 0; j < weakMatchs[i].size(); ++j)
      {
         if (leastDistance > weakMatchs[i][j].distance)
            leastDistance = weakMatchs[i][j].distance;
         if (maxDistance < weakMatchs[i][j].distance)
            maxDistance = weakMatchs[i][j].distance;
      }
   }

   std::vector< std::vector<cv::DMatch> > strongMatches;
   for(size_t i = 0; i < weakMatchs.size(); ++i)
   {
      std::vector<cv::DMatch> temp;
      for(size_t j = 0; j < weakMatchs[i].size(); ++j)
      {
         if(weakMatchs[i][j].distance < ((maxDistance-leastDistance)*.2+leastDistance))
            temp.push_back(weakMatchs[i][j]);
      }
      if(temp.size()>0) strongMatches.push_back(temp);
   }

   // Sort the matches in order of strength (i.e., confidence) using stl map:
   map<double, shared_ptr<AutoTiePoint> > tpMap;

   // Convert the openCV match points to something Atp could understand.
   string sid(""); // Leave blank to have it auto-assigned by CorrelationTiePoint constructor
   for (size_t i = 0; i < strongMatches.size(); ++i)
   {
      shared_ptr<AutoTiePoint> atp (new AutoTiePoint(this, sid));
      cv::KeyPoint cv_A = kpA[(strongMatches[i][0]).queryIdx];
      cv::KeyPoint cv_B;

      ossimDpt refImgPt (refRect.ul().x + cv_A.pt.x, refRect.ul().y + cv_A.pt.y);
      atp->setRefImagePt(refImgPt);

      // Create the match points
      double strength = 0;
      for (size_t j = 0; j < strongMatches[i].size(); ++j)
      {
         cv_B = kpB[(strongMatches[i][j]).trainIdx];
         ossimDpt cmpImgPt (cmpRect.ul().x + cv_B.pt.x, cmpRect.ul().y + cv_B.pt.y);
         double strength_j = 1.0 - (strongMatches[i][j].distance-leastDistance)/maxDistance;
         if (strength_j > strength)
            strength = strength_j;
         atp->addImageMatch(cmpImgPt, strength_j);
      }
      tpMap.insert(pair<double, shared_ptr<AutoTiePoint> >(strength, atp));

      sid.clear();
   }

   if (config.diagnosticLevel(2))
      clog<<MODULE<<"Before filtering, num matches in tile = "<<strongMatches.size()<<endl;

   // Now skim off the best matches and copy them to the list being returned:
   unsigned int N = config.getParameter("numFeaturesPerTile").asUint();
   unsigned int n = 0;
   map<double, shared_ptr<AutoTiePoint> >::iterator tp_iter = tpMap.begin();
   while ((tp_iter != tpMap.end()) && (n < N))
   {
      m_tiePoints.push_back(tp_iter->second);
      tp_iter++;
      n++;
   }

   if (config.diagnosticLevel(2))
      clog<<MODULE<<"After capping to max num features ("<<N<<"), num TPs in tile = "<<n<<endl;

   return m_tile;
}

}
