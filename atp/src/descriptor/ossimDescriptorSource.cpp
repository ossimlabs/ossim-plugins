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

ossimDescriptorSource::~ossimDescriptorSource()
{
}

void ossimDescriptorSource::initialize()
{
   AtpTileSource::initialize();

   ref_source = ((ossimImageSource*)getInput(0));
   cmp_source = ((ossimImageSource*)getInput(1));

   m_A2BXform = new ossimImageViewProjectionTransform(ref_source->getImageGeometry().get(), cmp_source->getImageGeometry().get());
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
   if(getNumberOfInputs() < 2){
      clog << MODULE << " ERROR: wrong number of inputs " << getNumberOfInputs() << " when expecting at least 2 \n";
      return 0;
   }

   string sid(""); // Leave blank to have it auto-assigned by CorrelationTiePoint constructor


   // Compute the bounds of the ref tile projected on the cmp tile
   std::vector<ossimDpt> corners;
   corners.push_back(tileRect.ul()); corners.push_back(tileRect.ur()); corners.push_back(tileRect.ll()); corners.push_back(tileRect.lr());
   for(size_t i = 0; i < 4; ++i) m_A2BXform->imageToView(corners[i],corners[i]);
   
   int lowX = corners[0].x, highX = corners[0].x, lowY = corners[0].y, highY = corners[0].y;
   for(size_t i = 1; i < 4; ++i){
      if(lowX > corners[i].x) lowX = corners[i].x;
      if(highX < corners[i].x) highX = corners[i].x;
      if(lowY > corners[i].y) lowY = corners[i].y;
      if(highY < corners[i].y) highY = corners[i].y;
   }
   ossimIrect boundsOfCMP(lowX-10,lowY-10,highX+20,highY+20);

   // Retrieve both the ref and cmp image data
   ref_tile = ref_source->getTile(tileRect, resLevel);
   if (ref_tile->getDataObjectStatus() == OSSIM_EMPTY){
      clog << MODULE << " ERROR: could not get REF tile with rect " << tileRect << "\n";
      return m_tile;
   }

   cmp_tile = cmp_source->getTile(boundsOfCMP, resLevel);
   if (cmp_tile->getDataObjectStatus() == OSSIM_EMPTY){
      clog << MODULE << " ERROR: could not get CMP tile with rect " << boundsOfCMP << "\n";
      return m_tile;
   }

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
   if(descriptorType == "AKAZE"){
      cv::Ptr<cv::AKAZE> detector = cv::AKAZE::create();
      detector->detectAndCompute(queryImg, cv::noArray(), kpA, desA);
      detector->detectAndCompute(trainImg, cv::noArray(), kpB, desB);
   } else if(descriptorType == "KAZE"){
      cv::Ptr<cv::KAZE> detector = cv::KAZE::create();
      detector->detectAndCompute(queryImg, cv::noArray(), kpA, desA);
      detector->detectAndCompute(trainImg, cv::noArray(), kpB, desB);
   } else if(descriptorType == "ORB"){

      // For some reason orb wants multiple channel mats so fake it.
      if (queryImg.channels() == 1) {
         std::vector<cv::Mat> channels;
         channels.push_back(queryImg);
         channels.push_back(queryImg);
         channels.push_back(queryImg);
         cv::merge(channels, queryImg);
      }

      if (trainImg.channels() == 1) {
         std::vector<cv::Mat> channels;
         channels.push_back(trainImg);
         channels.push_back(trainImg);
         channels.push_back(trainImg);
         cv::merge(channels, trainImg);
      }

      cv::Ptr<cv::ORB> detector = cv::ORB::create();
      detector->detectAndCompute(queryImg, cv::noArray(), kpA, desA);
      detector->detectAndCompute(trainImg, cv::noArray(), kpB, desB);
   } else if(descriptorType == "SURF"){

      queryImg.convertTo(queryImg, CV_8U);
      trainImg.convertTo(trainImg, CV_8U);

      cv::Ptr<cv::xfeatures2d::SURF> detector = cv::xfeatures2d::SURF::create();
      detector->detectAndCompute(queryImg, cv::noArray(), kpA, desA);
      detector->detectAndCompute(trainImg, cv::noArray(), kpB, desB);
   } else if(descriptorType == "SIFT"){

      queryImg.convertTo(queryImg, CV_8U);
      trainImg.convertTo(trainImg, CV_8U);

      cv::Ptr<cv::xfeatures2d::SIFT> detector = cv::xfeatures2d::SIFT::create();
      detector->detectAndCompute(queryImg, cv::noArray(), kpA, desA);
      detector->detectAndCompute(trainImg, cv::noArray(), kpB, desB);
   } else {
      std::clog << MODULE << " WARNING: No such descriptor as " << descriptorType << "\n";
      return m_tile;
   }


   // Get the DPoints using the appropriate matcher.
   std::vector< std::vector<cv::DMatch> > weakMatchs;
   if(!(desA.empty() || desB.empty())) {

      std::string matcherType = config.getParameter("matcher").asString();
      uint k = config.getParameter("k").asUint();
      if (kpA.size() < k) k = kpA.size();
      transform(matcherType.begin(), matcherType.end(), matcherType.begin(),::toupper);

      if(matcherType == "FLANN") {
         if(desA.type()!=CV_32F) desA.convertTo(desA, CV_32F);
         if(desB.type()!=CV_32F) desB.convertTo(desB, CV_32F);
         cv::FlannBasedMatcher matcher;
         matcher.knnMatch(desA, desB, weakMatchs, k);
      } else if(matcherType == "BF"){
         std::clog << MODULE << " WARNING: BF NOT YET IMPLEMENTED\n";
         return m_tile;
      } else {
         std::clog << MODULE << " WARNING: No such matcher as " << matcherType << "\n";
         return m_tile;
      }
   }


   int leastDistance = INT_MAX;
   int maxDistance = 0;
   // Find the highest distance to compute the relative confidence of each match
   for (size_t i = 0; i < weakMatchs.size(); ++i){
      for (size_t j = 0; j < weakMatchs[i].size(); ++j){
         if (leastDistance > weakMatchs[i][j].distance) leastDistance = weakMatchs[i][j].distance;
         if (maxDistance < weakMatchs[i][j].distance) maxDistance = weakMatchs[i][j].distance;
      }
   }

   std::vector< std::vector<cv::DMatch> > strongMatches;
   for(size_t i = 0; i < weakMatchs.size(); ++i){
      std::vector<cv::DMatch> temp;
      for(size_t j = 0; j < weakMatchs[i].size(); ++j){
         if(weakMatchs[i][j].distance < ((maxDistance-leastDistance)*.2+leastDistance))
            temp.push_back(weakMatchs[i][j]);
      }
      if(temp.size()>0) strongMatches.push_back(temp);
   }

   // Convert the openCV match points to something Atp could understand.
   for (size_t i = 0; i < strongMatches.size(); ++i) {
      shared_ptr<AutoTiePoint> atp (new AutoTiePoint(this, sid));
      cv::KeyPoint cv_A = kpA[(strongMatches[i][0]).queryIdx];
      cv::KeyPoint cv_B;

      atp->setRefImagePt(ossimDpt(tileRect.ul().x + cv_A.pt.x, tileRect.ul().y + cv_A.pt.y));

      // Create the match points
      for (size_t j = 0; j < strongMatches[i].size(); ++j){
         cv_B = kpB[(strongMatches[i][j]).trainIdx];
         atp->addImageMatch(ossimDpt(boundsOfCMP.ul().x + cv_B.pt.x, boundsOfCMP.ul().y + cv_B.pt.y), static_cast<double>(maxDistance-(strongMatches[i][j].distance))/maxDistance);
      }

      m_tiePoints.push_back(atp);
      sid.clear();
   }

   if (config.diagnosticLevel(2))
      clog<<MODULE<<"Before filtering: num matches in tile = "<<strongMatches.size()<<endl;

return m_tile;
}

}