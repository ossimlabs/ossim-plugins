//*****************************************************************************
// License:  See top level LICENSE.txt file.
//
// DESCRIPTION:
//	OSSIM sensor model plugin for CSM version 3 plugin.
//	CSM3 adjustable parameters can be used by setting
//	USE_INTERNAL_ADJUSTABLE_PARAMS to 1. Otherwise a default
// 	2 parameters intract/crosstrack adjustment is used.
//	CSM3 API also defines valid image domain and height range.
//	This implementaion ignores those and does not check for 
// 	valid input range.
//
// Author:  cchuah
//
//*******************************************************************
//  $Id: ossimCsm3SensorModel.cpp 1680 2016-01-12 16:27:39Z cchuah $

#include "ossimCsm3SensorModel.h"
#include "ossimMspLoader.h"
#include <ossim/elevation/ossimElevManager.h>
#include <ossim/support_data/ossimNitfFile.h>
#include <ossim/support_data/ossimNitfImageHeader.h>
#include <ossim/base/ossimIpt.h>
#include <csm/Plugin.h>

//enable internal sensor model adjustment if implemented.  Not throughly tested.
#define USE_INTERNAL_ADJUSTABLE_PARAMS 0

// default adjustable parameters if not using internal sensor model adjustments
static const ossimString PARAM_NAMES[] ={"intrack_offset",
                                         "crtrack_offset"};
static const ossimString PARAM_UNITS[] ={"pixel",
                                         "pixel"};


using namespace csm;

RTTI_DEF1(ossimCsm3SensorModel, "ossimCsm3SensorModel", ossimSensorModel);

ossimCsm3SensorModel::ossimCsm3SensorModel()
: m_imageFile(""),
  m_pluginName(""),
  m_sensorName(""),
  m_modelIsAdjustable(true),
  theIntrackOffset(0.0),
  theCrtrackOffset(0.0)
{
}

ossimCsm3SensorModel::ossimCsm3SensorModel(const ossimString& pluginName, 
                                           const ossimString& sensorName,
                                           const ossimString& imageFile,
                                           RasterGM* model)
: m_model(model),
  m_imageFile(imageFile), 
  m_pluginName(pluginName),
  m_sensorName(sensorName),
  m_modelIsAdjustable(true),
  theIntrackOffset(0.0),
  theCrtrackOffset(0.0)
{
#if USE_INTERNAL_ADJUSTABLE_PARAMS
   // check for adjustability
   if (m_model->getNumParameters() == 0)
      m_modelIsAdjustable = false;
#endif

   initializeModel();
}

ossimCsm3SensorModel::ossimCsm3SensorModel(const ossimCsm3SensorModel& src)
: ossimSensorModel(src),
  m_imageFile(src.m_imageFile), 
  m_pluginName(src.m_pluginName),
  m_sensorName(src.m_sensorName),
  m_modelIsAdjustable(true),
  theIntrackOffset(src.theIntrackOffset),
  theCrtrackOffset(src.theCrtrackOffset)
{
   // unfortunately there is no copy constructor for csm models, so we get the
   // original sensor state and construct from it

   // can they all be constructed from state?
   string srcState = src.m_model->getModelState();
   m_model.reset(ossimMspLoader::loadModelFromState(srcState));
}



ossimCsm3SensorModel::~ossimCsm3SensorModel()
{
   m_model.reset();
}


// CSM has a valid height range obtained by getValidHeightRange()
// This method does not check for valid height
void ossimCsm3SensorModel::lineSampleHeightToWorld(const ossimDpt& image_point,
                                                   const double&   heightEllipsoid,
                                                   ossimGpt&       worldPoint) const
{
   if (!insideImage(image_point))
   {
      worldPoint.makeNan();
      worldPoint = extrapolate(image_point, heightEllipsoid);
   }
   else
   {
      double desiredPrecision = 0.001;
      double* achievedPrecision = NULL;
      WarningList warnings;

      ImageCoord imagePt;
      imagePt.samp = image_point.x - theCrtrackOffset;
      imagePt.line = image_point.y - theIntrackOffset;

      EcefCoord ecfGpt = m_model->imageToGround(imagePt, heightEllipsoid, desiredPrecision,
                                                achievedPrecision, &warnings);

      if (warnings.size() > 0)
         ossimNotify(ossimNotifyLevel_WARN)
         << "lineSampleHeightToWorld: " << warnings.begin()->getMessage()
         << std::endl;

      worldPoint = ossimGpt(ossimEcefPoint(ecfGpt.x, ecfGpt.y, ecfGpt.z));
   }
}


void ossimCsm3SensorModel::worldToLineSample(const ossimGpt& worldPoint,
                                             ossimDpt&       ip) const
{
   if(worldPoint.isLatNan() || worldPoint.isLonNan())
   {
      ip.makeNan();
      return;
   }

   double desiredPrecision = 0.001;
   double* achievedPrecision = NULL;
   WarningList warnings;

   ossimEcefPoint ecfGpt(worldPoint);
   EcefCoord groundPt(ecfGpt.x(), ecfGpt.y(), ecfGpt.z());
   ImageCoord imagePt = m_model->groundToImage(groundPt, desiredPrecision,
                                               achievedPrecision, &warnings);

   if (warnings.size() > 0)
      ossimNotify(ossimNotifyLevel_WARN)
      << "worldToLineSample: " << warnings.begin()->getMessage()
      << std::endl;

   ip = ossimDpt(imagePt.samp + theCrtrackOffset, imagePt.line + theIntrackOffset);
}


void ossimCsm3SensorModel::imagingRay(const ossimDpt& image_point,
                                      ossimEcefRay&   image_ray) const
{
   // std::cout << "imaging Ray .................................\n";
   if(m_model)
   {
      double AP = 0.0;
      EcefLocus ecefLocus = m_model->imageToRemoteImagingLocus(ImageCoord(image_point.y, image_point.x),  AP);
      ossimEcefVector v(ecefLocus.direction.x, ecefLocus.direction.y, ecefLocus.direction.z);   
      image_ray.setOrigin(ossimEcefPoint(ecefLocus.point.x, ecefLocus.point.y, ecefLocus.point.z));
      image_ray.setDirection(v);
   } 



#if 0
   ossimGpt start;
   ossimGpt end;

   // get valid height range
   std::pair<double,double> hgtRange = m_model->getValidHeightRange();

   // use the upper height limit and ellipsoid surface to establish the ray
   lineSampleHeightToWorld(image_point, hgtRange.second, start);
   lineSampleHeightToWorld(image_point, 0.0, end);

   image_ray = ossimEcefRay(start, end);

   return;
#endif
}

void ossimCsm3SensorModel::updateModel()
{
   if(!m_model)
      return;

   if (!m_modelIsAdjustable)
      return;

#if USE_INTERNAL_ADJUSTABLE_PARAMS
   int nParams = getNumberOfAdjustableParameters();
   for(int idx = 0; idx < nParams; ++idx)
      m_model->setParameterValue(idx, computeParameterOffset(idx));
#else
   theIntrackOffset  = computeParameterOffset(INTRACK_OFFSET);
   theCrtrackOffset  = computeParameterOffset(CRTRACK_OFFSET);
#endif
}

//  This method returns the partial derivatives of line and sample
//  (in pixels per the applicable model parameter units), respectively,
//  with respect to the model parameter given by index at the given
//  groundPt (x,y,z in ECEF meters).
ossimDpt ossimCsm3SensorModel::computeSensorPartials(int index, const ossimEcefPoint& ecefPt) const
{
   double desiredPrecision = 0.001;
   double* achievedPrecision = NULL;
   WarningList warnings;

   RasterGM::SensorPartials partial = m_model->computeSensorPartials( index,
                                                                      EcefCoord(ecefPt.x(), ecefPt.y(), ecefPt.z()),
                                                                      desiredPrecision , achievedPrecision, &warnings) ;

   if (warnings.size() > 0)
      ossimNotify(ossimNotifyLevel_WARN)
      << "computeSensorPartials: " << warnings.begin()->getMessage()
      << std::endl;

   return ossimDpt(partial.first, partial.second);
}


// This method returns the partial derivatives of line and sample  (a six elements vector)
//  (in pixels per meter) with respect to the given ecefPt
std::vector<double> ossimCsm3SensorModel::computeGroundPartials(const ossimEcefPoint& ecefPt) const
{
   return m_model->computeGroundPartials(EcefCoord(ecefPt.x(), ecefPt.y(), ecefPt.z()));
}


// Initializes the adjustable parameter-related base-class data members to defaults.
//  For this model, adjustment '0' is ALWAYS equivalent to the initial condition as specified by 
//  the metadata.
// check to see if internal model is adjustable.  If not, initialize a two parameter adjustment
// providing only line and sample adjustments
void ossimCsm3SensorModel::initAdjustableParameters()
{
   if (getNumberOfAdjustments())
   {
      // Geometry was already initialized with initial condition "adjustment". So just set current
      // adjustment to the initial:
      setCurrentAdjustment(0);
   }
   else
   {
      // No prior adjustment has been defined, so create the initial:
      if (m_modelIsAdjustable)
      {
#if USE_INTERNAL_ADJUSTABLE_PARAMS
         //int numAdjParams = m_model->getNumParameters();
         resizeAdjustableParameterArray(m_model->getNumParameters());
         int numParams = getNumberOfAdjustableParameters();
         setAdjustmentDescription("INITIAL_CONDITION");
         for (int i=0; i<numParams; i++)
         {
            setParameterDescription(i, m_model->getParameterName(i));
            setParameterUnit(i, m_model->getParameterUnits(i));
            setParameterCenter(i, 0.0);
            // parameter sigma is calculated from parameter covariance
            double variance = m_model->getParameterCovariance(i, i);
            setAdjustableParameter(i, 0.0, sqrt(variance));
         }
#else
         // initialize the default 2 adjustable parameters
         resizeAdjustableParameterArray(NUM_ADJUSTABLE_PARAMS);
         int numParams = getNumberOfAdjustableParameters();
         setAdjustmentDescription("DEFAULT_INITIAL_CONDITION");
         for (int i=0; i<numParams; i++)
         {
            setAdjustableParameter(i, 0.0);
            setParameterDescription(i, PARAM_NAMES[i]);
            setParameterUnit(i,PARAM_UNITS[i]);
         }
         setParameterSigma(INTRACK_OFFSET, 50.0);
         setParameterSigma(CRTRACK_OFFSET, 50.0);
#endif
      }
   }
}

//  This method initializes the base class adjustable parameter and associated
//  sigmas arrays with quantities specific to this model. Adjustment 0 is 
//  considered the initial state of the parameters.
void ossimCsm3SensorModel::initializeModel()
{
   ossimNotify(ossimNotifyLevel_INFO) << "initializing ossimCsm3SensorModel\n" << std::endl;

   // this model has not been adjusted
   if (getNumberOfAdjustments() == 0)
      initAdjustableParameters();  // serves as initial condition

   int current_adj = getCurrentAdjustmentIdx();
   if (current_adj != 0)
      setCurrentAdjustment(0);

   //if (m_sensorName == "RSM")
   //{
   //    // the getImageSize() crashes for RSM
   //    //ImageVector size = model->getImageSize();
   //    // workaround using nitf header
   //    ossimRefPtr<ossimNitfFile> file = new ossimNitfFile;
   //    if(!file->parseFile(m_imageFile))
   //        setErrorStatus();
   //
   //    // get first image header.  there can only be one sensor in each nitf.
   //    ossimRefPtr<ossimNitfImageHeader> ih = file->getNewImageHeader(0);
   //    if(!ih)
   //        setErrorStatus();
   //
   //    //theImageID = ih->getImageId();
   //
   //    ossimIrect imageRect = ih->getImageRect();
   //    theImageSize = ossimIpt(imageRect.width(), imageRect.height());
   //}
   //else

   ImageVector size = m_model->getImageSize();
   theImageSize = ossimIpt(size.samp, size.line);
   ossimNotify(ossimNotifyLevel_INFO) << "Csm3Sensor image size: " << theImageSize << std::endl;

   // Note that the model might not be valid over the entire imaging operation.
   // Use getValidImageRange() to get the valid range of image coordinates.
   std::pair<ImageCoord,ImageCoord> valSize = m_model->getValidImageRange();
   double l0 = valSize.first.line;
   double s0 = valSize.first.samp;
   double l1 = valSize.second.line;
   double s1 = valSize.second.samp;

   theImageClipRect = ossimDrect(ossimDpt(valSize.first.samp,valSize.first.line),
                                 ossimDpt(valSize.second.samp,valSize.second.line));

   ossimDrect fullImgRect = ossimDrect(ossimDpt(0, 0),
                                       ossimDpt(theImageSize.samp-1, theImageSize.line-1));
   ossimNotify(ossimNotifyLevel_INFO) << "Csm3Sensor Valid Image Range: " << fullImgRect << std::endl;

   // set theImageClipRect to at most the full image
   theImageClipRect = theImageClipRect.clipToRect(fullImgRect);

   theSubImageOffset = ossimDpt(0,0);  // pixels

   // Assign the bounding ground polygon:
   ossimGpt v0, v1, v2, v3;
   ossimDpt ip0 (0.0, 0.0);
   lineSampleToWorld(ip0, v0);
   ossimDpt ip1 (theImageSize.samp-1.0, 0.0);
   lineSampleToWorld(ip1, v1);
   ossimDpt ip2 (theImageSize.samp-1.0, theImageSize.line-1.0);
   lineSampleToWorld(ip2, v2);
   ossimDpt ip3 (0.0, theImageSize.line-1.0);
   lineSampleToWorld(ip3, v3);

   theBoundGndPolygon
   = ossimPolygon (ossimDpt(v0), ossimDpt(v1), ossimDpt(v2), ossimDpt(v3));

   // get the ref image point and ground point
   EcefCoord refEcefPt = m_model->getReferencePoint();
   theRefGndPt = ossimGpt(ossimEcefPoint(refEcefPt.x, refEcefPt.y, refEcefPt.z));
   ossimNotify(ossimNotifyLevel_INFO) << "Csm3Sensor ref Ground Pt: " << theRefGndPt << std::endl;

   double desiredPrecision = 0.001;
   double* achievedPrecision = NULL;
   WarningList warnings;
   ImageCoord refImgPt = m_model->groundToImage(refEcefPt, desiredPrecision,
                                                achievedPrecision, &warnings);
   ossimNotify(ossimNotifyLevel_INFO) << "Csm3Sensor ref Image Pt: " <<
         ossimDpt(refImgPt.samp, refImgPt.line)  << std::endl;
   if (warnings.size() > 0)
      ossimNotify(ossimNotifyLevel_WARN)
      << "initializeModel: Computing refImgPt:\n" << warnings.begin()->getMessage()
      << std::endl;
   theRefImgPt = ossimDpt(refImgPt.samp, refImgPt.line);

   // calculate gsd
   try
   {
      computeGsd();
   }
   catch (const ossimException& e)
   {
      ossimNotify(ossimNotifyLevel_WARN)
                  << "initializeModel:: computeGsd exception:\n"
                  << e.what() << std::endl;
   }

   // Indicate that all params need to be recomputed:
   initChangeFlags();
   updateModel();
}


bool ossimCsm3SensorModel::saveState(ossimKeywordlist& kwl,  const char* prefix) const
{
   bool result = ossimSensorModel::saveState(kwl, prefix);

   if(result)
   {
      kwl.add(prefix, "plugin_name", m_pluginName, true);
      kwl.add(prefix, "sensor_name", m_sensorName, true);
      kwl.add(prefix, "image_file", m_imageFile, true);
      // saving csm state
      string state =  m_model->getModelState();
      kwl.add(prefix, "csm_sensor_state", ossimString(state));
   }

   return result;
}

bool ossimCsm3SensorModel::loadState(const ossimKeywordlist& kwl, const char* prefix)
{
   bool result = ossimSensorModel::loadState(kwl, prefix);

   if(result)
   {
      m_pluginName = kwl.find(prefix, "plugin_name");
      m_sensorName = kwl.find(prefix, "sensor_name");
      m_imageFile  = kwl.find(prefix, "image_file");

      // restore from csm3 sensor state
      ossimString sensorState  = kwl.find(prefix, "csm_sensor_state");
      m_model.reset(ossimMspLoader::loadModelFromState(sensorState));
   }

   return result;
}
