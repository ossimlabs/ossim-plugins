 //----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description:  Class definition for MRSID reader.
//
//----------------------------------------------------------------------------
// $Id: ossimMrSidReader.cpp 2645 2011-05-26 15:21:34Z oscar.kramer $


#include "ossimMrSidReader.h"

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
#include <ossim/support_data/ossimAuxXmlSupportData.h>
#include <ossim/support_data/ossimWkt.h>

//LizardTech Includes
#include <lt_base.h>
#include <lt_fileSpec.h>
#include <lt_ioFileStream.h>
#include <lti_imageReader.h>
#include <lti_pixel.h>
#include <lti_scene.h>
#include <lti_sceneBuffer.h>
#include <lt_fileSpec.h>
#include <lti_geoCoord.h>
#include <lt_ioFileStream.h>
#include <lti_pixelLookupTable.h>
#include <lti_utils.h>
#include <lti_metadataDatabase.h>
#include <lti_metadataRecord.h>

// System:
#include <fstream>
#include <iostream>
#include <string>

#ifdef OSSIM_ID_ENABLED
static const char OSSIM_ID[] = "$Id";
#endif
 
static ossimTrace traceDebug("ossimMrSidReader:debug");
static ossimTrace traceDump("ossimMrSidReader:dump");
//static ossimOgcWktTranslator wktTranslator;

RTTI_DEF1_INST(ossimMrSidReader,
               "ossimMrSidReader",
               ossimImageHandler)
 
ossimMrSidReader::ossimMrSidReader()
   : ossimImageHandler(),
     theReader(0),
     theImageNavigator(0),
     theMinDwtLevels(0),
     theImageRect(),
     theMrSidDims(),
     theNumberOfBands(0),
     theScalarType(OSSIM_SCALAR_UNKNOWN),
     theTile(0)
{
}

ossimMrSidReader::~ossimMrSidReader()
{
   closeEntry();

   // Note: Not a ref ptr.  This is a  MrSIDImageReader*.
   if ( theReader )
   {
      theReader->release();
      theReader = 0;
   }
   if (theImageNavigator != 0)
   {
      delete theImageNavigator;
      theImageNavigator = 0;
   }
}

ossimString ossimMrSidReader::getShortName()const
{
   return ossimString("ossim_mrsid_reader");
}

ossimString ossimMrSidReader::getLongName()const
{
   return ossimString("ossim mrsid reader");
}

ossimString ossimMrSidReader::getClassName()const
{
   return ossimString("ossimMrSidReader");
}

void ossimMrSidReader::getDecimationFactor(ossim_uint32 resLevel,
                                           ossimDpt& result) const
{
   if (theGeometry.valid())
   {
      theGeometry->decimationFactor(resLevel, result);
   }
   else
   {
      result.makeNan();
   }
}

void ossimMrSidReader::getDecimationFactors(
   std::vector<ossimDpt>& decimations) const
{
   if (theGeometry.valid())
   {
      if ( theGeometry->getNumberOfDecimations() )
      {
         theGeometry->decimationFactors(decimations);
      }
      else
      {
         // First time called...
         if ( computeDecimationFactors(decimations) )
         {
            theGeometry->setDiscreteDecimation(decimations);
         }
         else
         {
            decimations.clear();
         }
      }
   }
   else
   {
      if ( !computeDecimationFactors(decimations) )
      {
         decimations.clear();
      }
   }
}

ossim_uint32 ossimMrSidReader::getNumberOfDecimationLevels()const
{
   ossim_uint32 result = 1; // Add r0
   
   if (theMinDwtLevels)
   {
      //---
      // Add internal overviews.
      //---
      result += theMinDwtLevels;
   }

   if (theOverview.valid())
   {
      //---
      // Add external overviews.
      //---
      result += theOverview->getNumberOfDecimationLevels();
   }

   return result;
}

ossim_uint32 ossimMrSidReader::getNumberOfLines(
   ossim_uint32 resLevel) const
{
   ossim_uint32 result = 0;
   if ( isValidRLevel(resLevel) )
   {
      if (resLevel <= theMinDwtLevels)
      {
         if (theMrSidDims.size() > 0)
         {
            result = theMrSidDims[resLevel].height();
         }
      }
      else if (theOverview.valid())
      {
         result = theOverview->getNumberOfSamples(resLevel);
      }
   }
   return result;
}

ossim_uint32 ossimMrSidReader::getNumberOfSamples(
   ossim_uint32 resLevel) const
{
   ossim_uint32 result = 0;
   if ( isValidRLevel(resLevel) )
   {
      if (resLevel <= theMinDwtLevels)
      {
         if (theMrSidDims.size() > 0)
         {
            result = theMrSidDims[resLevel].width();
         }
      }
      else if (theOverview.valid())
      {
         result = theOverview->getNumberOfSamples(resLevel);
      }
   }
   return result;
}

bool ossimMrSidReader::open()
{
   static const char MODULE[] = "ossimMrSidReader::open";

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

   ossimString ext = theImageFile.ext();
   ext.downcase();
   if ( ext != "sid" )
   {
      return false;
   }

   LT_STATUS sts = LT_STS_Uninit;

   const LTFileSpec fileSpec(theImageFile.c_str());
 
   theReader = MrSIDImageReader::create();
   sts = theReader->initialize(fileSpec, true);
   if (LT_SUCCESS(sts) == false)
   {
      return false;
   }

   theImageNavigator = new LTINavigator(*theReader);

   theImageRect = ossimIrect(0, 0, theReader->getWidth()-1, theReader->getHeight()-1);
   theNumberOfBands = theReader->getNumBands();
   theMinDwtLevels = theReader->getNumLevels();

   getDataType();

   if (getImageDimensions(theMrSidDims))
   {
      if (theScalarType != OSSIM_SCALAR_UNKNOWN)
      {
         ossim_uint32 width  = theReader->getWidth();
         ossim_uint32 height = theReader->getHeight();

         // Check for zero width, height.
         if ( !width || !height )
         {
            ossimIpt tileSize;
            ossim::defaultTileSize(tileSize);

            width  = tileSize.x;
            height = tileSize.y;
         }

         theTile = ossimImageDataFactory::instance()->create(this, this);

         theTile->initialize();

         // Call the base complete open to pick up overviews.
         computeMinMax();
         completeOpen();

         result = true;
      }
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

bool ossimMrSidReader::isOpen()const
{
   return theTile.get();
}

void ossimMrSidReader::closeEntry()
{
   theTile = 0;
   
   ossimImageHandler::close();
}

ossimRefPtr<ossimImageData> ossimMrSidReader::getTile(
   const ossimIrect& rect, ossim_uint32 resLevel)
{
   LT_STATUS sts = LT_STS_Uninit;

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
   if( resLevel > theMinDwtLevels )
   {
      if(theOverview.valid())
      {
         ossimRefPtr<ossimImageData> tileData = theOverview->getTile(rect, resLevel);
         tileData->setScalarType(getOutputScalarType());
         return tileData;
      }
   }

   theTile->setImageRectangle(rect);

   // Compute clip rectangle with respect to the image bounds.
   ossimIrect clipRect   = rect.clipToRect(imageBound);

   if (rect.completely_within(clipRect) == false)
   {
      // Not filling whole tile so blank it out first.
      theTile->makeBlank();
   }

   lt_uint16 anOssimBandIndex = 0;

   LTIPixel pixel(theReader->getColorSpace(), theNumberOfBands, theReader->getDataType());
   LTISceneBuffer sceneBuffer(pixel, clipRect.width(), clipRect.height(), NULL);

   if (!theGeometry.valid())
   {
      theGeometry = getImageGeometry();
   }
   double mag = theGeometry->decimationFactor(resLevel).lat;
   sts = theImageNavigator->setSceneAsULWH(clipRect.ul().x,
                                           clipRect.ul().y,
                                           clipRect.lr().x - clipRect.ul().x + 1,
                                           clipRect.lr().y - clipRect.ul().y + 1, mag);

   LTIScene scene = theImageNavigator->getScene();
   sts = theReader->read(scene, sceneBuffer);

   if (LT_SUCCESS(sts) == true)
   {
      if(clipRect == rect)
      {
         void *buf = theTile->getBuf();
         sceneBuffer.exportDataBSQ(buf);
      }
      else
      {
         for(anOssimBandIndex = 0; anOssimBandIndex < theNumberOfBands; anOssimBandIndex++)
         {
            theTile->loadBand(sceneBuffer.getBandData(anOssimBandIndex),//sceneBuffer.getTotalBandData(anOssimBandIndex),
                              clipRect, anOssimBandIndex);
         }
      }
   }

   theTile->validate();
   return theTile;
}

ossim_uint32 ossimMrSidReader::getNumberOfInputBands() const
{
   return theNumberOfBands;
}

ossim_uint32 ossimMrSidReader::getNumberOfOutputBands()const
{
   return theNumberOfBands;
}

ossim_uint32 ossimMrSidReader::getImageTileWidth() const
{
   ossim_uint32 result = 0;
   if ( theMrSidDims.size() )
   {
      if ( theMrSidDims[0].width() != theImageRect.width() )
      {
         // Not a single tile.
         result = theMrSidDims[0].width();
      }
   }
   return result;
}

ossim_uint32 ossimMrSidReader::getImageTileHeight() const
{
   ossim_uint32 result = 0;
   if ( theMrSidDims.size() )
   {
      if ( theMrSidDims[0].height() != theImageRect.height() )
      {
         // Not a single tile.
         result = theMrSidDims[0].height();
      }
   }
   return result;
}

ossimScalarType ossimMrSidReader::getOutputScalarType() const
{
   return theScalarType;
}

ossimRefPtr<ossimImageGeometry> ossimMrSidReader::getImageGeometry()
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

ossimRefPtr<ossimImageGeometry> ossimMrSidReader::getExternalImageGeometry() const
{
   // Check for external .geom file:
   ossimRefPtr<ossimImageGeometry> geom = ossimImageHandler::getExternalImageGeometry();
   if ( geom.valid() == false )
   {
      // Check for .aux.xml file:
      ossimFilename auxXmlFile = theImageFile;
      auxXmlFile += ".aux.xml";
      if  ( auxXmlFile.exists() )
      {
         ossimAuxXmlSupportData sd;
         ossimRefPtr<ossimProjection> proj = sd.getProjection( auxXmlFile );
         if ( proj.valid() )
         {
            geom = new ossimImageGeometry();
            geom->setProjection( proj.get() );
         }
      }
   }
   return geom;
}

ossimRefPtr<ossimImageGeometry> ossimMrSidReader::getInternalImageGeometry() const
{
   static const char MODULE[] = "ossimMrSidReader::getInternalImageGeometry";
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << MODULE << " entered...\n";
   }

   ossimRefPtr<ossimImageGeometry> geom = new ossimImageGeometry();

   // Must cast away constness.
   ossimMrSidReader* th = const_cast<ossimMrSidReader*>(this);
   ossimRefPtr<ossimProjection> proj = th->getGeoProjection();

   geom->setProjection(proj.get());

   return geom;
}

bool ossimMrSidReader::computeDecimationFactors(
   std::vector<ossimDpt>& decimations) const
{
   bool result = true;

   decimations.clear();

   const ossim_uint32 LEVELS = getNumberOfDecimationLevels();

   for (ossim_uint32 level = 0; level < LEVELS; ++level)
   {
      ossimDpt pt;

      if (level == 0)
      {
         // Assuming r0 is full res for now.
         pt.x = 1.0;
         pt.y = 1.0;
      }
      else
      {
         // Get the sample decimation.
         ossim_float64 r0 = getNumberOfSamples(0);
         ossim_float64 rL = getNumberOfSamples(level);
         if ( (r0 > 0.0) && (rL > 0.0) )
         {
            pt.x = rL / r0;
         }
         else
         {
            result = false;
            break;
         }

         // Get the line decimation.
         r0 = getNumberOfLines(0);
         rL = getNumberOfLines(level);
         if ( (r0 > 0.0) && (rL > 0.0) )
         {
            pt.y = rL / r0;
         }
         else
         {
            result = false;
            break;
         }
      }

      decimations.push_back(pt);
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimMrSidReader::computeDecimationFactors DEBUG\n";
      for (ossim_uint32 i = 0; i < decimations.size(); ++i)
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "decimation[" << i << "]: " << decimations[i]
            << std::endl;
      }
   }

   return result;
}

bool ossimMrSidReader::getImageDimensions(std::vector<ossimIrect>& tileDims) const
{
   bool result = true;

   tileDims.clear();

   if (theReader != 0)
   {
      ossim_uint32 levels = theReader->getNumLevels();

      for (ossim_uint32 level=0; level <= levels; ++level)
      {
         lt_uint32 width = 0;
         lt_uint32 height = 0;

         double mag = LTIUtils::levelToMag(level);
         LTIUtils::getDimsAtMag(theImageRect.width(), theImageRect.height(), mag, width, height);

         // Make the imageRect upper left relative to any sub image offset.
         ossimIrect imageRect(0, 0, width-1, height-1);

         tileDims.push_back(imageRect);
      }
   }
   else
   {
      result = false;
   }

   return result;
}

bool ossimMrSidReader::loadState(const ossimKeywordlist& kwl,
                                 const char* prefix)
{
   bool result = false;

   if ( ossimImageHandler::loadState(kwl, prefix) )
   {
      result = open();
   }

   return result;
}

void ossimMrSidReader::getDataType() 
{
   if (theReader != 0)
   {
      LTIDataType pixelType = theReader->getPixelProps().getDataType();   

      LTIPixel pixel = theReader->getPixelProps();

      switch (pixelType)
      {
         case LTI_DATATYPE_UINT8:
         {
            theScalarType = OSSIM_UINT8;
            break;
         }
         case LTI_DATATYPE_SINT8:
         {
            theScalarType = OSSIM_SINT8;
            break;
         }
         case LTI_DATATYPE_UINT16:
         {
            theScalarType = OSSIM_UINT16;
            break;
         }
         case LTI_DATATYPE_SINT16:
         {
            theScalarType = OSSIM_SINT16;
            break;
         }
         case LTI_DATATYPE_UINT32:
         {
            theScalarType = OSSIM_UINT32;
            break;
         }
         case LTI_DATATYPE_SINT32:
         {
            theScalarType = OSSIM_SINT32;
            break;
         }
         case LTI_DATATYPE_FLOAT32:
         {
            theScalarType = OSSIM_FLOAT32;
            break;
         }
         case LTI_DATATYPE_FLOAT64:
         {
            theScalarType = OSSIM_FLOAT64;
            break;
         }
         default:
         {
            theScalarType = OSSIM_SCALAR_UNKNOWN;
            break;
         }
      }
   }
}

ossimProjection* ossimMrSidReader::getGeoProjection()
{
   if (theReader == 0)
      return 0;

   // A local KWL will be filled as items are read from the support data. At the end,
   // the projection factory will be provided with the populated KWL to instantiate proper 
   // map projection. No prefix needed.
   ossimKeywordlist kwl; 

   // Add the lines and samples.
   kwl.add(ossimKeywordNames::NUMBER_LINES_KW, getNumberOfLines(0));
   kwl.add(ossimKeywordNames::NUMBER_SAMPLES_KW, getNumberOfSamples(0));

   ossimString proj_type;
   ossimString datum_type;
   ossimString scale_units  = "unknown";
   ossimString tie_pt_units = "unknown";

   ossim_uint32 code = 0;
   ossim_uint32 gcsCode = 0;
   ossim_uint32 pcsCode = 0;

   const LTIGeoCoord& geo = theReader->getGeoCoord();
   LTIMetadataDatabase metaDb = theReader->getMetadata();

#if 0 /* Please keep for debug. (drb) */
   for (ossim_uint32 i = 0; i < metaDb.getIndexCount(); ++i )
   {
      const LTIMetadataRecord* rec = 0;
      metaDb.getDataByIndex(i, rec);
      cout << "rec.getTagName(): " << rec->getTagName() << endl;
   }
   const char* projStr = geo.getWKT();
   if ( projStr )
   {
      std::string wktStr = projStr;
      
      cout << "wktStr: " << wktStr << endl;
      ossimWkt wkt;
      if ( wkt.open( wktStr ) )
      {
         cout << "kwl:\n" << wkt.getKwl() << endl;
      }
   }
#endif
   
   // Can only handle non-rotated images since only projection object returned (no 2d transform):
   if( (geo.getXRot() != 0.0) || (geo.getYRot() != 0.0))
      return 0;

   bool gcsFound = getMetadataElement(
      metaDb, "GEOTIFF_NUM::2048::GeographicTypeGeoKey", &gcsCode);
   bool pcsFound = getMetadataElement(
      metaDb, "GEOTIFF_NUM::3072::ProjectedCSTypeGeoKey", &pcsCode);

   if (gcsFound && !pcsFound)
   {
      code = gcsCode;
      kwl.add(ossimKeywordNames::TYPE_KW, "ossimEquDistCylProjection");
      proj_type = "ossimEquDistCylProjection";
      kwl.add(ossimKeywordNames::PCS_CODE_KW, code);
      tie_pt_units = "degrees";

      // Assign units if set in Metadata
      char unitStr[200];
      if (getMetadataElement(metaDb, "GEOTIFF_CHAR::GeogAngularUnitsGeoKey",
                             &unitStr, sizeof(unitStr)) == true)
      {
         ossimString unitTag(unitStr);
         if ( unitTag.contains("Angular_Degree") ) // decimal degrees
            scale_units = "degrees";
         else if ( unitTag.contains("Angular_Minute") ) // decimal minutes
            scale_units = "minutes";
         else if ( unitTag.contains("Angular_Second") ) // decimal seconds
            scale_units = "seconds";
      }
   }
   else 
   {
      if (!pcsFound)
      {
         pcsFound = getMetadataElement(metaDb, "GEOTIFF_NUM::3074::ProjectionGeoKey", &code);
      }
      else
      {
        code = pcsCode;
      }

      if (pcsFound)
      {
         kwl.add(ossimKeywordNames::PCS_CODE_KW, code);

         ossimString unitTag;

         char unitStr[200];
         if (getMetadataElement(metaDb, "GEOTIFF_CHAR::ProjLinearUnitsGeoKey", &unitStr, 
                                sizeof(unitStr)) == true )
         {
            unitTag = unitStr;
         }
         else
         {
            // Have data with no ProjLinearUnitsGeoKey so try WKT string.
            const char* wktProjStr = geo.getWKT();
            if ( wktProjStr )
            {
               std::string wktStr = wktProjStr;
               ossimWkt wkt;
               if ( wkt.parse( wktStr ) )
               {
                  unitTag = wkt.getKwl().findKey( std::string("PROJCS.UNIT.name") );
               }
            }
         }

         if ( unitTag.size() )
         {
            unitTag.downcase();
            
            if ( unitTag.contains("meter") ||  unitTag.contains("metre") )
            {
               scale_units = "meters";
            }
            else if ( unitTag.contains("linear_foot_us_survey") )
            {
               scale_units = "us_survey_feet";
            }
            else if ( unitTag.contains("linear_foot") )
            {
               scale_units = "feet";
            }
            tie_pt_units = scale_units;
         }
         
      }
      else
      {
         // Try with WKT:
         const char* projStr = geo.getWKT();
         kwl.add(ossimKeywordNames::TYPE_KW, projStr);
      }
   }

   char rasterTypeStr[200];
   strcpy( rasterTypeStr, "unnamed" );
   double topLeftX = geo.getX(); // AMBIGUOUS! OLK 5/10
   double topLeftY = geo.getY(); // AMBIGUOUS! OLK 5/10
   if (getMetadataElement(metaDb, "GEOTIFF_CHAR::GTRasterTypeGeoKey", &rasterTypeStr, sizeof(rasterTypeStr)) == true)
   {
      ossimString rasterTypeTag(rasterTypeStr);

      // If the raster type is pixel_is_area, shift the tie point by
      // half a pixel to locate it at the pixel center. 
      if ( rasterTypeTag.contains("RasterPixelIsPoint") )
      {
         topLeftX -= geo.getXRes() / 2.0;  // AMBIGUOUS -- DOESN'T MATCH COMMENT! OLK 5/10
         topLeftY += geo.getYRes() / 2.0;  // AMBIGUOUS! OLK 5/10
      }
   }
   ossimDpt gsd(fabs(geo.getXRes()), fabs(geo.getYRes()));
   ossimDpt tie(topLeftX, topLeftY);

   // CANNOT HANDLE 2D TRANSFORMS -- ONLY REAL PROJECTIONS. (OLK 5/10)
   //std::stringstream mString;
   //// store as a 4x4 matrix
   //mString << ossimString::toString(geo.getXRes(), 20)
   //  << " " << ossimString::toString(geo.getXRot(), 20)
   //  << " " << 0 << " "
   //  << ossimString::toString(geo.getX(), 20)
   //  << " " << ossimString::toString(geo.getYRot(), 20)
   //  << " " << ossimString::toString(geo.getYRes(), 20)
   //  << " " << 0 << " "
   //  << ossimString::toString(geo.getY(), 20)
   //  << " " << 0 << " " << 0 << " " << 1 << " " << 0
   //  << " " << 0 << " " << 0 << " " << 0 << " " << 1;
   //kwl.add(ossimKeywordNames::IMAGE_MODEL_TRANSFORM_MATRIX_KW, mString.str().c_str());

   // if meta data does not have the code info, try to read from .aux file
   if (code == 0)
   {
      ossimFilename auxFile = theImageFile;
      auxFile.setExtension("aux");
      ossimAuxFileHandler auxHandler;
      if (auxFile.exists() && auxHandler.open(auxFile))
      {
         ossimString proj_name = auxHandler.getProjectionName();
         ossimString datum_name = auxHandler.getDatumName();
         ossimString unitType = auxHandler.getUnitType();

         // HACK: Geographic projection is specified in non-WKT format. Intercepting here. OLK 5/10
         // TODO: Need projection factory that can handle miscellaneous non-WKT specs as they are 
         //       encountered. 
         if (proj_name.contains("Geographic"))
         {
            kwl.add(ossimKeywordNames::TYPE_KW, "ossimEquDistCylProjection", false);
            scale_units = "degrees";
            tie_pt_units = "degrees";
         }
         else
         {
            // pass along MrSid's projection name and pray it can be resolved:
            kwl.add(ossimKeywordNames::PROJECTION_KW, proj_name, false);
            if (unitType.empty())
            {
               if (proj_name.downcase().contains("feet"))
               {
                  scale_units = "feet";
                  tie_pt_units = "feet";
               }
            }
            else
            {
               scale_units = unitType;
               tie_pt_units = unitType;
            }
         }

         // HACK: WGS-84 is specified in non-WKT format. Intercepting here. OLK 5/10
         // TODO: Need datum factory that can handle miscellaneous non-WKT specs as they are 
         //       encountered. 
         if (datum_name.contains("WGS") && datum_name.contains("84"))
            kwl.add(ossimKeywordNames::DATUM_KW, "EPSG:6326");
         else
            kwl.add(ossimKeywordNames::DATUM_KW, datum_name, false);
      }
   }

   if ( scale_units.empty()  || (scale_units == "unknown" ) ||
        tie_pt_units.empty() || (tie_pt_units == "unknown" ) )
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << "ossimMrSidReader::getProjection WARNING: Undefined units!"
         << "\nscale units:  " << scale_units
         << "\ntie_pt_units: " << tie_pt_units
         << std::endl;
   }

   kwl.add(ossimKeywordNames::PIXEL_SCALE_XY_KW, gsd.toString());
   kwl.add(ossimKeywordNames::PIXEL_SCALE_UNITS_KW, scale_units);
   kwl.add(ossimKeywordNames::TIE_POINT_XY_KW, tie.toString());
   kwl.add(ossimKeywordNames::TIE_POINT_UNITS_KW, tie_pt_units);

   ossimProjection* proj = 
      ossimProjectionFactoryRegistry::instance()->createProjection(kwl);
   return proj;
}

bool ossimMrSidReader::getMetadataElement(LTIMetadataDatabase metaDb,
                                          const char *tagName, 
                                          void *pValue,
                                          int iLength)
{
   if (!metaDb.has(tagName))
   {
      return false;
   }

   const LTIMetadataRecord* metaRec = 0;
   metaDb.get(tagName, metaRec );

   if (!metaRec->isScalar())
   {
      return false;
   }

   // XXX: return FALSE if we have more than one element in metadata record
   int iSize;
   switch(metaRec->getDataType())
   {
      case LTI_METADATA_DATATYPE_UINT8:
      case LTI_METADATA_DATATYPE_SINT8:
         iSize = 1;
         break;
      case LTI_METADATA_DATATYPE_UINT16:
      case LTI_METADATA_DATATYPE_SINT16:
         iSize = 2;
         break;
      case LTI_METADATA_DATATYPE_UINT32:
      case LTI_METADATA_DATATYPE_SINT32:
      case LTI_METADATA_DATATYPE_FLOAT32:
         iSize = 4;
         break;
      case LTI_METADATA_DATATYPE_FLOAT64:
         iSize = 8;
         break;
      case LTI_METADATA_DATATYPE_ASCII:
         iSize = iLength;
         break;
      default:
         iSize = 0;
         break;
   }

   if ( metaRec->getDataType() == LTI_METADATA_DATATYPE_ASCII )
   {
      strncpy( (char *)pValue, ((const char**)metaRec->getScalarData())[0], iSize );
      ((char *)pValue)[iSize - 1] = '\0';
   }
   else
   {
      memcpy( pValue, metaRec->getScalarData(), iSize);
   }

   return true;
}

void ossimMrSidReader::computeMinMax()
{
}
