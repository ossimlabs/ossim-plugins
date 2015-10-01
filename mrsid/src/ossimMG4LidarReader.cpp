//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description:  Class definition for MRSID MG4 reader.
//
// Author: Mingjie Su
//
//----------------------------------------------------------------------------
// $Id: ossimMG4LidarReader.cpp 2645 2011-05-26 15:21:34Z oscar.kramer $

#include <fstream>
#include <iostream>
#include <string>

//ossim includes
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimEndian.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimIoStream.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimScalarTypeLut.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimUnitTypeLut.h>

#include <ossim/imaging/ossimImageDataFactory.h>
#include <ossim/imaging/ossimImageGeometryRegistry.h>

#include <ossim/projection/ossimProjection.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <ossim/projection/ossimEpsgProjectionFactory.h>
#include <ossim/projection/ossimUtmProjection.h>

#include <ossim/support_data/ossimAuxFileHandler.h>
#include <ogr_spatialref.h>

#include "ossimMG4LidarReader.h"

#ifdef OSSIM_ID_ENABLED
static const char OSSIM_ID[] = "$Id";
#endif

// Resolution Ratio between adjacent levels.

static double maxRasterSize = 2048.0;
static double maxBlockSideSize = 1024.0;

static ossimTrace traceDebug("ossimMG4LidarReader:debug");
static ossimTrace traceDump("ossimMG4LidarReader:dump");
//static ossimOgcWktTranslator wktTranslator;

RTTI_DEF1_INST(ossimMG4LidarReader,
   "ossimMG4LidarReader",
   ossimImageHandler)

   ossimMG4LidarReader::ossimMG4LidarReader()
   : ossimImageHandler(),
   m_reader(NULL),
   m_imageRect(),
   m_numberOfSamples(0),
   m_numberOfLines(0),
   m_scalarType(OSSIM_SCALAR_UNKNOWN),
   m_tile(0)
{
}

ossimMG4LidarReader::~ossimMG4LidarReader()
{
   closeEntry();
}

ossimString ossimMG4LidarReader::getShortName()const
{
   return ossimString("ossim_mg4_lidar_reader");
}

ossimString ossimMG4LidarReader::getLongName()const
{
   return ossimString("ossim mg4 lidar reader");
}

ossimString ossimMG4LidarReader::getClassName()const
{
   return ossimString("ossimMG4LidarReader");
}

ossim_uint32 ossimMG4LidarReader::getNumberOfDecimationLevels()const
{
   ossim_uint32 result = 1; // Add r0

   if (theOverview.valid())
   {
      //---
      // Add external overviews.
      //---
      result += theOverview->getNumberOfDecimationLevels();
   }

   return result;
}

ossim_uint32 ossimMG4LidarReader::getNumberOfLines(
   ossim_uint32 resLevel) const
{
   if (theOverview.valid())
   {
      return theOverview->getNumberOfSamples(resLevel);
   }
   return m_numberOfLines;
}

ossim_uint32 ossimMG4LidarReader::getNumberOfSamples(
   ossim_uint32 resLevel) const
{
   if (theOverview.valid())
   {
      return theOverview->getNumberOfSamples(resLevel);
   }
   return m_numberOfSamples;
}

bool ossimMG4LidarReader::open()
{
   static const char MODULE[] = "ossimMG4LidarReader::open";

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " entered...\n"
         << "image: " << theImageFile << "\n";
   }

   bool result = false;

   if(isOpen())
   {
      closeEntry();
   }

   int gen = 0;
   bool raster = false;
   result = Version::getMrSIDFileVersion(theImageFile.c_str(), gen, raster);
   if (result == false)
   {
      return result;
   }

   m_reader = MG4PointReader::create();
   m_reader->init(theImageFile.c_str());
   m_bounds = m_reader->getBounds();

   int numPoints = m_reader->getNumPoints();
   m_numberOfSamples = m_bounds.x.length();
   m_numberOfLines = m_bounds.y.length();

   double pts_per_area = ((double)m_reader->getNumPoints())/(m_bounds.x.length()*m_bounds.y.length());
   double average_pt_spacing = sqrt(1.0 / pts_per_area) ;
   double cell_side = average_pt_spacing;
   maxRasterSize = std::max(m_bounds.x.length()/cell_side, m_bounds.y.length()/cell_side);
   //openZoomLevel(0);

   //get data type from X channel as default
   getDataType(0);

   if (m_scalarType != OSSIM_SCALAR_UNKNOWN)
   {
      m_tile = ossimImageDataFactory::instance()->create(this, this);

      m_tile->initialize();

      // Call the base complete open to pick up overviews.
      completeOpen();

      result = true;
   }

   if (result == false)
   {
      closeEntry();
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " exit status = " << (result?"true":"false\n")
         << std::endl;
   }

   return result;
}

void ossimMG4LidarReader::openZoomLevel(ossim_int32 zoom)
{
   // geo dimensions
   const double gWidth = m_reader->getBounds().x.length() ;
   const double gHeight = m_reader->getBounds().y.length() ;

   // geo res
   double xRes = pow(2.0, zoom) * gWidth / maxRasterSize ;
   double yRes = pow(2.0, zoom) * gHeight / maxRasterSize ;
   xRes = yRes = std::max(xRes, yRes);

   // pixel dimensions
   m_numberOfSamples  = static_cast<int>(gWidth / xRes  + 0.5);
   m_numberOfLines  = static_cast<int>(gHeight / yRes + 0.5);
}

bool ossimMG4LidarReader::isOpen()const
{
   return m_tile.get();
}

void ossimMG4LidarReader::closeEntry()
{
   m_tile = 0;

   if (m_reader)
   {
      RELEASE(m_reader);
      m_reader = 0;
   }
   ossimImageHandler::close();
}

ossimRefPtr<ossimImageData> ossimMG4LidarReader::getTile(
   const ossimIrect& rect, ossim_uint32 resLevel)
{
   // This tile source bypassed, or invalid res level, return null tile.
   if(!isSourceEnabled() || !isOpen() || !isValidRLevel(resLevel))
   {
      return ossimRefPtr<ossimImageData>();
   }

   ossimIrect imageBound = getBoundingRect(resLevel);
   if(!rect.intersects(imageBound))
   {
      return ossimRefPtr<ossimImageData>();
   }

   // Check for overview.
   if( resLevel > 0 )
   {
      if(theOverview.valid())
      {
         ossimRefPtr<ossimImageData> tileData = theOverview->getTile(rect, resLevel);
         tileData->setScalarType(getOutputScalarType());
         return tileData;
      }
   }

   m_tile->setImageRectangle(rect);

   // Compute clip rectangle with respect to the image bounds.
   ossimIrect clipRect   = rect.clipToRect(imageBound);

   if (rect.completely_within(clipRect) == false)
   {
      // Not filling whole tile so blank it out first.
      m_tile->makeBlank();
   }

   // access the point cloud with PointSource::read()
   const ossimDpt UL_PT(m_bounds.x.min + clipRect.ul().x, m_bounds.y.min + clipRect.ul().y);
   const ossimDpt LR_PT(m_bounds.x.min + clipRect.ul().x + clipRect.width(), m_bounds.y.min + clipRect.ul().y + clipRect.height());

   Bounds bounds(UL_PT.lon, LR_PT.lon, 
      UL_PT.lat, LR_PT.lat,
      -HUGE_VAL, +HUGE_VAL);

   ossim_float32 fraction = 1.0/pow(2.0, (double)resLevel);

   PointIterator* iter = m_reader->createIterator(bounds, fraction, m_reader->getPointInfo(), NULL);

   // create a buffer to store the point data
   PointData points;
   points.init(m_reader->getPointInfo(), clipRect.width()*clipRect.height());

   size_t count = 0;
   ossimDpt lasPt;
   double* dataValues = new double[clipRect.width()*clipRect.height()];
   while((count = iter->getNextPoints(points)) != 0)
   {
      //loop through each point
      for(size_t i = 0; i < count; i += 1)
      {
         const ChannelData* channelX = points.getChannel(CHANNEL_NAME_X);
         const ChannelData* channelY = points.getChannel(CHANNEL_NAME_Y);
         const ChannelData* channelZ = points.getChannel(CHANNEL_NAME_Z);

         const void *dataX = channelX->getData();
         const void *dataY = channelY->getData();
         const void *dataZ = channelZ->getData();

         lasPt.x = static_cast<const double*>(dataX)[i];
         lasPt.y = static_cast<const double*>(dataY)[i];

         ossim_int32 line = static_cast<ossim_int32>(lasPt.y - UL_PT.y);
         ossim_int32 samp = static_cast<ossim_int32>(lasPt.x - UL_PT.x);
         ossim_int32 bufIndex = line * clipRect.width() + samp;

         if (bufIndex < clipRect.width()*clipRect.height())
         {
            dataValues[bufIndex] = static_cast<const double*>(dataZ)[i];
         }
      }
   }
   RELEASE(iter);

   m_tile->loadBand((void*)dataValues, clipRect, 0);
   m_tile->validate();

   delete[] dataValues;

   return m_tile;
}

ossim_uint32 ossimMG4LidarReader::getNumberOfInputBands() const
{
   return 1;
}

ossim_uint32 ossimMG4LidarReader::getNumberOfOutputBands()const
{
   return 1;
}

ossim_uint32 ossimMG4LidarReader::getImageTileWidth() const
{
   return 0;
}

ossim_uint32 ossimMG4LidarReader::getImageTileHeight() const
{
   return 0;
}

ossimScalarType ossimMG4LidarReader::getOutputScalarType() const
{
   return m_scalarType;
}

ossimRefPtr<ossimImageGeometry> ossimMG4LidarReader::getImageGeometry()
{
   if ( !theGeometry )
   {
      //---
      // Check for external geom:
      //---
      theGeometry = getExternalImageGeometry();

      if ( !theGeometry )
      {
         //---
         // Check the internal geometry first to avoid a factory call.
         //---
         theGeometry = getInternalImageGeometry();

         // At this point it is assured theGeometry is set.

         // Check for set projection.
         if ( !theGeometry->getProjection() )
         {
            // Try factories for projection.
            ossimImageGeometryRegistry::instance()->extendGeometry(this);
         }
      }

      // Set image things the geometry object should know about.
      initImageParameters( theGeometry.get() );
   }

   return theGeometry;
}

ossimRefPtr<ossimImageGeometry> ossimMG4LidarReader::getInternalImageGeometry() const
{
   static const char MODULE[] = "ossimMG4LidarReader::getInternalImageGeometry";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   ossimRefPtr<ossimImageGeometry> geom = new ossimImageGeometry();

   // Must cast away constness.
   ossimMG4LidarReader* th = const_cast<ossimMG4LidarReader*>(this);
   ossimRefPtr<ossimProjection> proj = th->getGeoProjection();

   geom->setProjection(proj.get());

   return geom;
}

bool ossimMG4LidarReader::loadState(const ossimKeywordlist& kwl,
   const char* prefix)
{
   bool result = false;

   if ( ossimImageHandler::loadState(kwl, prefix) )
   {
      result = open();
   }

   return result;
}

void ossimMG4LidarReader::getDataType(ossim_int32 channelId) 
{
   if (m_reader)
   {
      DataType pixelType = m_reader->getChannel(channelId).getDataType();
      switch (pixelType)
      {
      case DATATYPE_UINT8:
         {
            m_scalarType = OSSIM_UINT8;
            break;
         }
      case DATATYPE_SINT8:
         {
            m_scalarType = OSSIM_SINT8;
            break;
         }
      case DATATYPE_UINT16:
         {
            m_scalarType = OSSIM_UINT16;
            break;
         }
      case DATATYPE_SINT16:
         {
            m_scalarType = OSSIM_SINT16;
            break;
         }
      case DATATYPE_UINT32:
         {
            m_scalarType = OSSIM_UINT32;
            break;
         }
      case DATATYPE_SINT32:
         {
            m_scalarType = OSSIM_SINT32;
            break;
         }
      case DATATYPE_FLOAT32:
         {
            m_scalarType = OSSIM_FLOAT32;
            break;
         }
      case DATATYPE_FLOAT64:
         {
            m_scalarType = OSSIM_FLOAT64;
            break;
         }
      default:
         {
            m_scalarType = OSSIM_SCALAR_UNKNOWN;
            break;
         }
      }
   }
}

ossimProjection* ossimMG4LidarReader::getGeoProjection()
{
   ossimProjection* proj = NULL;
   if (m_reader)
   {
      const char* wkt = m_reader->getWKT();
      if (wkt)
      {
         OGRSpatialReferenceH  hSRS = NULL;

         //Translate the WKT into an OGRSpatialReference. 
         hSRS = OSRNewSpatialReference(NULL);
         if(OSRImportFromWkt( hSRS, (char**)&wkt) != OGRERR_NONE)
         {
            OSRDestroySpatialReference( hSRS );
            return NULL;
         }

         //Determine if State Plane Coordinate System
         const char* epsg_code = OSRGetAttrValue(hSRS, "AUTHORITY", 1);
         proj = ossimProjectionFactoryRegistry::instance()->createProjection(ossimString(epsg_code));

         //get unit type
         ossimUnitType unitType = OSSIM_METERS;
         const char* units = OSRGetAttrValue(hSRS, "UNIT", 0);
         bool bGeog = OSRIsGeographic(hSRS);
         if (bGeog == false)
         {
            if ( units != NULL )
            {
               // Down case to avoid case mismatch.
               ossimString s = units;
               s.downcase();

               if( ( s == ossimString("us survey foot") ) ||
                  ( s == ossimString("u.s. foot") ) ||
                  ( s == ossimString("foot_us") ) )
               {
                  unitType = OSSIM_US_SURVEY_FEET;
               }
               else if( s == ossimString("degree") )
               {
                  unitType = OSSIM_DEGREES;
               }
               else if( s == ossimString("feet") )
               {
                  unitType = OSSIM_FEET;
               }
            }
         }
         else
         {
            unitType = OSSIM_DEGREES;
         }

         //set ul tie point and gsd
         ossimMapProjection* mapProj = dynamic_cast<ossimMapProjection*>(proj);
         if (mapProj)
         {
            double xMin = m_reader->getBounds().x.min;
            double yMax = m_reader->getBounds().y.max;
            double xRes = m_reader->getBounds().x.length()/m_numberOfSamples;
            double yRes = m_reader->getBounds().y.length()/m_numberOfLines;
            ossimDpt gsd(xRes, yRes);
            if (mapProj->isGeographic())
            {
               ossimGpt tie(yMax, xMin);
               mapProj->setUlTiePoints(tie);
               mapProj->setDecimalDegreesPerPixel(gsd);
            }
            else
            {
               ossimDpt tie(xMin, yMax);
               if ( unitType == OSSIM_US_SURVEY_FEET)
               {
                  gsd = gsd * US_METERS_PER_FT;
                  tie = tie * US_METERS_PER_FT;
               }
               else if ( unitType == OSSIM_FEET )
               {
                  gsd = gsd * MTRS_PER_FT;
                  tie = tie * MTRS_PER_FT;
               }
               mapProj->setUlTiePoints(tie);
               mapProj->setMetersPerPixel(gsd);
            }
         }
         return proj;
      }
   }
   return proj;
}

template<typename DTYPE>
const DTYPE ossimMG4LidarReader::getChannelElement(const ChannelData* channel, size_t idx)
{
   DTYPE retval = static_cast<DTYPE>(0);
   switch (channel->getDataType())
   {
   case (DATATYPE_FLOAT64):
      retval = static_cast<DTYPE>(static_cast<const double*>(channel->getData())[idx]);
      break;
   case (DATATYPE_FLOAT32):
      retval = static_cast<DTYPE>(static_cast<const float *>(channel->getData())[idx]);
      break;
   case (DATATYPE_SINT32):
      retval = static_cast<DTYPE>(static_cast<const long *>(channel->getData())[idx]);
      break;
   case (DATATYPE_UINT32):
      retval = static_cast<DTYPE>(static_cast<const unsigned long *>(channel->getData())[idx]);
      break;
   case (DATATYPE_SINT16):
      retval = static_cast<DTYPE>(static_cast<const short *>(channel->getData())[idx]);
      break;
   case (DATATYPE_UINT16):
      retval = static_cast<DTYPE>(static_cast<const unsigned short *>(channel->getData())[idx]);
      break;
   case (DATATYPE_SINT8):
      retval = static_cast<DTYPE>(static_cast<const char *>(channel->getData())[idx]);
      break;
   case (DATATYPE_UINT8):
      retval = static_cast<DTYPE>(static_cast<const unsigned char *>(channel->getData())[idx]);
      break;
   case (DATATYPE_SINT64):
      retval = static_cast<DTYPE>(static_cast<const GIntBig *>(channel->getData())[idx]);
      break;
   case (DATATYPE_UINT64):
      retval = static_cast<DTYPE>(static_cast<const GUIntBig *>(channel->getData())[idx]);
      break;
   default:
      break;
   }
   return retval;
}

