//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include <descriptor/DescriptorMatchTileSource.h>
#include <descriptor/Descriptor.h>
#include <descriptor/Matcher.h>
#include <AtpConfig.h>
#include <AtpOpenCV.h>
#include <ossim/imaging/ossimImageData.h>
#include <ossim/base/ossimConstants.h>
#include <ossim/imaging/ossimImageDataFactory.h>
#include <opencv/cv.h>

namespace ATP
{
DescriptorMatchTileSource::DescriptorMatchTileSource()
{
}

DescriptorMatchTileSource::DescriptorMatchTileSource(ossimConnectableObject::ConnectableObjectList& inputSources)
: AtpTileSource(inputSources)
{
}

DescriptorMatchTileSource::~DescriptorMatchTileSource()
{
}

void DescriptorMatchTileSource::initialize()
{
   AtpTileSource::initialize();

   ossimRefPtr<ossimImageSource> ref_source ((ossimImageSource*)getInput(0));
   ossimRefPtr<ossimImageSource> cmp_source ((ossimImageSource*)getInput(1));

   m_A2BXform = new ossimImageViewProjectionTransform(ref_source->getImageGeometry().get(), cmp_source->getImageGeometry().get());
   // Base class should have done everything for Descriptor-based matching, but need to verify
}

ossimRefPtr<ossimImageData> DescriptorMatchTileSource::getTile(const ossimIrect& tileRect,
                                                               ossim_uint32 resLevel)
{   
   AtpConfig& config = AtpConfig::instance();
   if (config.diagnosticLevel(3))
      clog<<"\n\nDescriptorMatchTileSource::getTile() -- tileRect: "<<tileRect<<endl;

   m_tiePoints.clear();
   if(!m_tile.get())
   {
      // try to initialize
      initialize();
      if(!m_tile.get())
         return m_tile;
   }

   if(getNumberOfInputs() < 2)
      return 0;

   m_tile->setImageRectangle(tileRect);
   m_tile->makeBlank();
   string sid(""); // Leave blank to have it auto-assigned by CorrelationTiePoint constructor

   ossimRefPtr<ossimImageSource> ref_source ((ossimImageSource*)getInput(0));
   ossimRefPtr<ossimImageSource> cmp_source ((ossimImageSource*)getInput(1));

   // Need to search for features in the REF tile first, then consider only those locations
   // for correlating:
   ossimRefPtr<ossimImageData> ref_tile = ref_source->getTile(tileRect, resLevel);
   if (ref_tile->getDataObjectStatus() == OSSIM_EMPTY)
      return m_tile;
   vector<ossimIpt> featurePoints;
   ossimRefPtr<ossimImageData> tileptr (ref_tile);


   // Create ossimImageViewProjectionTransform to compute cmp/query bounding Irect.
   ossimDpt ul = tileRect.ul();
   ossimDpt ur = tileRect.ur();
   ossimDpt ll = tileRect.ll();
   ossimDpt lr = tileRect.lr();
   m_A2BXform->imageToView(ul,ul);
   m_A2BXform->imageToView(ur,ur);
   m_A2BXform->imageToView(ll,ll);
   m_A2BXform->imageToView(lr,lr);

   // Add some padding because there are some errors and distortions
   ul.x = ul.x - 10;   ul.y = ul.y - 10;
   lr.x = lr.x + 10;   lr.y = lr.y + 10;
   ll.x = ll.x - 10;   ll.y = ll.y + 10;
   ur.x = ur.x + 10;   ur.y = ur.y - 10;

   ossimIrect boundsOfB(ul,ur,lr,ll);
   

   // Retrieve the cmp/query image.
   ossimRefPtr<ossimImageData> cmp_tile = cmp_source->getTile(boundsOfB, resLevel);
   if (ref_tile->getDataObjectStatus() == OSSIM_EMPTY)
      return m_tile;


   Descriptor descript;

   IplImage* refImage = convertToIpl32(ref_tile.get());
   IplImage* cmpImage = convertToIpl32(cmp_tile.get());

   cv::Mat trainImg = cv::cvarrToMat(refImage);
   cv::Mat queryImg = cv::cvarrToMat(cmpImage);

   std::vector<cv::KeyPoint> kpA;
   std::vector<cv::KeyPoint> kpB;

   cv::Mat desA;
   cv::Mat desB;


   // Use proper descriptor from config file
   // TODO: DYLAN: need to map from string or int to enum here. What is expected in the config file?
   //       Assuming uint here.
   DescriptorType descriptor = (DescriptorType) config.getParameter("descriptor").asUint();
   if(descriptor == DescriptorType::AKAZE){
      kpA = descript.akaze(trainImg, desA);
      kpB = descript.akaze(queryImg, desB);
   } else if(descriptor == DescriptorType::KAZE){
      kpA = descript.kaze(trainImg, desA);
      kpB = descript.kaze(queryImg, desB);
   } else if(descriptor == DescriptorType::SURF){
      kpA = descript.surf(trainImg, desA);
      kpB = descript.surf(queryImg, desB);
   } else if(descriptor == DescriptorType::SIFT){
      kpA = descript.sift(trainImg, desA);
      kpB = descript.sift(queryImg, desB);
   }


   // If not enough keypoints were found for a size of k
   // continue on because it would not be able to be matched
   unsigned int k = config.getParameter("k").asUint();
   if(k>kpB.size())
      return m_tile;

   Matcher match;

   std::vector< std::vector<cv::DMatch> > matches;

   // Use proper matcher from config file
   // TODO: DYLAN: need to map from string or int to enum here. What is expected in the config file?
   //       Assuming uint here.
   MatcherType matcher = (MatcherType) config.getParameter("matcher").asUint();
   if(matcher == MatcherType::FLANN){
      matches = match.flann(desA, desB, k);
   } else if(matcher == MatcherType::BF){
      matches = match.bruteForce(desA, desB, k, config.getParameter("errorFunction").asInt());
   }


   // Loop through the matched points and generate a tiepoint list
   for(size_t i = 0; i < matches.size(); ++i){
      std::shared_ptr<AutoTiePoint> tiepoint_candidate = std::make_shared<AutoTiePoint>(this, std::to_string(i));
      ossimDpt refImagePt; refImagePt.x = kpA[matches[i][0].trainIdx].pt.x; refImagePt.y = kpA[matches[i][0].trainIdx].pt.y;
      tiepoint_candidate->setRefImagePt(refImagePt);
      for(size_t j = 0; j < matches[i].size(); ++j){
         ossimDpt cmpImagePt; cmpImagePt.x = kpA[matches[i][j].queryIdx].pt.x; cmpImagePt.y = kpA[matches[i][j].queryIdx].pt.y;
         tiepoint_candidate->addImageMatch(cmpImagePt);
      } // For loop of j (peaks, i = 0 is strongest match, i = k-1 is weakest match)
   } // For loop of i (matched points)

   //*******************
   // TODO: The ref_tile contains the tile on the ref/training image for the Irect passed in. Add
   // logic here to pull from the CMP/query image as well with its own Irect.
   // Take a look at AtpOpenCV.h for possible useful utility functions.
   //*******************


   // Leave this here to check the validity of the returned tile. Need to decide what actually to
   // write to the output tile (if anything). The main product of this source is the tiepoint list,
   // but this tile can be consumed for debug purposes.
   m_tile->validate();
   //m_tile->write("chip.ras");
   return m_tile;
}


}


