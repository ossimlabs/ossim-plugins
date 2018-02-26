//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "ossimDescriptorSource.h"
#include "../AtpOpenCV.h"
#include "../../AtpCommon.h"
#include <ossim/imaging/ossimImageDataFactory.h>

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

   m_nominalCmpPatchSize = 0.0;
   unsigned int refPatchSize = AtpConfig::instance().getParameter("corrWindowSize").asUint(); // min distance between points
   m_nominalCmpPatchSize = DEFAULT_CMP_PATCH_SIZING_FACTOR*refPatchSize;

   if (m_refIVT && m_cmpIVT)
   {
      ossimDpt gsd = m_refIVT->getOutputMetersPerPixel();
      double nominalGsd = (gsd.x + gsd.y)/2.0;

      // Fetch the CE90 of both images to determine cmp image search patch size:
      ossimSensorModel* ref_sm = dynamic_cast<ossimSensorModel*>(
              m_refIVT->getImageGeometry()->getProjection());
      ossimSensorModel* cmp_sm = dynamic_cast<ossimSensorModel*>(
              m_cmpIVT->getImageGeometry()->getProjection());
      if (ref_sm && cmp_sm)
      {
         double ref_ce = ref_sm->getNominalPosError();
         double cmp_ce = cmp_sm->getNominalPosError();
         double total_error = ref_ce + cmp_ce; // meters -- being conservative here and not doing rss
         ossimDpt gsd = m_refIVT->getOutputMetersPerPixel();
         double nominalGsd = (gsd.x + gsd.y)/2.0;
         double searchSizePixels = 2.0*total_error/nominalGsd;
         m_nominalCmpPatchSize = refPatchSize + searchSizePixels;
      }
      else
      {
         double searchSizePixels = 2.0*DEFAULT_NOMINAL_POSITION_ERROR/nominalGsd;
         m_nominalCmpPatchSize = refPatchSize + searchSizePixels;
      }
   }
}

ossimRefPtr<ossimImageData> ossimDescriptorSource::getTile(const ossimIrect& tileRect,
                                                            ossim_uint32 resLevel)
{
   static const char* MODULE = "ossimDescriptorSource::getTile()  ";
   AtpConfig& config = AtpConfig::instance();

   if (config.diagnosticLevel(2))
      CINFO<<"\n\n"<< MODULE << " -- tileRect: "<<tileRect<<endl;
   m_tiePoints.clear();

   if(!m_tile.get())
   {
      // try to initialize
      initialize();
      if(!m_tile.get()){
         CINFO << MODULE << " ERROR: could not initialize tile\n";
         return m_tile;
      }
   }

   // Make sure there are at least two images as input.
   if(getNumberOfInputs() < 2)
   {
      CINFO << MODULE << " ERROR: wrong number of inputs " << getNumberOfInputs() << " when expecting at least 2 \n";
      return 0;
   }


   // The tileRect passed in is in a common map-projected view space. Need to transform the rect
   // into image space for both ref and cmp images:
   ossimIrect refRect (m_refIVT->getViewToImageBounds(tileRect));
   ossimIrect cmpRect (m_cmpIVT->getViewToImageBounds(tileRect));
   cmpRect.expand(ossimIpt(m_nominalCmpPatchSize, m_nominalCmpPatchSize));

   // Retrieve both the ref and cmp image data
   ossimRefPtr<ossimImageData> ref_tile = m_refChain->getTile(refRect, resLevel);
   if (ref_tile->getDataObjectStatus() == OSSIM_EMPTY)
   {
      CWARN << MODULE << " ERROR: could not get REF tile with rect " << refRect << "\n";
      return m_tile;
   }
   if (config.diagnosticLevel(5))
      ref_tile->write("REF_TILE.RAS");


   ossimRefPtr<ossimImageData> cmp_tile = m_cmpChain->getTile(cmpRect, resLevel);
   if (cmp_tile->getDataObjectStatus() == OSSIM_EMPTY)
   {
      CWARN << MODULE << " ERROR: could not get CMP tile with rect " << cmpRect << "\n";
      return m_tile;
   }
   if (config.diagnosticLevel(5))
      cmp_tile->write("CMP_TILE.RAS");

   m_tile = ref_tile;

   // Convert both into OpenCV Mat objects using the Ipl image.
   IplImage* refImage = convertToIpl(ref_tile.get());
   IplImage* cmpImage = convertToIpl(cmp_tile.get());
   cv::Mat queryImg = cv::cvarrToMat(refImage);
   cv::Mat trainImg = cv::cvarrToMat(cmpImage);

   // Get the KeyPoints using appropriate descriptor.
   vector<cv::KeyPoint> kpA;
   vector<cv::KeyPoint> kpB;
   cv::Mat desA;
   cv::Mat desB;

   ossimString descriptorType (config.getParameter("descriptor").asString());
   descriptorType.upcase();

   // Search for features and compute descriptors for all:
   cv::Ptr<cv::Feature2D> detector;
   if(descriptorType == "AKAZE")
      detector = cv::AKAZE::create();
   else if(descriptorType == "KAZE")
      detector = cv::KAZE::create();
   else if(descriptorType == "ORB")
      detector = cv::ORB::create();
   else if(descriptorType == "SURF")
      detector = cv::xfeatures2d::SURF::create();
   else if(descriptorType == "SIFT")
      detector = cv::xfeatures2d::SIFT::create();
   else
   {
      CWARN << MODULE << " WARNING: No such descriptor as " << descriptorType << "\n";
      return m_tile;
   }

#if 1
   detector->detectAndCompute(queryImg, cv::noArray(), kpA, desA);
   detector->detectAndCompute(trainImg, cv::noArray(), kpB, desB);

#else
   // Separate detect and compute to limit the number of features for which descriptors are
   // computed...  Perform detection:
   detector->detect(queryImg, kpA);
   detector->detect(trainImg, kpB);

   CINFO << "\nDETECT:\n  kpA.size = "<<kpA.size() << endl;
   CINFO << "  kpb.size = "<<kpA.size() << endl;

   // Limit strongest detections to max number of features per tile on query image:
   unsigned int maxNumFeatures = config.getParameter("numFeaturesPerTile").asUint();
   CINFO << "\n numFeatures= "<<maxNumFeatures << endl;
   if (kpA.size() > maxNumFeatures)
   {
      sort(kpA.begin(), kpA.end(), sortFunc);
      kpA.resize(maxNumFeatures);
   }

   //if (kpB.size() > maxNumFeatures)
   //{
   //   sort(kpB.begin(), kpB.end(), sortFunc);
   //   kpB.resize(maxNumFeatures);
   //}
   //CINFO << "\nTRIM:\n  kpA.size = "<<kpA.size() << endl;
   //CINFO << "  kpb.size = "<<kpA.size() << endl;

   // Now perform descriptor computations on remaining features:
   detector->compute(queryImg, kpA, desA);
   detector->compute(trainImg, kpB, desB);
   CINFO << "\nCOMPUTE:\n  desA.size = "<<desA.size() << endl;
   CINFO << "  desB.size = "<<desB.size() << endl;

#endif

   if (config.diagnosticLevel(5))
   {
      CINFO << "\nDETECT:\n  kpA.size = " << kpA.size() << endl;
      CINFO << "  kpb.size = " << kpA.size() << endl;
      CINFO << "\nCOMPUTE:\n  desA.size = " << desA.size() << endl;
      CINFO << "  desB.size = " << desB.size() << endl;
   }

   // Get the DPoints using the appropriate matcher.
   std::vector< std::vector<cv::DMatch> > matches;
   if(!(desA.empty() || desB.empty()))
   {
      std::string matcherType = config.getParameter("matcher").asString();
      uint k = config.getParameter("maxNumMatchesPerFeature").asUint();
      if (config.paramExists("k")) // support legacy keywords
         k = config.getParameter("k").asUint();
      if (kpA.size() < k)
         k = kpA.size();
      transform(matcherType.begin(), matcherType.end(), matcherType.begin(),::toupper);

      shared_ptr<cv::DescriptorMatcher> matcher;
      if(matcherType == "FLANN")
      {
         if(desA.type()!=CV_32F)
            desA.convertTo(desA, CV_32F);
         if(desB.type()!=CV_32F)
            desB.convertTo(desB, CV_32F);

         matcher.reset(new cv::FlannBasedMatcher);
      }
      else if(matcherType == "BF")
      {
         if(desA.type()!=CV_8U)
            desA.convertTo(desA, CV_8U);
         if(desB.type()!=CV_8U)
            desB.convertTo(desB, CV_8U);

         matcher.reset(new cv::BFMatcher(cv::NORM_HAMMING, true));
      }
      else
      {
         CWARN << MODULE << " WARNING: No such matcher as " << matcherType << "\n";
         return m_tile;
      }

      matcher->knnMatch(desA, desB, matches, k);
   }


   float minDistance = INT_MAX;
   float maxDistance = 0;

   // Find the highest distance to compute the relative confidence of each match
   for (auto &featureMatches : matches)
   {
      for (auto &match : featureMatches)
      {
         if (minDistance >match.distance)
            minDistance = match.distance;
         if (maxDistance < match.distance)
            maxDistance = match.distance;
      }
   }

   // Experimental: force min distance to be the perfect match case = 0:
   minDistance = 0.0;
   float delta = maxDistance - minDistance;

   // Sort the matches in order of strength (i.e., confidence) using stl map:
   map<double, shared_ptr<AutoTiePoint> > tpMap;

   // Convert the openCV match points to something Atp could understand.
   string sid(""); // Leave blank to have it auto-assigned by CorrelationTiePoint constructor

   for (auto &featureMatches : matches)
   {
      if (featureMatches.empty())
         continue;

      shared_ptr<AutoTiePoint> atp (new AutoTiePoint(this, sid));
      cv::KeyPoint cv_A = kpA[(featureMatches[0]).queryIdx];
      cv::KeyPoint cv_B;

      ossimDpt refImgPt (refRect.ul().x + cv_A.pt.x, refRect.ul().y + cv_A.pt.y);
      atp->setRefImagePt(refImgPt);

      // Create the match points
      double strength = 0;
      for (auto &match : featureMatches)
      {
         cv_B = kpB[(match).trainIdx];
         ossimDpt cmpImgPt (cmpRect.ul().x + cv_B.pt.x, cmpRect.ul().y + cv_B.pt.y);
         double strength_j = 1.0 - (match.distance-minDistance)/delta;
         if (strength_j > strength)
            strength = strength_j;
         atp->addImageMatch(cmpImgPt, strength_j);
      }

      // Insert into sorted map using one's complement as key since in ascending order:
      tpMap.insert(pair<double, shared_ptr<AutoTiePoint> >(1.0-strength, atp));
      sid.clear();
   }

   if (config.diagnosticLevel(2))
      CINFO<<MODULE<<"Before filtering, num matches in tile = "<<matches.size()<<endl;

   // Now skim off the best matches and copy them to the list being returned:
   unsigned int N = config.getParameter("numFeaturesPerTile").asUint();
   unsigned int n = 0;
   auto tp_iter = tpMap.begin();
   while ((tp_iter != tpMap.end()) && (n < N))
   {
      m_tiePoints.push_back(tp_iter->second);
      tp_iter++;
      n++;
   }

   if (config.diagnosticLevel(2))
      CINFO<<MODULE<<"After capping to max num features ("<<N<<"), num TPs in tile = "<<n<<endl;

   return m_tile;
}

}
