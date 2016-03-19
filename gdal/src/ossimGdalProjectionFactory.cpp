//*****************************************************************************
//
// License:  LGPL
//
// See LICENSE.txt file in the top level directory for more details.
//
// DESCRIPTION:
//   Contains implementation of class ossimMapProjectionFactory
//
//*****************************************************************************
//  $Id: ossimGdalProjectionFactory.cpp 22734 2014-04-15 19:28:42Z gpotts $
#include <sstream>
#include "ossimGdalProjectionFactory.h"
#include "ossimGdalTileSource.h"
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <ossim/projection/ossimUtmProjection.h>
#include <ossim/projection/ossimEquDistCylProjection.h>
#include <ossim/projection/ossimEquDistCylProjection.h>
#include <ossim/projection/ossimTransMercatorProjection.h>
#include <ossim/projection/ossimOrthoGraphicProjection.h>
#include "ossimOgcWktTranslator.h"
#include <ossim/base/ossimTrace.h>
#include "ossimOgcWktTranslator.h"
#include <ossim/projection/ossimPolynomProjection.h>
#include <ossim/projection/ossimBilinearProjection.h>
#include <gdal_priv.h>
#include <cpl_string.h>
#include <ossim/base/ossimTieGptSet.h>
#include <ossim/base/ossimUnitConversionTool.h>
#include <ossim/base/ossimUnitTypeLut.h>

static ossimTrace traceDebug("ossimGdalProjectionFactory:debug");

ossimGdalProjectionFactory* ossimGdalProjectionFactory::theInstance = 0;
static ossimOgcWktTranslator wktTranslator;

ossimGdalProjectionFactory* ossimGdalProjectionFactory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimGdalProjectionFactory;
   }

   return (ossimGdalProjectionFactory*) theInstance;
}

ossimProjection* ossimGdalProjectionFactory::createProjection(const ossimFilename& filename,
                                                             ossim_uint32 entryIdx)const
{
   ossimKeywordlist kwl;
//    ossimRefPtr<ossimImageHandler> h = new ossimGdalTileSource;
   //std::cout << "ossimGdalProjectionFactory::createProjection: " << filename << std::endl;
   GDALDatasetH  h; 
   GDALDriverH   driverH = 0;
   ossimProjection* proj = 0;

   if(ossimString(filename).trim().empty()) return 0;

   h = GDALOpen(filename.c_str(), GA_ReadOnly);
   if(h)
   {
      driverH = GDALGetDatasetDriver( h );
      ossimString driverName( driverH ? GDALGetDriverShortName( driverH ) : "" );
      // use OSSIM's projection loader for NITF
      //
      if(driverName == "NITF")
      {
         GDALClose(h);
         return 0;
      }
      if(entryIdx != 0)
      {
         char** papszMetadata = GDALGetMetadata( h, "SUBDATASETS" );

         //---
         // ??? (drb) Should this be:
         // if ( entryIdx >= CSLCount(papszMetadata) ) close...
         //---
         if( papszMetadata&&(CSLCount(papszMetadata) < static_cast<ossim_int32>(entryIdx)) )
         {
            ossimNotify(ossimNotifyLevel_WARN) << "ossimGdalProjectionFactory::createProjection: We don't support multi entry handlers through the factory yet, only through the handler!";
            GDALClose(h);
            return 0;
         }
         else
         {
            GDALClose(h);
            return 0;
         }
      }

      ossimString wkt(GDALGetProjectionRef( h ));
      double geoTransform[6];
      bool transOk = GDALGetGeoTransform( h, geoTransform ) == CE_None;
      bool wktTranslatorOk = wkt.empty()?false:wktTranslator.toOssimKwl(wkt, kwl);
      if(!wktTranslatorOk)
      {
         ossim_uint32 gcpCount = GDALGetGCPCount(h);
         if(gcpCount > 3)
         {
            ossim_uint32 idx = 0;
            const GDAL_GCP* gcpList = GDALGetGCPs(h);
            ossimTieGptSet tieSet;
            if(gcpList)
            {
               for(idx = 0; idx < gcpCount; ++idx)
               {
                  ossimDpt dpt(gcpList[idx].dfGCPPixel,
                               gcpList[idx].dfGCPLine);
                  ossimGpt gpt(gcpList[idx].dfGCPY,
                               gcpList[idx].dfGCPX,
                               gcpList[idx].dfGCPZ);
                  tieSet.addTiePoint(new ossimTieGpt(gpt, dpt, .5));
               }

               //ossimPolynomProjection* tempProj = new ossimPolynomProjection;
               ossimBilinearProjection* tempProj = new ossimBilinearProjection;
			   //tempProj->setupOptimizer("1 x y x2 xy y2 x3 y3 xy2 x2y z xz yz");
               tempProj->optimizeFit(tieSet);
               proj = tempProj;
            }
         }
      }
      if ( transOk && proj==0 )
      {
         ossimString proj_type(kwl.find(ossimKeywordNames::TYPE_KW));
         ossimString datum_type(kwl.find(ossimKeywordNames::DATUM_KW));
         ossimString units(kwl.find(ossimKeywordNames::UNITS_KW));
         if ( proj_type.trim().empty() &&
              (driverName == "MrSID" || driverName == "JP2MrSID") )
         {
            bool bClose = true;
            // ESH 04/2008, #54: if no rotation factors use geographic system
            if( geoTransform[2] == 0.0 && geoTransform[4] == 0.0 )
            {
               ossimString projTag( GDALGetMetadataItem( h, "IMG__PROJECTION_NAME", "" ) );
               if ( projTag.contains("Geographic") )
               {
                  bClose = false;

                  kwl.add(ossimKeywordNames::TYPE_KW,
                          "ossimEquDistCylProjection", true);
                  proj_type = kwl.find( ossimKeywordNames::TYPE_KW );

                  // Assign units if set in Metadata
                  ossimString unitTag( GDALGetMetadataItem( h, "IMG__HORIZONTAL_UNITS", "" ) );
                  if ( unitTag.contains("dd") ) // decimal degrees
                  {
                     units = "degrees";
                  }
                  else if ( unitTag.contains("dm") ) // decimal minutes
                  {
                     units = "minutes";
                  }
                  else if ( unitTag.contains("ds") ) // decimal seconds
                  {
                     units = "seconds";
                  }
               }
            }

            if ( bClose == true )
            {
               GDALClose(h);
               return 0;
            }
         }

         // Pixel-is-point of pixel-is area affects the location of the tiepoint since OSSIM is
         // always pixel-is-point so 1/2 pixel shift may be necessary:
         if((driverName == "MrSID") || (driverName == "JP2MrSID") || (driverName == "AIG"))
         {
            const char* rasterTypeStr = GDALGetMetadataItem( h, "GEOTIFF_CHAR__GTRasterTypeGeoKey", "" );
            ossimString rasterTypeTag( rasterTypeStr );

            // If the raster type is pixel_is_area, shift the tie point by
            // half a pixel to locate it at the pixel center.
            if ((driverName == "AIG") || (rasterTypeTag.contains("RasterPixelIsArea")))
            {
               geoTransform[0] += fabs(geoTransform[1]) / 2.0;
               geoTransform[3] -= fabs(geoTransform[5]) / 2.0;
            }
         }
         else
         {
            // Conventionally, HFA stores the pixel alignment type for each band. Here assume all
            // bands are the same. Consider only the first band:
            GDALRasterBandH bBand = GDALGetRasterBand( h, 1 );
            char** papszMetadata = GDALGetMetadata( bBand, NULL );
            if (CSLCount(papszMetadata) > 0)
            {
               for(int i = 0; papszMetadata[i] != NULL; i++ )
               {
                  ossimString metaStr = papszMetadata[i];
                  metaStr.upcase();
                  if (metaStr.contains("AREA_OR_POINT"))
                  {
                     ossimString pixel_is_point_or_area = metaStr.split("=")[1];
                     pixel_is_point_or_area.upcase();
                     if (pixel_is_point_or_area.contains("AREA"))
                     {
                        // Need to shift the tie point so that pixel is point:
                        geoTransform[0] += fabs(geoTransform[1]) / 2.0;
                        geoTransform[3] -= fabs(geoTransform[5]) / 2.0;
                     }
                     break;
                  }
               }
            }
         }

         kwl.remove(ossimKeywordNames::UNITS_KW);
         ossimDpt gsd(fabs(geoTransform[1]), fabs(geoTransform[5]));
         ossimDpt tie(geoTransform[0], geoTransform[3]);

         ossimUnitType savedUnitType =
            static_cast<ossimUnitType>(ossimUnitTypeLut::instance()->getEntryNumber(units));
         ossimUnitType unitType = savedUnitType;
         if(unitType == OSSIM_UNIT_UNKNOWN)
            unitType = OSSIM_METERS;

         if((proj_type == "ossimLlxyProjection") || (proj_type == "ossimEquDistCylProjection"))
         {
            // ESH 09/2008 -- Add the orig_lat and central_lon if the image
            // is using geographic coordsys.  This is used to convert the
            // gsd to linear units.

            // Half the number of pixels in lon/lat directions
            int nPixelsLon = GDALGetRasterXSize(h)/2.0;
            int nPixelsLat = GDALGetRasterYSize(h)/2.0;

            // Shift from image corner to center in lon/lat
            double shiftLon =  nPixelsLon * fabs(gsd.x);
            double shiftLat = -nPixelsLat * fabs(gsd.y);

            // lon/lat of center pixel of the image
            double centerLon = tie.x + shiftLon;
            double centerLat = tie.y + shiftLat;

            kwl.add(ossimKeywordNames::ORIGIN_LATITUDE_KW,
                    centerLat,
                    true);
            kwl.add(ossimKeywordNames::CENTRAL_MERIDIAN_KW,
                    centerLon,
                    true);

            kwl.add(ossimKeywordNames::TIE_POINT_LAT_KW,
                    tie.y,
                    true);
            kwl.add(ossimKeywordNames::TIE_POINT_LON_KW,
                    tie.x,
                    true);

            kwl.add(ossimKeywordNames::DECIMAL_DEGREES_PER_PIXEL_LAT,
                    gsd.y,
                    true);
            kwl.add(ossimKeywordNames::DECIMAL_DEGREES_PER_PIXEL_LON,
                    gsd.x,
                    true);

            if(savedUnitType == OSSIM_UNIT_UNKNOWN)
            {
               unitType = OSSIM_DEGREES;
            }
         }

         kwl.add(ossimKeywordNames::PIXEL_SCALE_XY_KW,
                 gsd.toString(),
                 true);
         kwl.add(ossimKeywordNames::PIXEL_SCALE_UNITS_KW,
                 units,
                 true);
         kwl.add(ossimKeywordNames::TIE_POINT_XY_KW,
                 tie.toString(),
                 true);
         kwl.add(ossimKeywordNames::TIE_POINT_UNITS_KW,
                 units,
                 true);

         std::stringstream matrixString;
         // store as a 4x4 matrix
         matrixString
            << ossimString::toString(geoTransform[1], 20)
            << " " << ossimString::toString(geoTransform[2], 20)
            << " " << 0 << " "
            << ossimString::toString(geoTransform[0], 20)
            << " " << ossimString::toString(geoTransform[4], 20)
            << " " << ossimString::toString(geoTransform[5], 20)
            << " " << 0 << " "
            << ossimString::toString(geoTransform[3], 20)
            << " " << 0 << " " << 0 << " " << 1 << " " << 0
            << " " << 0 << " " << 0 << " " << 0 << " " << 1;
         
         kwl.add(ossimKeywordNames::IMAGE_MODEL_TRANSFORM_MATRIX_KW,
                 matrixString.str().c_str(), true);
         kwl.add(ossimKeywordNames::IMAGE_MODEL_TRANSFORM_UNIT_KW,
                 units.string().c_str(), true);

         //---
         // SPECIAL CASE:  ArcGrid in British National Grid
         //---
         if(driverName == "AIG" && datum_type == "OSGB_1936")
         {
            ossimFilename prj_file = filename.path() + "/prj.adf";

            if(prj_file.exists())
            {
               ossimKeywordlist prj_kwl(' ');
               prj_kwl.addFile(prj_file);

               ossimString proj = prj_kwl.find("Projection");

               // Reset projection and Datum correctly for BNG.
               if(proj.upcase().contains("GREATBRITAIN"))
               {

                  kwl.add(ossimKeywordNames::TYPE_KW,
                          "ossimBngProjection", true);

                  ossimString datum  = prj_kwl.find("Datum");

                  if(datum != "")
                  {
                     if(datum == "OGB_A")
                        datum = "OGB-A";
                     else if(datum == "OGB_B")
                        datum = "OGB-B";
                     else if(datum == "OGB_C")
                        datum = "OGB-C";
                     else if(datum == "OGB_D")
                        datum = "OGB-D";
                     else if(datum == "OGB_M")
                        datum = "OGB-M";
                     else if(datum == "OGB_7")
                        datum = "OGB-7";

                     kwl.add(ossimKeywordNames::DATUM_KW,
                             datum, true);
                  }
               }
            }
         }
     }
	 if(traceDebug())
	 {
		 ossimNotify(ossimNotifyLevel_DEBUG) << "ossimGdalProjectionFactory: createProjection KWL = \n " << kwl << std::endl;
	 }
      GDALClose(h);
      proj = ossimProjectionFactoryRegistry::instance()->createProjection(kwl);
  }

   return proj;
}

ossimProjection* ossimGdalProjectionFactory::createProjection(const ossimKeywordlist &keywordList,
                                                             const char *prefix) const
{
   const char *lookup = keywordList.find(prefix, ossimKeywordNames::TYPE_KW);
   if(lookup&&(!ossimString(lookup).empty()))
   {
      ossimProjection* proj = createProjection(ossimString(lookup));
      if(proj)
      {
         // make sure we restore any passed in tie points and meters per pixel information
         ossimKeywordlist tempKwl;
         ossimKeywordlist tempKwl2;
         proj->saveState(tempKwl);
         tempKwl2.add(keywordList, prefix, true);
         tempKwl.add(prefix, tempKwl2,  true);
         tempKwl.add(prefix, ossimKeywordNames::TYPE_KW, proj->getClassName(), true);
	 proj->loadState(tempKwl);
         if(traceDebug())
         {
            tempKwl.clear();
            proj->saveState(tempKwl);
            if(traceDebug())
            {
               ossimNotify(ossimNotifyLevel_DEBUG)<< "ossimGdalProjectionFactory::createProjection Debug: resulting projection \n " << tempKwl << std::endl;
            }
         }

         return proj;
      }
   }
   return 0;
}

ossimProjection* ossimGdalProjectionFactory::createProjection(const ossimString &name) const
{
   ossimString tempName = name;
   if(traceDebug())
   {
       ossimNotify(ossimNotifyLevel_WARN) << "ossimGdalProjectionFactory::createProjection: "<< name << "\n";
   }
   tempName = tempName.trim();

   if ( tempName.size() >= 6 )
   {
      ossimString testName(tempName.begin(),
                           tempName.begin()+6);
      testName = testName.upcase();
      if((testName == "PROJCS")||
         (testName == "GEOGCS"))
      {
         ossimKeywordlist kwl;
         if ( theWktTranslator.toOssimKwl(name.c_str(), kwl, "") )
         {
            if(traceDebug())
            {
               ossimNotify(ossimNotifyLevel_DEBUG)<< "ossimGdalProjectionFactory::createProjection Debug: trying to create projection \n " << kwl << std::endl;
            }
            return ossimProjectionFactoryRegistry::instance()->createProjection(kwl,"");
        }
      }
   }

   return 0;
}

ossimObject* ossimGdalProjectionFactory::createObject(const ossimString& typeName)const
{
   return createProjection(typeName);
}

ossimObject* ossimGdalProjectionFactory::createObject(const ossimKeywordlist& kwl,
                                                     const char* prefix)const
{
   return createProjection(kwl, prefix);
}

void ossimGdalProjectionFactory::getTypeNameList(std::vector<ossimString>& /* typeList */)const
{
}

std::list<ossimString> ossimGdalProjectionFactory::getList()const
{
   std::list<ossimString> result;

   return result;
}
