//*******************************************************************
//
// License:  LGPL
//
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Garrett Potts
//
//*******************************************************************
//  $Id: ossimGdalWriter.cpp 21634 2012-09-06 18:15:26Z dburken $

#include "ossimGdalWriter.h"
#include "ossimOgcWktTranslator.h"
#include "ossimGdalTiledDataset.h"
#include <ossim/base/ossimBooleanProperty.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimListener.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimNumericProperty.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimStringProperty.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimXmlAttribute.h>
#include <ossim/base/ossimXmlNode.h>
#include <ossim/imaging/ossimImageChain.h>
#include <ossim/imaging/ossimImageGeometry.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/projection/ossimMapProjection.h>
#include <ossim/projection/ossimMapProjectionInfo.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <ossim/vpfutil/set.h>
#include <cmath>
#include <iterator>
#include <sstream>
using namespace std;

static int CPL_STDCALL gdalProgressFunc(double percentComplete,
                            const char* msg,
                            void* data)
{
   ossimGdalWriter* writer = (ossimGdalWriter*)data;
   
   ossimProcessProgressEvent event(writer,
                                   percentComplete*100.0,
                                   msg,
                                   false);
   
   writer->fireEvent(event);
   
   return !writer->needsAborting();
}

static ossimTrace traceDebug(ossimString("ossimGdalWriter:debug"));

RTTI_DEF1(ossimGdalWriter, "ossimGdalWriter", ossimImageFileWriter);

static ossimOgcWktTranslator translator;

ossimGdalWriter::ossimGdalWriter()
   :ossimImageFileWriter(),
    theDriverName(""),
    theDriver((GDALDriverH)0),
    theDataset((GDALDatasetH)0),
    theJpeg2000TileSize(),
    theDriverOptionValues(),
    theGdalDriverOptions(0),
    theGdalOverviewType(ossimGdalOverviewType_NONE),
    theColorLutFlag(false),
    theColorLut(0),
    theLutFilename(),
    theNBandToIndexFilter(0)
{ 
}

ossimGdalWriter::~ossimGdalWriter()
{
   deleteGdalDriverOptions();
   close();
   theDataset = 0;
   theDriverName = "";
}

bool ossimGdalWriter::isOpen()const
{
   return (theDriver != 0);
}

bool ossimGdalWriter::open()
{
   theDriverName = convertToDriverName(theOutputImageType);
   theDriver = GDALGetDriverByName(theDriverName.c_str());

   if(theDriver)
   {
      return true;
   }

   return false;
}

void ossimGdalWriter::close()
{
   if(theDataset)
   {
      GDALClose(theDataset);
      theDataset = 0;
   }
}

void  ossimGdalWriter::setOutputImageType(const ossimString& type)
{
   ossimImageFileWriter::setOutputImageType(type);
   theDriverOptionValues.clear();
}

void ossimGdalWriter::setLut(const ossimNBandLutDataObject& lut)
{
  theColorLutFlag = true;
  theColorLut = (ossimNBandLutDataObject*)lut.dup();
}

/*!
 * saves the state of the object.
 */
bool ossimGdalWriter::saveState(ossimKeywordlist& kwl,
                                const char* prefix)const
{

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalWriter::saveState entered ..."
         << "\nprefix:         " << prefix << std::endl;
   }

   ossimString rrdOption = CPLGetConfigOption("HFA_USE_RRD","");
   if(rrdOption != "")
   {
      kwl.add(prefix,
              "HFA_USE_RRD",
              rrdOption,
              true);
   }
   kwl.add(prefix,
           "gdal_overview_type",
           gdalOverviewTypeToString(),
           true);
   kwl.add(prefix, theDriverOptionValues);

   kwl.add(prefix,
     "color_lut_flag",
     (ossim_uint32)theColorLutFlag,
     true);

   if(theColorLutFlag)
   {
     if(theLutFilename != "")
     {
       kwl.add(prefix,
         "lut_filename",
         theLutFilename.c_str(),
         true);
     }
     else
     {
        if ( theColorLut.valid() )
        {
           ossimString newPrefix = ossimString(prefix) + "lut.";
           theColorLut->saveState(kwl, newPrefix.c_str());
        }
     }
   }
   
   return ossimImageFileWriter::saveState(kwl, prefix);
}

/*!
 * Method to the load (recreate) the state of an object from a keyword
 * list.  Return true if ok or false on error.
 */
bool ossimGdalWriter::loadState(const ossimKeywordlist& kwl,
                                const char* prefix)
{
   ossimImageFileWriter::loadState(kwl, prefix);

   ossimString regExpression =  (ossimString("^") +
                                 ossimString(prefix) +
                                 "property[0-9]+");
   const char* hfa_use_rrd = kwl.find(prefix, "HFA_USE_RRD");

   if(hfa_use_rrd)
   {
      CPLSetConfigOption("HFA_USE_RRD", hfa_use_rrd);
   }
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalWriter::loadState entered ..."
         << "\nprefix:         " << (prefix?prefix:"null")
         << "\nregExpression:  " << regExpression
         << "\nkwl:" << kwl << std::endl;
   }
   const char* overviewType = kwl.find(prefix, "gdal_overview_type");

   if(overviewType)
   {
      theGdalOverviewType = gdalOverviewTypeFromString(ossimString(overviewType));
   }

   ossimString newPrefix = ossimString(prefix) + "lut.";

   const char* colorLutFlag = kwl.find(prefix, "color_lut_flag");
   if(colorLutFlag)
   {
     theColorLutFlag = ossimString(colorLutFlag).toBool();
   }
   else
   {
     theColorLutFlag = false;
   }

   theLutFilename = ossimFilename(kwl.find(prefix, "lut_filename"));
   theLutFilename = ossimFilename(theLutFilename.trim());
   if ( theColorLut.valid() == false ) theColorLut = new ossimNBandLutDataObject();
   if(theLutFilename != "")
   {
     theColorLut->open(theLutFilename);
   }
   else
   {
     theColorLut->loadState(kwl, newPrefix.c_str());
   }
   
   vector<ossimString> keys = kwl.getSubstringKeyList( regExpression );
   theDriverOptionValues.clear();
   
   deleteGdalDriverOptions();

   kwl.extractKeysThatMatch(theDriverOptionValues,
                            regExpression);

   if(prefix)
   {
      theDriverOptionValues.stripPrefixFromAll(prefix);
   }
   
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalWriter::loadState exiting..." << std::endl;
   }
   return true;
}

void ossimGdalWriter::getSupportedWriters(vector<ossimString>& /* writers */)
{
}

bool ossimGdalWriter::isLutEnabled()const
{
  return (theColorLutFlag);
}

void ossimGdalWriter::checkColorLut()
{
   bool needColorLut = false;
   bool needLoop = true;
   ossimRefPtr<ossimNBandLutDataObject> colorLut = 0;
   ossimConnectableObject::ConnectableObjectList imageInputs = theInputConnection->getInputList();
   if (imageInputs.size() > 0)
   {
      for (ossim_uint32 i = 0; i < imageInputs.size(); i++)
      {
         if (needLoop == false)
         {
            break;
         }
         ossimImageChain* source = PTR_CAST(ossimImageChain, imageInputs[i].get());
         if (source)
         {
            ossimConnectableObject::ConnectableObjectList imageChains = source->getInputList();
            for (ossim_uint32 j = 0; j < imageChains.size(); j++)
            {
               if (needLoop == false)
               {
                  break;
               }
               ossimImageChain* imageChain = PTR_CAST(ossimImageChain, imageChains[j].get());
               if (imageChain)
               {
                  ossimConnectableObject::ConnectableObjectList imageHandlers =
                     imageChain->findAllObjectsOfType(STATIC_TYPE_INFO(ossimImageHandler), false);

                  for (ossim_uint32 h= 0; h < imageHandlers.size(); h++)
                  {
                     ossimImageHandler* handler =
                        PTR_CAST(ossimImageHandler, imageHandlers[h].get());
                     if (handler)
                     {
                        if (handler->hasLut() != 0) //
                        {
                           colorLut = handler->getLut();
                           needColorLut = true;
                        }
                        else //if not all handlers have color luts, ignore the color lut.
                        {
                           needColorLut = false;
                           needLoop = false;
                           break;
                        }
                     }
                  }
               }
            }
         }
      }
   }

   if (needColorLut && colorLut != 0)
   {
      setLut(*colorLut.get());
   }
}

bool ossimGdalWriter::writeFile()
{
   bool result = true; // On failure set to false.
   
   const char *MODULE = "ossimGdalWriter::writeFile()";

   if(traceDebug())
   {
      CLOG << "entered..." << std::endl;
   }
      
   if(theDataset)
   {
      close();
   }

   checkColorLut();

   //---
   // If isInputDataIndexed is true the data fed to writer is already in palette index form so
   // the addition of the ossimNBandToIndexFilter is not needed.
   //---
   if( isLutEnabled() && !isInputDataIndexed() )
   {
      theNBandToIndexFilter = new ossimNBandToIndexFilter;
      theNBandToIndexFilter->connectMyInputTo(0, theInputConnection->getInput());
      theNBandToIndexFilter->setLut(*theColorLut.get());
      theNBandToIndexFilter->initialize();
      theInputConnection->disconnect();
      theInputConnection->connectMyInputTo(0, theNBandToIndexFilter.get());
      theInputConnection->initialize();
   }
   else
   {
      theNBandToIndexFilter = 0;
   }

   //---
   // setup gdal driver options
   //---
   ossimRefPtr<ossimProperty> blockSize =
      getProperty(theDriverName+ossimString("_BLOCKSIZE"));
   if(blockSize.valid())
   {
      ossim_uint32 size = blockSize->valueToString().toUInt32();
      ossimIpt tileSize;
      if(size > 0)
      {
         tileSize = ossimIpt(size, size);
         theInputConnection->setTileSize(tileSize);
      }
      else
      {
         ossimIpt defaultSize = theInputConnection->getTileSize();
         if(defaultSize.y != defaultSize.x)
         {
            defaultSize.y = defaultSize.x;
            theInputConnection->setTileSize(defaultSize);
         }
         blockSize->setValue(ossimString::toString(defaultSize.x));
         setProperty(blockSize);
         theInputConnection->setTileSize(defaultSize);
      }
   }
   allocateGdalDriverOptions();

   if(!theInputConnection->isMaster()) // MPI slave process...
   {
      theInputConnection->slaveProcessTiles();
      if(theNBandToIndexFilter.valid())
      {
        theInputConnection->connectMyInputTo(0, theNBandToIndexFilter->getInput());
        theNBandToIndexFilter = 0;
      }
      result = true;
   }
   else // MPI master process.
   {
      open();
      
      if(isOpen())
      {
         GDALDataType gdalType = getGdalDataType(theInputConnection->
                                                 getOutputScalarType());
         ossim_uint32 bandCount = theInputConnection->getNumberOfOutputBands();
				 
         theDataset = GDALCreate( theDriver ,
                                  theFilename.c_str(),
                                  (int)theAreaOfInterest.width(),
                                  (int)theAreaOfInterest.height(),
                                  (int)bandCount,
                                  gdalType,
                                  theGdalDriverOptions);

         // ESH 09/2009 -- If no raster bands do block write.
         int nRasterBands = GDALGetRasterCount( theDataset );

         if(theDataset && nRasterBands>0)
         {
            writeProjectionInfo(theDataset);

            if (theColorLutFlag)
            {
              writeColorMap(nRasterBands);
            }
            
            theInputConnection->setToStartOfSequence();
            ossimRefPtr<ossimImageData> currentTile =
               theInputConnection->getNextTile();
            ossimRefPtr<ossimImageData> outputTile =
               (ossimImageData*)currentTile->dup();
            outputTile->initialize();

            //---
            // DRB 20081017
            // Trac #404: Setting no data value for imagine (hfa) and j2k
            // causing view issues with ArcMap and Erdas Imagine when null
            // values are present in
            // valid image data.
            //---
            if (theDriver)
            {
               ossimString driverName = GDALGetDriverShortName(theDriver);
               if ( (driverName != "HFA") && (driverName != "JP2MrSID") && 
                    (driverName != "JP2KAK") && (driverName != "JPEG2000") )
               {
                  for(ossim_uint32 band = 0;
                      band < currentTile->getNumberOfBands();
                      ++band)
                  {
                     GDALRasterBandH aBand =
                        GDALGetRasterBand(theDataset, band+1);
                     if(aBand)
                     {
                        GDALSetRasterNoDataValue(
                           aBand, theInputConnection->getNullPixelValue(band));
                     }
                  }
                  if(traceDebug())
                  {
                     ossimNotify(ossimNotifyLevel_DEBUG)
                        << "GDALSetRasterNoDataValue called for driver: "
                        << driverName << std::endl;
                  }
               }
               else 
               {
                  if(traceDebug())
                  {
                     ossimNotify(ossimNotifyLevel_DEBUG)
                        << "GDALSetRasterNoDataValue not called for driver: "
                        << driverName << std::endl;
                  }
               }

               // ESH 05/2009: Add min/max to gdal metadata
               ossim_uint32 numberOfBands = currentTile->getNumberOfBands();
               for( ossim_uint32 idx = 0; idx < numberOfBands; ++idx )
               {
                  GDALRasterBandH aBand =
                     GDALGetRasterBand(theDataset, idx+1);
                  if(aBand)
                  {
                     // GDAL always assumes "area".
                     GDALSetMetadataItem( aBand, "AREA_OR_POINT", "Area", 0 );

                     double minPix = (double)currentTile->getMinPix( idx );
                     double maxPix = (double)currentTile->getMaxPix( idx );

                     // Only set the metadata if the values makes some sense.
                     if ( maxPix > 0.0 && minPix < maxPix )
                     {
                        char szValue[128];
                        sprintf( szValue, "%.14g", minPix );
                        GDALSetMetadataItem( aBand, "STATISTICS_MINIMUM", szValue, 0 );

                        sprintf( szValue, "%.14g", maxPix );
                        GDALSetMetadataItem( aBand, "STATISTICS_MAXIMUM", szValue, 0 );
                     }
                  }
               }
            }
   
            ossim_uint32 numberOfTiles =
               (theInputConnection->getNumberOfTiles()*
                theInputConnection->getNumberOfOutputBands());
   
            ossim_uint32 tileNumber = 0;
            while(currentTile.valid()&&(!needsAborting()))
            {
               ossimIrect clipRect =
                  currentTile->getImageRectangle().clipToRect(theAreaOfInterest);
               outputTile->setImageRectangle(clipRect);
               outputTile->loadTile(currentTile.get());
               //ossimIpt offset = currentTile->getOrigin() -
               //theAreaOfInterest.ul();
               ossimIpt offset = clipRect.ul() - theAreaOfInterest.ul();
      
               for(ossim_uint32 band = 0;
                   ((band < (currentTile->getNumberOfBands()))&&
                    (!needsAborting()));
                   ++band)
               {
                  GDALRasterBandH aBand=0;
                  aBand = GDALGetRasterBand(theDataset, band+1);

                  if(aBand)
                  {
                     GDALRasterIO( aBand,
                        GF_Write,
                        offset.x,
                        offset.y,
                        clipRect.width(),
                        clipRect.height(),
                        outputTile->getBuf(band),
                        outputTile->getWidth(),
                        outputTile->getHeight(),
                        gdalType,
                        0,
                        0);
                  }

                  ++tileNumber;
                  ossimProcessProgressEvent event(this,
                                                  ((double)tileNumber/
                                                   (double)numberOfTiles)*100.0,
                                                  "",
                                                  false);
                  
                  fireEvent(event);
               }
               currentTile = theInputConnection->getNextTile();
               
            }
            
            if(theDataset)
            {
               close();
               if((theGdalOverviewType != ossimGdalOverviewType_NONE)&&
                  (!needsAborting()))
               {
                  theDataset= GDALOpen( theFilename.c_str(), GA_Update );
                  
                  if( theDataset == 0 )
                  {
                     theDataset = GDALOpen( theFilename.c_str(), GA_ReadOnly );
                  }
                  buildGdalOverviews();
                  close();
               }
            }
            else
            {
               ossimNotify(ossimNotifyLevel_WARN)
                  << "ossimGdalWriter::writeFile unable to create dataset"
                  << std::endl;
               result = false;
            }
            
         } // End of "if (theDataset)"
         else
         {
            result = writeBlockFile();
         }
         
      } 
      else // open failed...
      {
         result = false;
         if(traceDebug())
         {
            CLOG << "Unable to open file for writing. Exiting ..."
                 << std::endl;
         }
      }

      if (result)
      {
         // Clean up files...
         postProcessOutput();
      }

   } // End of "else // MPI master process."

   if(theNBandToIndexFilter.valid())
   {
     theInputConnection->connectMyInputTo(0, theNBandToIndexFilter->getInput());
     theNBandToIndexFilter = 0;
   }

   return result;
   
} // End of: ossimGdalWriter::writeFile

bool ossimGdalWriter::writeBlockFile()
{
   theInputConnection->setAreaOfInterest(theAreaOfInterest);
   theInputConnection->setToStartOfSequence();
   
   MEMTiledDataset* tiledDataset = new MEMTiledDataset(theInputConnection.get());

   //---
   // DRB 20081017
   // Trac #404: Setting no data value for imagine (hfa) and j2k causing view
   // issues with ArcMap and Erdas Imagine when null values are present in
   // valid image data.
   //---
   if (theDriver)
   {
      ossimString driverName = GDALGetDriverShortName(theDriver);
      if ( (driverName == "HFA") || (driverName == "JP2MrSID") || 
           (driverName == "JP2KAK") || (driverName == "JPEG2000") )
      {
         tiledDataset->setNoDataValueFlag(false);
      }
   }

   ossimString driverName="null";
   if (theDriver)
   {
      driverName = GDALGetDriverShortName(theDriver);
   }

   writeProjectionInfo(tiledDataset);
   theDataset = GDALCreateCopy( theDriver,
                                theFilename.c_str(),
                                tiledDataset ,
                                true,
                                theGdalDriverOptions,
                                (GDALProgressFunc)&gdalProgressFunc,
                                this);
   
   if(theDataset&&!needsAborting())
   {
      if(theGdalOverviewType != ossimGdalOverviewType_NONE)
      {
         buildGdalOverviews();
      }
      close();
   }
   else
   {
      if(traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG) << "ossimGdalWriter::writeBlockFile(): Unable to create dataset for file " << theFilename << std::endl;
      }
      return false;
   }
   // Trac #299: Ingest tasks are failing when generating png thumbnails
   // ESH 08/2008: Added in a NULL check on theDataset, since GDALGetDriverShortName 
   // crashes the app if given a NULL pointer.
   if( theDataset && ossimString(GDALGetDriverShortName(theDataset)) == ossimString("PNG") )
   {
      setWriteExternalGeometryFlag(true);
   }

   return true;
}

void ossimGdalWriter::writeColorMap(int bands)
{
   if ( theDataset && theColorLut.valid() )
   {
      bool hasAlpha = (theColorLut->getNumberOfBands() == 4);

      for (int band = 0; band < bands; band++)
      {
         GDALRasterBandH aBand=0;
         aBand = GDALGetRasterBand(theDataset, band+1);
         if(aBand)
         {
            bool hasEntryLabel = false;
            ossim_uint32 entryNum = theColorLut->getNumberOfEntries();
            std::vector<ossimString> entryLabels = theColorLut->getEntryLabels(band);
            if (entryLabels.size() == entryNum)
            {
               hasEntryLabel = true;
            }
            
            GDALColorTable* gdalColorTable = new GDALColorTable();
            for(ossim_uint32 entryIndex = 0; entryIndex < entryNum; entryIndex++)
            {
               GDALColorEntry colorEntry;
               colorEntry.c1 = (*theColorLut)[entryIndex][0];
               colorEntry.c2 = (*theColorLut)[entryIndex][1];
               colorEntry.c3 = (*theColorLut)[entryIndex][2];
               colorEntry.c4 = hasAlpha ? (*theColorLut)[entryIndex][3] : 255;
#if 0
               if (hasEntryLabel)
               {
                  char* labelName = const_cast<char*>(entryLabels[entryIndex].c_str());
                  colorEntry.poLabel = labelName;
               }
               else
               {
                  colorEntry.poLabel = NULL;
               }
#endif
               
               gdalColorTable->SetColorEntry(entryIndex, &colorEntry);
            }
            
            GDALSetRasterColorTable(aBand, gdalColorTable);
            delete gdalColorTable;
         }
      }
   }
}

void ossimGdalWriter::writeProjectionInfo(GDALDatasetH dataset)
{
   // Get current input geometry
   ossimRefPtr<ossimImageGeometry> geom = theInputConnection->getImageGeometry();
   if(geom.valid())
   {
      const ossimMapProjection* mapProj =
         PTR_CAST(ossimMapProjection, geom->getProjection());

      if (mapProj)
      {
         ossim_uint32 pcs_code = mapProj->getPcsCode();
         if (pcs_code == 0)
            pcs_code = 32767;

         double geoTransform[6] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

         ossimDpt tiePoint;
         ossimDpt scale;
         if (mapProj->isGeographic())
         {
            // Get the ground tie point.
            ossimGpt gpt;
            mapProj->lineSampleToWorld(theAreaOfInterest.ul(), gpt);
            tiePoint = gpt;

            // Get the scale.
            scale.x =  mapProj->getDecimalDegreesPerPixel().x;
            scale.y = -mapProj->getDecimalDegreesPerPixel().y;
         }
         else
         {
            // Get the easting northing tie point.
            mapProj->lineSampleToEastingNorthing(theAreaOfInterest.ul(),
                                                 tiePoint);

            // Get the scale.
            scale.x =  mapProj->getMetersPerPixel().x;
            scale.y = -mapProj->getMetersPerPixel().y;
         }
         
         geoTransform[1] = scale.x;
         geoTransform[5] = scale.y;
         geoTransform[0] = tiePoint.x;
         geoTransform[3] = tiePoint.y;

         //---
         // Since OSSIM is pixel_is_point, shift the tie point by
         // half a pixel since GDAL output is pixel_is_area.
         //---
         geoTransform[0] -= fabs(scale.x) / 2.0;
         geoTransform[3] += fabs(scale.y) / 2.0;

         // Translate projection information to GDAL settable WKT
         ossimString wktString = "";
         if( mapProj->getProjectionName() == ossimString("ossimBngProjection") )
         {
            wktString = "PROJCS[\"OSGB 1936 / British National Grid\",GEOGCS[\"OSGB 1936\","
               "DATUM[\"OSGB_1936\",SPHEROID[\"Airy 1830\",6377563.396,299.3249646]],PRIMEM[\""
               "Greenwich\",0],UNIT[\"degree\",0.0174532925199433]],PROJECTION[\"Transverse_Mercator\"],"
               "PARAMETER[\"latitude_of_origin\",49],PARAMETER[\"central_meridian\",-2],PARAMETER[\""
               "scale_factor\",0.999601272],PARAMETER[\"false_easting\",400000],PARAMETER[\"false_"
               "northing\",-100000],UNIT[\"meter\",1]]";
         }
         else if (mapProj->getProjectionName() == ossimString("ossimAlbersProjection") )
         {
            wktString = "PROJCS[\"Albers Conical Equal Area\",GEOGCS[\"NAD83\",DATUM[\"North_American_Datum_1983\","
               "SPHEROID[\"GRS 1980\",6378137,298.257222101,AUTHORITY[\"EPSG\",\"7019\"]],"
               "TOWGS84[0,0,0,0,0,0,0],AUTHORITY[\"EPSG\",\"6269\"]],PRIMEM[\"Greenwich\",0,"
               "AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9108\"]],"
               "AUTHORITY[\"EPSG\",\"4269\"]],PROJECTION[\"Albers_Conic_Equal_Area\"],PARAMETER[\"standard_parallel_1\",29.5],"
               "PARAMETER[\"standard_parallel_2\",45.5],PARAMETER[\"latitude_of_center\",23],PARAMETER[\"longitude_of_center\",-96],"
               "PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"meters\",1]]";
         }
         else
         {
            ossimKeywordlist kwl;
            mapProj->saveState(kwl);
            wktString = translator.fromOssimKwl(kwl);
         }

         // Set GDAL projection and corner coordinates.
         GDALSetProjection(dataset, wktString.c_str());
         GDALSetGeoTransform(dataset, geoTransform);

      } // matches: if (mapProj)

   } // matches: if(geom.valid())
}

void ossimGdalWriter::setOptions(ossimKeywordlist& /* kwl */,
                                 const char* prefix)
{
   ossimString regExpression = ossimString("^(") +
                               ossimString(prefix) +
                               "option[0-9]+.)";

//    while(numberOfMatches < static_cast<unsigned long>(numberOfSources))
//    {
//       ossimString newPrefix = prefix;
//       newPrefix += ossimString("option");
//       newPrefix += ossimString::toString(i);
//       newPrefix += ossimString(".");

//       const char* name  = kwl.find(newPrefix.c_str(), "name");
//       const char* value = kwl.find(newPrefix.c_str(), "value");

//       if(ossimString(name) == "tilewidth")
//       {
//          theJpeg2000TileSize.x = ossimString(value).toInt();
//       }
//       else if(ossimString(name) == "tileheight")
//       {
//          theJpeg2000TileSize.y = ossimString(value).toInt();
//       }
//    }

}


GDALDataType ossimGdalWriter::getGdalDataType(ossimScalarType scalar)
{
   GDALDataType dataType = GDT_Unknown;
   
   switch(scalar)
   {
      case OSSIM_UCHAR:
      {
         dataType = GDT_Byte;
         break;
      }
      case OSSIM_USHORT11:
      case OSSIM_USHORT16:
      {
         dataType = GDT_UInt16;
         break;
      }
      case OSSIM_SSHORT16:
      {
         dataType = GDT_Int16;
         break;
      }
      case OSSIM_FLOAT:
      case OSSIM_NORMALIZED_FLOAT:
      {
         dataType = GDT_Float32;
         break;
      }
      case OSSIM_DOUBLE:
      case OSSIM_NORMALIZED_DOUBLE:
      {
         dataType = GDT_Float64;
         break;
      }
      default:
         break;
   }

   return dataType;
}

void ossimGdalWriter::allocateGdalDriverOptions()
{
   deleteGdalDriverOptions();
   ossimString regExpression =  (ossimString("^")+
                                 "property[0-9]+");
   vector<ossimString> keys = theDriverOptionValues.getSubstringKeyList( regExpression );

   if(keys.size())
   {
      ossim_uint32 i = 0;
      theGdalDriverOptions = new char*[keys.size()+1];
      for(i = 0 ; i < keys.size(); ++i)
      {
         ossimString name    = theDriverOptionValues.find(keys[i] +".name");
         ossimString value   = theDriverOptionValues.find(keys[i] +".value");

         //---
         // Special hack...
         // 
         // Lossless compression for non 8 bit data not supported for kakadu.
         //---
         if (theInputConnection.valid() && (GDALGetDriverByName("JP2KAK")) && (name == "QUALITY"))
         {
            if (theInputConnection->getOutputScalarType() != OSSIM_UINT8)
            {
               double d = value.toDouble();
               if (d >= 99.5)
               {
                  if (traceDebug())
                  {
                     ossimNotify(ossimNotifyLevel_DEBUG)
                        << "DEBUG:"
                        << "\nLossless quality not valid for non 8 bit data"
                        << "\nResetting to 99.0"
                        << std::endl;
                  }

                  value = "99.0";
               }
            }
         }

         // ESH 01/2009 -- For HFA handling of SRS's that come from pcs codes:
         // use new option in HFA driver (gdal-1.6.0). This option forces the
         // coordinate system tags to be more compatible with ArcGIS. We might 
         // have to add in a check for the pcs code here if that is possible.
         //---
         // Special hack...
         //---
         if ( (GDALGetDriverByName("HFA")) &&
              (name == "FORCETOPESTRING") )
         {
            value = "TRUE";
         }

         ossimString combine = name + "=" + value;
         
         theGdalDriverOptions[i] = new char[combine.size() + 1];
		   strcpy(theGdalDriverOptions[i], combine.c_str());
         
         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "\nkey:  " << keys[i]
               << "\nname:  " << name
               << "\nvalue: " << value
               << "\ngdal option:  " << combine
               << std::endl;
         }
      }
      theGdalDriverOptions[keys.size()] = 0;
   }
}

void ossimGdalWriter::appendGdalOption(const ossimString& name,
                                       const ossimString& value,
                                       bool replaceFlag)
{
   ossimString regExpression =  (ossimString("^")+
                                 "property[0-9]+");
   vector<ossimString> keys = theDriverOptionValues.getSubstringKeyList( regExpression );
   if(keys.size())
   {
      ossim_uint32 i = 0;
      for(i = 0 ; i < keys.size(); ++i)
      {
         ossimString tempName    = theDriverOptionValues.find(keys[i] +".name");
         if(tempName == name)
         {
            if(replaceFlag)
            {
               theDriverOptionValues.add(keys[i]+".value",
                                         value,
                                         true);
            }
            
            return;
         }
      }
   }
   ossimString prefix = "property" + ossimString::toString((ossim_uint32)keys.size()) + ".";
   
   theDriverOptionValues.add(prefix,
                             "name",
                             name,
                             true);
   theDriverOptionValues.add(prefix,
                             "value",
                             value,
                             true);
}

void ossimGdalWriter::deleteGdalDriverOptions()
{
   int i = 0;

   if(theGdalDriverOptions)
   {
      char* currentOption = theGdalDriverOptions[i];

      while(currentOption)
      {
         ++i;
         delete [] currentOption;
         currentOption = theGdalDriverOptions[i];
      }
      delete [] theGdalDriverOptions;
      theGdalDriverOptions = 0;
   }
}

ossimString ossimGdalWriter::convertToDriverName(const ossimString& imageTypeName)const
{
   ossimString strippedName = imageTypeName;
   strippedName = strippedName.substitute("gdal_", "");
   if(GDALGetDriverByName(strippedName.c_str()))
   {
      return strippedName;
   }

   // check for mime types
   //
   if(imageTypeName == "image/jp2")
   {
      if(GDALGetDriverByName("JP2KAK"))
      {
         return "JP2KAK";
      }
      else if(GDALGetDriverByName("JPEG2000"))
      {
         return "JPEG2000";
      }
      else if(GDALGetDriverByName("JP2MrSID"))
      {
         return "JP2MrSID";
      }
   }
   if(imageTypeName == "image/png")
   {
      return "PNG";
   }
   if(imageTypeName == "image/wbmp")
   {
	   return "BMP";
   }
   if(imageTypeName == "image/bmp")
   {
	   return "BMP";
   }
   if(imageTypeName == "image/jpeg")
   {
      return "JPEG";
   }
   if(imageTypeName == "image/gif")
   {
      return "GIF";
   }
   

   // keep these for backward compatibility
   //
   if( imageTypeName == "gdal_imagine_hfa")
   {
      return "HFA";
   }
   if(imageTypeName == "gdal_nitf_rgb_band_separate")
   {
      return "NITF";
   }
   if(imageTypeName == "gdal_jpeg2000")
   {
      if(GDALGetDriverByName("JP2KAK"))
      {
         return "JP2KAK";
      }
      else if(GDALGetDriverByName("JPEG2000"))
      {
         return "JPEG2000";
      }
      else if(GDALGetDriverByName("JP2MrSID"))
      {
         return "JP2MrSID";
      }
   }                                       
   if(imageTypeName == "gdal_arc_info_aig")
   {
      return "AIG";
   }
   if(imageTypeName == "gdal_arc_info_gio")
   {
      return "GIO";
   }
   if(imageTypeName == "gdal_arc_info_ascii_grid")
   {
      return "AAIGrid";
   }
   if(imageTypeName == "gdal_mrsid")
   {
      return "MrSID";
   }
   if(imageTypeName == "gdal_jp2mrsid")
   {
      return "JP2MrSID";
   }
   if(imageTypeName == "gdal_png")
   {
      return "PNG";
   }
   if(imageTypeName == "gdal_jpeg")
   {
      return "JPEG";
   }
      
   if(imageTypeName == "gdal_gif")
   {
      return "GIF";
   }
   if(imageTypeName == "gdal_dted")
   {
      return "DTED";
   }
   if(imageTypeName == "gdal_usgsdem")
   {
      return "USGSDEM";
   }
   if(imageTypeName == "gdal_bmp")
   {
      return "BMP";
   }
   if(imageTypeName == "gdal_raw_envi")
   {
      return "ENVI";
   }
   if(imageTypeName == "gdal_raw_esri")
   {
      return "EHdr";
   }
   if(imageTypeName == "gdal_raw_pci")
   {
      return "PAux";
   }
   if(imageTypeName == "gdal_pcidsk")
   {
      return "PCIDSK";
   }
   if(imageTypeName == "gdal_sdts")
   {
      return "SDTS";
   }
   if(imageTypeName == "gdal_jp2ecw")
   {
      return "JP2ECW";
   }
   if(imageTypeName == "gdal_ecw")
   {
      return "ECW";
   }
   return imageTypeName;
}

void ossimGdalWriter::getImageTypeList(std::vector<ossimString>& imageTypeList)const
{
   int driverCount = GDALGetDriverCount();
   int idx = 0;

   for(idx = 0; idx < driverCount; ++idx)
   {
      GDALDriverH driver =  GDALGetDriver(idx);

      
      if(driver)
      {
         const char* metaData = GDALGetMetadataItem(driver, GDAL_DCAP_CREATE, 0);
         if(metaData)
         {
            imageTypeList.push_back("gdal_" + ossimString(GDALGetDriverShortName(driver)));
         }
         else
         {
            metaData = GDALGetMetadataItem(driver, GDAL_DCAP_CREATECOPY, 0);
            if(metaData)
            {
               imageTypeList.push_back("gdal_" + ossimString(GDALGetDriverShortName(driver)));
            }
         }
      }
   }
}

bool ossimGdalWriter::hasImageType(const ossimString& imageType) const
{
   // check for non standard image types found in the
   // getImageTypeList
   //
   if((imageType == "gdal_imagine_hfa") ||
      (imageType == "gdal_nitf_rgb_band_separate") ||
      (imageType == "gdal_jpeg2000") ||
      (imageType == "gdal_jp2ecw") ||
      (imageType == "gdal_arc_info_aig") ||
      (imageType == "gdal_arc_info_gio") ||
      (imageType == "gdal_arc_info_ascii_grid") ||
      (imageType == "gdal_mrsid") ||
      (imageType == "image/mrsid") ||
      (imageType == "gdal_jp2mrsid") ||
      (imageType == "image/jp2mrsid") ||
      (imageType == "gdal_png") ||
      (imageType == "image/png") ||
      (imageType == "image/bmp") ||
      (imageType == "image/wbmp") ||
      (imageType == "gdal_jpeg") ||
      (imageType == "image/jpeg") ||
      (imageType == "gdal_gif") ||
      (imageType == "image/gif") ||
      (imageType == "gdal_dted") ||
      (imageType == "image/dted") ||
      (imageType == "gdal_bmp") ||
      (imageType == "gdal_xpm") ||
      (imageType == "image/x-xpixmap") ||
      (imageType == "gdal_raw_envi") ||
      (imageType == "gdal_raw_esri") ||
      (imageType == "gdal_raw_esri") ||
      (imageType == "gdal_pcidsk") ||
      (imageType == "gdal_sdts"))
   {
      return true;
   }
   
   return ossimImageFileWriter::hasImageType(imageType);
}

ossimString ossimGdalWriter::getExtension() const
{
   ossimString result;
   
   ossimString driverName = convertToDriverName(theOutputImageType);
   GDALDriverH driver =  GDALGetDriverByName(driverName.c_str());
   
   if(driver)
   {
      result  = ossimString(GDALGetMetadataItem(driver, GDAL_DMD_EXTENSION, 0));
      std::vector<ossimString> splitString;
      result.split(splitString, "/");
      if(!splitString.empty()) result = splitString[0];
   }   
   return result;
   
   //  we now use the metadata interface to get access to a drivers default file extension
#if 0
   ossimString imgTypeStr(theOutputImageType);
   imgTypeStr.downcase();

   // mime types
   if(imgTypeStr == "image/jp2")
   {
      return ossimString("jp2");
   }
   if(imgTypeStr == "image/png")
   {
      return ossimString("png");
   }
   if(imgTypeStr == "image/wbmp")
   {
      return ossimString("bmp");
   }
   if(imgTypeStr == "image/bmp")
   {
      return ossimString("bmp");
   }
   if(imgTypeStr == "image/jpeg")
   {
      return ossimString("jpg");
   }
   if(imgTypeStr == "image/gif")
   {
      return ossimString("gif");
   }


   // keep these for backward compatibility
   //
   if( imgTypeStr == "gdal_imagine_hfa")
   {
      return ossimString("hfa");
   }
   if(imgTypeStr == "gdal_nitf_rgb_band_separate")
   {
      return ossimString("ntf");
   }
   if(imgTypeStr == "gdal_jpeg2000")
   {
      return ossimString("jp2");
   }                                       
   if(imgTypeStr == "gdal_arc_info_aig")
   {
      return ossimString("aig");
   }
   if(imgTypeStr == "gdal_arc_info_gio")
   {
      return ossimString("gio");
   }
   if(imgTypeStr == "gdal_arc_info_ascii_grid")
   {
      return ossimString("aig");
   }
   if(imgTypeStr == "gdal_mrsid")
   {
      return ossimString("sid");
   }
   if(imgTypeStr == "gdal_jp2mrsid")
   {
      return ossimString("jp2");
   }
   if(imgTypeStr == "gdal_png")
   {
      return ossimString("png");
   }
   if(imgTypeStr == "gdal_jpeg")
   {
      return ossimString("jpg");
   }

   if(imgTypeStr == "gdal_gif")
   {
      return ossimString("gif");
   }
   if(imgTypeStr == "gdal_dted")
   {
      return ossimString("dte");
   }
   if(imgTypeStr == "gdal_usgsdem")
   {
      return ossimString("dem");
   }
   if(imgTypeStr == "gdal_bmp")
   {
      return ossimString("bmp");
   }
   if(imgTypeStr == "gdal_raw_envi")
   {
      return ossimString("raw");
   }
   if(imgTypeStr == "gdal_raw_esri")
   {
      return ossimString("raw");
   }
   if(imgTypeStr == "gdal_raw_pci")
   {
      return ossimString("raw");
   }
   if(imgTypeStr == "gdal_pcidsk")
   {
      return ossimString("pix");
   }
   if(imgTypeStr == "gdal_sdts")
   {
      return ossimString("sdt");
   }
   if(imgTypeStr == "gdal_jp2ecw")
   {
      return ossimString("jp2");
   }
   if(imgTypeStr == "gdal_ecw")
   {
      return ossimString("ecw");
   }
   return ossimString("ext");
#endif
}

void ossimGdalWriter::setProperty(ossimRefPtr<ossimProperty> property)
{
   if(!property.valid()) return;
	 ossimString propName = property->getName();
   if(!validProperty(property->getName()))
   {
      if(property->getName() == "image_type")
      {
         theDriverOptionValues.clear();
      }
      ossimImageFileWriter::setProperty(property);
      return;
   }

   ossimString driverName = convertToDriverName(theOutputImageType);
   ossimString name = property->getName();

   if(name == "HFA_USE_RRD")
   {
      bool value = property->valueToString().toBool();
      CPLSetConfigOption("HFA_USE_RRD", value?"YES":"NO");
   }
   else if(name == "gdal_overview_type")
   {
      theGdalOverviewType = gdalOverviewTypeFromString(property->valueToString());
   }
   else if (name == "format")
   {
      storeProperty(name,
                    property->valueToString());
   }
   else if(name == "lut_file")
   {
     theLutFilename = ossimFilename(property->valueToString());
     theColorLut->open(theLutFilename);
   }
   else if(name == "color_lut_flag")
   {
     theColorLutFlag = property->valueToString().toBool();
   }
   else
   {
      name = name.substitute(driverName+"_","");
      storeProperty(name,
                    property->valueToString());
   }
}

ossimRefPtr<ossimProperty> ossimGdalWriter::getProperty(const ossimString& name)const
{

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << "ossimGdalWriter::getProperty: entered..." << std::endl;
   }

   if(!validProperty(name))
   {
      return ossimImageFileWriter::getProperty(name);
   }

   if(name == "HFA_USE_RRD")
   {
      return  new ossimBooleanProperty(name, ossimString(CPLGetConfigOption(name.c_str(),"NO")).toBool());
   }
   else if(name == "gdal_overview_type")
   {
      ossimStringProperty* prop =  new ossimStringProperty(name, gdalOverviewTypeToString(), false);

      prop->addConstraint("none");
      prop->addConstraint("nearest");
      prop->addConstraint("average");
      
      return prop;
   }
   const ossimRefPtr<ossimXmlNode>  node = getGdalOptions();
   ossimString driverName = convertToDriverName(theOutputImageType);
   GDALDriverH driver =  GDALGetDriverByName(driverName.c_str());
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << "Driver name = " << driverName << "\n"
                                          << "name        = " << name << std::endl;
   }

   bool storedValueFlag=false;
   ossimString storedValue;

   if(node.valid()&&driver&&name.contains(driverName))
   {
      ossimString storedName = name;
      storedName = storedName.substitute(driverName+"_","");
      storedValueFlag = getStoredPropertyValue(storedName,
                                               storedValue);
      vector<ossimRefPtr<ossimXmlNode> > nodelist;
      node->findChildNodes("Option", nodelist);
      ossim_uint32 idx = 0;
      for(idx = 0; idx < nodelist.size(); ++idx)
      {
         ossimRefPtr<ossimXmlAttribute> nameAttribute = nodelist[idx]->findAttribute("name");
         if(nameAttribute.valid())
         {
            if((driverName+"_"+nameAttribute->getValue())== name)
            {
               ossimRefPtr<ossimXmlAttribute> type = nodelist[idx]->findAttribute("type");
               
               vector<ossimRefPtr<ossimXmlNode> > valuelist;
               
               if(type.valid())
               {
                  ossimRefPtr<ossimXmlAttribute> descriptionAttribute = nodelist[idx]->findAttribute("description");
                  nodelist[idx]->findChildNodes("Value",
                                       valuelist);
                  ossimString description;
                  ossimString typeValue = type->getValue();
                  typeValue = typeValue.downcase();

                  if(descriptionAttribute.valid())
                  {
                     description = descriptionAttribute->getValue();
                  }
                  if((typeValue == "int")||
                     (typeValue == "integer"))
                  {
                     ossimNumericProperty* prop = new ossimNumericProperty(name,
                                                                           storedValue);
                     prop->setNumericType(ossimNumericProperty::ossimNumericPropertyType_INT);
                     prop->setDescription(description);
                     return prop;
                  }
                  else if(typeValue == "unsigned int")
                  {
                     ossimNumericProperty* prop = new ossimNumericProperty(name,
                                                                           storedValue);
                     prop->setNumericType(ossimNumericProperty::ossimNumericPropertyType_UINT);
                     prop->setDescription(description);
                     return prop;
                  }
                  else if(typeValue == "float")
                  {
                     ossimNumericProperty* prop = new ossimNumericProperty(name,
                                                                           storedValue);
                     prop->setNumericType(ossimNumericProperty::ossimNumericPropertyType_FLOAT32);
                     prop->setDescription(description);
                     return prop;
                  }
                  else if(typeValue == "boolean")
                  {
                     ossimBooleanProperty* prop = new ossimBooleanProperty(name,
                                                                           storedValue.toBool());

                     prop->setDescription(description);
                     return prop;
                  }
                  else if((typeValue == "string-select")||
                          (typeValue == "string"))
                  {
                     ossim_uint32 idx2 = 0;
                     ossimString defaultValue = "";
                     
                     if(storedValueFlag)
                     {
                        defaultValue = storedValue;
                     }
                     else if(valuelist.size())
                     {
                        defaultValue = valuelist[0]->getText();
                     }
                     ossimStringProperty* prop = new ossimStringProperty(name,
                                                                         defaultValue,
                                                                         false);
                     for(idx2 = 0; idx2 < valuelist.size();++idx2)
                     {
                        prop->addConstraint(valuelist[idx2]->getText());
                     }
                     prop->setDescription(description);
                     return prop;
                  }
                  else
                  {
                     if(traceDebug())
                     {
                        ossimNotify(ossimNotifyLevel_INFO) << "************************TYPE VALUE = " << typeValue << " NOT HANDLED************"<< std::endl;
                        ossimNotify(ossimNotifyLevel_INFO) << "************************DEFAULTING TO TEXT ************" << std::endl;
                     }
                     ossim_uint32 idx2 = 0;
                     ossimString defaultValue = "";
                     
                     if(storedValueFlag)
                     {
                        defaultValue = storedValue;
                     }
                     else if(valuelist.size())
                     {
                        defaultValue = valuelist[0]->getText();
                     }
                     ossimStringProperty* prop = new ossimStringProperty(name,
                                                                         defaultValue,
                                                                         false);
                     for(idx2 = 0; idx2 < valuelist.size();++idx2)
                     {
                        prop->addConstraint(valuelist[idx2]->getText());
                     }
                     prop->setDescription(description);
                     return prop;
                  }
               }
            }
         }
      }
   }
   
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << "ossimGdalWriter::getProperty: entered..." << std::endl;
   }
   return 0;
}

void ossimGdalWriter::getPropertyNames(std::vector<ossimString>& propertyNames)const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << "ossimGdalWriter::getPropertyNames: entered......." << std::endl;
   }
   ossimImageFileWriter::getPropertyNames(propertyNames);
   propertyNames.push_back("gdal_overview_type");
   propertyNames.push_back("HFA_USE_RRD");
   getGdalPropertyNames(propertyNames);
}

void ossimGdalWriter::getGdalPropertyNames(std::vector<ossimString>& propertyNames)const
{
   ossimString driverName = convertToDriverName(theOutputImageType);
   GDALDriverH driver =  GDALGetDriverByName(driverName.c_str());

   if(driver)
   {
      const ossimRefPtr<ossimXmlNode>  node = getGdalOptions();
      
      if(node.valid())
      {
         vector<ossimRefPtr<ossimXmlNode> > nodelist;
         node->findChildNodes("Option", nodelist);

         if(traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG) << "GDAL XML PROPERTY LIST" << std::endl
                                                << node.get() << std::endl;
         }
         ossim_uint32 idx = 0;
         for(idx= 0; idx < nodelist.size(); ++idx)
         {
            ossimRefPtr<ossimXmlAttribute> name = nodelist[idx]->findAttribute("name");
            if(name.valid())
            {
               if(traceDebug())
               {
                  ossimNotify(ossimNotifyLevel_DEBUG) << "Adding property = " << name->getValue() << std::endl;
               }
               propertyNames.push_back(driverName + "_" + name->getValue());
            }
         }
      }         
   }
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << "ossimGdalWriter::getPropertyNames: leaving......." << std::endl;
   }
   
}

const ossimRefPtr<ossimXmlNode> ossimGdalWriter::getGdalOptions()const
{
   ossimString driverName = convertToDriverName(theOutputImageType);
   GDALDriverH driver =  GDALGetDriverByName(driverName.c_str());

   if(driver)
   {
      const char* metaData = GDALGetMetadataItem(driver, GDAL_DMD_CREATIONOPTIONLIST, 0);
      if(metaData)
      {
         istringstream in(metaData);
         ossimRefPtr<ossimXmlNode>  node = new ossimXmlNode(in);
         vector<ossimRefPtr<ossimXmlNode> > nodelist;
         node->findChildNodes("Option", nodelist);
         if(nodelist.size())
         {
            return node;
         }
      }
   }
   return 0;
}

void ossimGdalWriter::storeProperty(const ossimString& name,
                                    const ossimString& value)
{
   ossimString regExpression =  (ossimString("^") +
                                 "property[0-9]+");

   vector<ossimString> keys = theDriverOptionValues.getSubstringKeyList( regExpression );
   ossim_uint32 idx = 0;
   for(idx = 0; idx < keys.size(); ++idx)
   {
      ossimString storedName    = theDriverOptionValues.find(keys[idx] +".name");

      if(storedName == name)
      {
         theDriverOptionValues.add(keys[idx] +".value",
                                   value,
                                   true);
         return;
      }
   }
   ossimString prefix = (ossimString("property") +
                         ossimString::toString(theDriverOptionValues.getSize()/2) +
                         ".");
   theDriverOptionValues.add(prefix.c_str(),
                             "name",
                             name,
                             true);
   theDriverOptionValues.add(prefix.c_str(),
                             "value",
                             value,
                             true);
}

bool ossimGdalWriter::getStoredPropertyValue(const ossimString& name,
                                             ossimString& value)const
{
   ossimString regExpression =  (ossimString("^") +
                                 "property[0-9]+");

   vector<ossimString> keys = theDriverOptionValues.getSubstringKeyList( regExpression );
   ossim_uint32 idx = 0;
   for(idx = 0; idx < keys.size(); ++idx)
   {
      ossimString storedName    = theDriverOptionValues.find(keys[idx] +".name");
      if(storedName == name)
      {
         value   = ossimString(theDriverOptionValues.find(keys[idx] +".value"));
         return true;
      }
   }
   
   return false;
}

bool ossimGdalWriter::validProperty(const ossimString& name)const
{
   std::vector<ossimString> propertyNames;

   if((name == "gdal_overview_type")||
      (name == "HFA_USE_RRD") || name == "format")
   {
      return true;
   }
   getGdalPropertyNames(propertyNames);

   ossim_uint32 idx = 0;
   for(idx = 0; idx < propertyNames.size(); ++idx)
   {
      if(name == propertyNames[idx])
      {
         return true;
      }
   }

   return false;
}

ossimString ossimGdalWriter::gdalOverviewTypeToString()const
{
   switch(theGdalOverviewType)
   {
      case ossimGdalOverviewType_NONE:
      {
         return "none";
      }
      case ossimGdalOverviewType_NEAREST:
      {
         return "nearest";
      }
      case ossimGdalOverviewType_AVERAGE:
      {
         return "average";
      }
   }

   return "none";
}

ossimGdalWriter::ossimGdalOverviewType ossimGdalWriter::gdalOverviewTypeFromString(const ossimString& typeString)const
{
   ossimString tempType = typeString;
   tempType = tempType.downcase();
   tempType = tempType.trim();

   if(tempType == "nearest")
   {
      return ossimGdalOverviewType_NEAREST;
   }
   else if(tempType == "average")
   {
      return ossimGdalOverviewType_AVERAGE;
   }

   return ossimGdalOverviewType_NONE;
}

void ossimGdalWriter::buildGdalOverviews()
{

  if(!theDataset)
  {
    return;
  }
  ossimIrect bounds = theInputConnection->getBoundingRect();
  
  ossim_uint32 minValue = ossim::min((ossim_uint32)bounds.width(),
				   (ossim_uint32)bounds.height());
  
  int nLevels = static_cast<int>(std::log((double)minValue)/std::log(2.0));
  int idx = 0;
  if(nLevels)
    {
      ossim_int32* levelsPtr = new ossim_int32[nLevels];
      *levelsPtr = 2;
      for(idx = 1; idx < nLevels;++idx)
	{
	  levelsPtr[idx] = levelsPtr[idx-1]*2;
	}
      if(GDALBuildOverviews(theDataset,
			    gdalOverviewTypeToString().c_str(),
			    nLevels,
			    levelsPtr,
			    0,
			    0,
			    GDALTermProgress,
			    0)!=CE_None)
	{
	  ossimNotify(ossimNotifyLevel_WARN) << "ossimGdalWriter::buildGdalOverviews():  Overview building failed" 
					     << std::endl;
	}
      delete [] levelsPtr;
    }
}

void ossimGdalWriter::postProcessOutput() const
{
   //---
   // DRB 20081029
   // Trac #419:
   //
   // The file.aux.xml file contains nodata value keys which is causing view
   // issues with ArcMap and Erdas Imagine when null values are present in
   // valid image data.
   //--- 
   if (theDriver)
   {
      ossimString driverName = GDALGetDriverShortName(theDriver);
      if ( (driverName == "JP2MrSID") || (driverName != "JP2KAK") ||
           (driverName == "JPEG2000") )
      {
         ossimFilename auxXmlFile = theFilename;
         auxXmlFile += ".aux.xml";
         if ( auxXmlFile.exists() )
         {
            if ( auxXmlFile.remove() )
            {
               ossimNotify(ossimNotifyLevel_NOTICE)
                  << "ossimGdalWriter::postProcessOutput NOTICE:"
                  << "\nFile removed:  " << auxXmlFile << std::endl;
            }
         }
      }
   }
}

bool ossimGdalWriter::isInputDataIndexed()
{
   bool result = false;
   if ( theInputConnection.valid() )
   {
      result = theInputConnection->isIndexedData();
   }
   return result;
}
