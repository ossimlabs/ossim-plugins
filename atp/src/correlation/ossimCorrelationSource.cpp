//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "ossimCorrelationSource.h"
#include "../AtpOpenCV.h"
#include <ossim/imaging/ossimImageDataFactory.h>
#include "../../AtpCommon.h"

namespace ATP
{
static const int DEFAULT_CMP_PATCH_SIZING_FACTOR = 2;
static const double DEFAULT_NOMINAL_POSITION_ERROR = 50; // meters

ossimCorrelationSource::ossimCorrelationSource()
: m_nominalCmpPatchSize(0)
{
}

ossimCorrelationSource::ossimCorrelationSource(ossimConnectableObject::ConnectableObjectList& inputSources)
: AtpTileSource(inputSources),
  m_nominalCmpPatchSize(0)
{
   initialize();
}

ossimCorrelationSource::ossimCorrelationSource(AtpGeneratorBase* generator)
: AtpTileSource(generator),
  m_nominalCmpPatchSize(0)
{
}

ossimCorrelationSource::~ossimCorrelationSource()
{
}

void ossimCorrelationSource::initialize()
{
   // The base class sets up the input (image-space) protion of the chain:
   AtpTileSource::initialize();

   unsigned int refPatchSize = AtpConfig::instance().getParameter("corrWindowSize").asUint(); // min distance between points
   m_nominalCmpPatchSize = DEFAULT_CMP_PATCH_SIZING_FACTOR*refPatchSize;
   double total_error = 0;
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
         total_error = ref_ce + cmp_ce; // meters -- being conservative here and not doing rss
         double searchSizePixels = 2.0 * total_error / nominalGsd;
         m_nominalCmpPatchSize = refPatchSize + searchSizePixels;
      }
      if (total_error == 0)
      {
         double searchSizePixels = 2.0*DEFAULT_NOMINAL_POSITION_ERROR/nominalGsd;
         m_nominalCmpPatchSize = refPatchSize + searchSizePixels;
      }
   }
}

ossimRefPtr<ossimImageData> ossimCorrelationSource::getTile(const ossimIrect& tileRect,
                                                            ossim_uint32 resLevel)
{
   static const char* MODULE = "ossimCorrelationSource::getTile()  ";
   AtpConfig& config = AtpConfig::instance();
   if (config.diagnosticLevel(2))
      CINFO<<"\n\nossimCorrelationSource::getTile() -- tileRect UL: "<<tileRect.ul()<<endl;
   m_tiePoints.clear();

   if(getNumberOfInputs() < 2)
      return 0;

   string sid(""); // Leave blank to have it auto-assigned by AutoTiePoint constructor
   if (!config.getParameter("useRasterMode").asBool())
   {
      m_tile = m_refChain->getTile(tileRect, resLevel);
      if (m_tile->getDataObjectStatus() == OSSIM_EMPTY)
         return m_tile;

      // Need to search for features in the REF tile first, then consider only those locations
      // for correlating:
      vector<ossimIpt> featurePoints;
      ossimRefPtr<ossimImageData> tileptr (m_tile);

      findFeatures(tileptr.get(), featurePoints);
      if (featurePoints.empty())
         return m_tile;

      // Loop over features to perform correlations:
      for (size_t i=0; i<featurePoints.size(); ++i)
      {
         shared_ptr<ATP::AutoTiePoint> atp (new ATP::AutoTiePoint(this, sid));
         atp->setRefViewPt(featurePoints[i]);

         // Trick here to keep using the same ID until a good correlation is found:
         if (!correlate(atp))
         {
            sid = atp->getTiePointId();
            atp->setTiePointId("NP");
            m_tiePoints.push_back(atp);
         }
         else
         {
            m_tiePoints.push_back(atp);
            sid.clear();
         }
      }
      if (config.diagnosticLevel(2))
         CINFO<<MODULE<<"Before filtering: num matches in tile = "<<m_tiePoints.size()<<endl;
   }
   else // raster mode
   {
     // Scan raster-fashion along REF tile and correlate at intervals:
      unsigned int num_attempts = 0;
      int corrStepSize = 1;
      if (config.paramExists("corrStepSize"))
         corrStepSize = (int) config.getParameter("corrStepSize").asUint();
      for (int y = tileRect.ul().y; y < tileRect.lr().y; y += corrStepSize)
      {
         for (int x = tileRect.ul().x; x < tileRect.lr().x; x += corrStepSize)
         {
            ++num_attempts;
            shared_ptr<AutoTiePoint> atp (new AutoTiePoint(this, sid));
            atp->setRefViewPt(ossimDpt(x,y));

             // Trick here to keep using the same ID until a good correlation is found:
            if (!correlate(atp))
               sid = atp->getTiePointId();
            else
            {
               m_tiePoints.push_back(atp);
               sid.clear();
            }
         }
      }
      if (config.diagnosticLevel(2))
         CINFO<<MODULE<<"Tile match ratio: "<<m_tiePoints.size()<<" / "<<num_attempts<<endl;

   }
   return m_tile;
}

void ossimCorrelationSource::findFeatures(const ossimImageData* imageChip,
                                          vector<ossimIpt>& feature_points)
{
   const char* MODULE = "ossimCorrelationSource::findFeatures() -- ";

   feature_points.clear();
   if (!imageChip || (imageChip->getDataObjectStatus() == OSSIM_EMPTY))
   {
      CWARN<<MODULE<<"WARNING: NULL or empty image data passed in."<<endl;
      return;
   }

   // THE FOLLOWING ARE CRITICAL VALUES AND MAYBE SHOULD BE READ FROM KWL:
   AtpConfig& config = AtpConfig::instance();
   double refPatchSize = (double) config.getParameter("corrWindowSize").asUint(); // min distance between points
   static const double quality = config.getParameter("minFeatureStrength").asFloat();
   static const int use_harris= (int) config.getParameter("useHarrisCorner").asBool();
   static const double harris_k = config.getParameter("harrisKFactor").asFloat();
   const int averaging_block_size = (int) refPatchSize;

//   imageChip->write("imageChip.ras");//### TODO REMOVE
   IplImage *ipl_image = convertToIpl(imageChip);
   if(!ipl_image)
   {
      CFATAL<<MODULE << "ERROR: Got null IplImage pointer converting from ossimImageData*!";
      return;
   }
//   cvNamedWindow("ipl_image", 1); //###
//   cvShowImage("ipl_image", ipl_image); //###

   // Need to mask pixels outside of intersection:
   ossimIpt tileUL (imageChip->getImageRectangle().ul());
   IplImage* mask  = 0;
   double null_pixel = imageChip->getNullPix(0);
   if (imageChip->getDataObjectStatus() == OSSIM_PARTIAL)
   {
      CvSize bufsize = cvGetSize(ipl_image);
      mask  = cvCreateImage(bufsize, IPL_DEPTH_8U, 1 );
      ossimDpt pt;
      long offset = 0;
      for (int y=0; y<bufsize.height; y++)
      {
         for (int x=0; x<=bufsize.width-1; x++)
         {
            if (imageChip->getPix(offset) != null_pixel)
               mask->imageData[offset] = (unsigned char) 0xFF;
            else
               mask->imageData[offset] = (unsigned char) 0;
            ++offset;
         }
      }
   }

   // Use CV to find features in the patch:
   // Adjust the number of features to look for if we are doing adaptive correlation thresholding
   // (ACT). This would be more appropriate to call "secondary feature consideration":
//   IplImage* harris  = cvCreateImage(cvGetSize(ipl_image), 32, 1 );
//   cvCornerHarris(ipl_image, harris, 10, 5);
//   cvNamedWindow("Harris", CV_WINDOW_AUTOSIZE);
//   cvShowImage ("Harris", harris);
//   cvWaitKey();

   int num_pts = (int) config.getParameter("numFeaturesPerTile").asUint();
   CvPoint2D32f* points = (CvPoint2D32f*)cvAlloc(num_pts*sizeof(CvPoint2D32f));
   cvGoodFeaturesToTrack(ipl_image, 0, 0, points, &num_pts, quality, refPatchSize, mask,
                         averaging_block_size, use_harris, harris_k);

   // Loop over all features found in this patch and assign a feature point at that view
   // point. IMPORTANT NOTE: it is assumed that the ordering of the points[] array is by
   // descending strength of feature.
   unsigned int num_added = 0;
   for(int i=0; i<num_pts; i++)
   {
      // Correct point for the view-space offset of the search patch since points are relative to
      // patch upper left:
      CvPoint pt=cvPointFrom32f(points[i]);
      ossimDpt viewPt (pt.x, pt.y);
      viewPt += tileUL;

      // TODO REMOVE
//      ossimDpt imagePt;
//      m_refIVT->viewToImage(viewPt, imagePt);
//      CINFO<<"  Feature REF image point: "<<imagePt<<endl;

      // Good feature found, set the feature point's attributes and save to list:
      feature_points.push_back(viewPt);
      ++num_added;
   }

   if (AtpConfig::instance().diagnosticLevel(2))
   {
      if (num_added)
         CINFO<<MODULE<<"Number of features found in search patch: "<<num_added<<endl;
      else
         CINFO<<MODULE<<"No features found."<<endl;
   }

   // Cleanup mem use:
//   cvReleaseImage(&harris);
   cvReleaseImage(&mask);
   cvReleaseImage(&ipl_image);
   cvFree((void**)&points);

   return;
}

bool ossimCorrelationSource::correlate(shared_ptr<AutoTiePoint> atp)
{
   // This method returns TRUE if a match was found

   const ossimString MODULE("ossimCorrelationSource::correlate() -- ");
   AtpConfig& config = AtpConfig::instance();

   // Establish the REF patch rect centered on the viewpoint and the CMP patch rect sized by image
   // error and offset by prior residuals:
   unsigned int refPatchSize = AtpConfig::instance().getParameter("corrWindowSize").asUint();
   ossimDpt refViewPt;
   atp->getRefViewPoint(refViewPt);
   ossimIrect refPatchRect(refViewPt, refPatchSize, refPatchSize);
   ossimIrect cmpPatchRect(refViewPt, m_nominalCmpPatchSize, m_nominalCmpPatchSize);

   // Grab the image patches from the cmp and ref images.
   ossimRefPtr<ossimImageData> refpatch (m_refChain->getTile(refPatchRect, 0));
   if (!refpatch.valid())
   {
      CWARN << MODULE << "Error getting ref patch image data." << endl;
      return false;
   }

   if (config.diagnosticLevel(5))
   {
      ossimDpt refImgPt, cmpImgPt;
      m_refIVT->viewToImage(refViewPt, refImgPt);
      m_cmpIVT->viewToImage(refViewPt, cmpImgPt);
      CINFO<<"\nFeature view pt: "<<refViewPt
          <<"\n     REF img pt: "<<refImgPt
          <<"\n     CMP img pt: "<<cmpImgPt<<endl;
      refpatch->write("REF_PATCH.RAS");

      vector<double> min, max;
      refpatch->computeMinMaxPix(min, max);
      if (min.size() && max.size())
         CINFO<<"   REF min, max: "<<(int)min[0]<<", "<<(int)max[0]<<endl;
   }

   ossimDataObjectStatus stat = refpatch->getDataObjectStatus();
   if (stat != OSSIM_FULL)
   {
      if (config.diagnosticLevel(5))
         CINFO<<MODULE << "REF patch contained a null pixel. Skipping this point..."<<endl;
      return false;
   }

   ossimRefPtr<ossimImageData> cmppatch (m_cmpChain->getTile(cmpPatchRect, 0));
   if (!cmppatch.valid())
   {
      CWARN << MODULE << "Error getting cmp patch image data." << endl;
      return false;
   }
   if (cmppatch->getDataObjectStatus() != OSSIM_FULL)
   {
      if (config.diagnosticLevel(5))
         CINFO<<MODULE << "CMP patch contained a null pixel. Skipping this point..."<<endl;
      return false;
   }

   if (config.diagnosticLevel(5))
   {
      cmppatch->write("CMP_PATCH.RAS");
      vector<double> min, max;
      cmppatch->computeMinMaxPix(min, max);
      if (min.size() && max.size())
         CINFO << "   CMP min, max: " << (int) min[0] << ", " << (int) max[0]<< "\n" << endl;
   }

   // Invoke the proper correlation code to arive at the peak collection for this TP:
   bool corr_ok;
   corr_ok = OpenCVCorrelation(atp, refpatch.get(), cmppatch.get());
   int numMatches = atp->getNumMatches();

   // The peaks for the given patch pair have been established and stored in the TPs.
   if (corr_ok)
   {
      if (config.diagnosticLevel(4))
      {
         if (numMatches)
         {
            double corr_value;
            atp->getConfidenceMeasure(corr_value);
            CINFO<<MODULE<<"Match found for TP "<<atp->getTiePointId()<<" = "<<corr_value<<endl;
         }
        else
            CINFO<<MODULE<<"No match found for TP "<<atp->getTiePointId()<<endl;
      }
   }
   else // Correlation problem occurred
   {
      CWARN << MODULE << "Error encountered during correlation. Aborting correlation." << endl;
      return false;
   }

   if (config.diagnosticLevel(5))
   {
      ossimDpt cmpimgpt,refimgpt,refviewpt,cmpviewpt;
      ossimGpt refGpt;
      atp->getRefImagePoint(refimgpt);
      atp->getRefViewPoint(refviewpt);
      atp->getCmpImagePoint(cmpimgpt);
      atp->getCmpViewPoint(cmpviewpt);
      atp->getRefGroundPoint(refGpt);
      if(numMatches>0)
      {
         double val=0;
         ossimDpt residual;
         atp->getConfidenceMeasure(val);
         atp->getVectorResidual(residual);
         CINFO<<"AutoTiePoint: ("<<atp->getTiePointId()<<") has "<<numMatches<<" matches,\n"
               << "  best: (" << val << ") residual: " << residual <<"\n"
               << "  ref gpt:        "<<refGpt<<"\n"
               << "  cmpimg center:  "<<cmpimgpt<<"\n"
               << "  refimg center:  "<<refimgpt<<"\n"
               << "  cmpview center: "<<cmpviewpt<<"\n"
               << "  refview center: "<<refviewpt<<endl;
         CINFO<<endl;
      }
      else {
         CINFO<<"AutoTiePoint: (" << atp->getTiePointId() << ") has no peaks.\n"
               << "  ref gpt:        "<<refGpt<<"\n"
               << "  refimg center:  "<<refimgpt<<"\n"
               << "  refview center: "<<refviewpt<<endl;
      }
   }

   if (numMatches)
      return true;
   return false;
}

bool ossimCorrelationSource::OpenCVCorrelation(std::shared_ptr<AutoTiePoint> atp,
                                               const ossimImageData* refpatch,
                                               const ossimImageData* cmppatch)
{
   static const char* MODULE = "ossimCorrelationSource::OpenCVCorrelation() -- ";

   int ref_patch_size = refpatch->getWidth();
   int refPatchOffset = ref_patch_size / 2; // assumes square patch

   IplImage *cmpimage, *refimage, *result;
   int method;

   // Convert image data to CV IPL form:
   cmpimage = convertToIpl32(cmppatch);
   refimage = convertToIpl32(refpatch);
   if (!cmpimage || !refimage)
   {
      CWARN << MODULE << "Error encountered creating IPL image tiles. Aborting..." << endl;
      return false;
   }

   // Assign the peak threshold depending on whether it is same collect or diff collect:
   AtpConfig& config = AtpConfig::instance();
   double peak_threshold = config.getParameter("peakThreshold").asFloat();

   CvSize resdim;
   resdim.height = cmpimage->height - refimage->height + 1;
   resdim.width = cmpimage->width - refimage->width + 1;
   result = cvCreateImage(resdim, IPL_DEPTH_32F, 1);
   if (!result)
   {
      CWARN << MODULE << "Error encountered creating IPL image tile (result). Aborting..." << endl;
      return false;
   }

   // Do the actual correlation:
   unsigned int corrMethod = config.getParameter("correlationMethod").asUint();
   cvMatchTemplate(cmpimage, refimage, result, (int) corrMethod);

   if (config.diagnosticLevel(5) )
   {
      //refpatch->write("ref.ras");
      //cmppatch->write("cmp.ras");
      ossimRefPtr<ossimImageData> resultPatch =
              ossimImageDataFactory::instance()->create(this, OSSIM_UINT8, 1, resdim.width, resdim.height);
      resultPatch->initialize();
      copyIpl32ToOid(result, resultPatch.get());
      resultPatch->write("CORR.RAS");
   }
   ossimIpt cmpPeakLocV;
   float corrCoef;

   // Search through all the peaks and the tiepoint object will track which is best.
   multimap<float /* corr_coef */, ossimIpt /* loc */, greater<float> > peaks_map;
   ossimIpt cmpPatchLocV (cmppatch->getImageRectangle().ul());
   for (int dy = 0; dy < result->height; dy++)
   {
      for (int dx = 0; dx < result->width; dx++)
      {
         // Fetch the pixel's corr coef and insert in map to order by strongest peak:
         corrCoef = ((float*) (result->imageData + result->widthStep * dy))[dx];
         if (corrCoef >= peak_threshold)
         {
            cmpPeakLocV.x = cmpPatchLocV.x + refPatchOffset + dx;
            cmpPeakLocV.y = cmpPatchLocV.y + refPatchOffset + dy;
            peaks_map.insert(pair<float, ossimIpt>(corrCoef, cmpPeakLocV));
         }
      }
   }

   if (peaks_map.size() > 0)
   {
      // The peaks are in order by greatest correlation value. Send the best to the CTP:
      multimap<float, ossimIpt, greater<float> >::iterator peak = peaks_map.begin();
      unsigned int maxNumPeaks = AtpConfig::instance().getParameter("maxNumMatchesPerFeature").asUint();
      if (config.paramExists("maxNumPeaksPerFeature")) // support legacy keywords
         maxNumPeaks = config.getParameter("maxNumPeaksPerFeature").asUint();
      for (unsigned int i = 0; (i < maxNumPeaks) && (peak != peaks_map.end()); i++)
      {
         cmpPeakLocV = peak->second;
         corrCoef = peak->first;
         atp->addViewMatch(cmpPeakLocV, corrCoef);
         peak++;
      }
   }

   cvReleaseImage(&result);
   cvReleaseImage(&refimage);
   cvReleaseImage(&cmpimage);

   return true;
}

}


