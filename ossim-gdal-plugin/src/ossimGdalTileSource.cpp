//*******************************************************************
//
// License:  See top level LICENSE.txt file.
//
// Author:  Garrett Potts
//
// Description:
//
// Contains class implementaiton for the class "ossimGdalTileSource".
//
//*******************************************************************
//  $Id: ossimGdalTileSource.cpp 23191 2015-03-14 15:01:39Z dburken $

#include "ossimGdalTileSource.h"
#include "ossimGdalType.h"
#include "ossimOgcWktTranslator.h"
#include <ossim/base/ossimBooleanProperty.h>
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimGrect.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimPreferences.h>
#include <ossim/base/ossimStringProperty.h>
#include <ossim/base/ossimTieGptSet.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimUnitConversionTool.h>
#include <ossim/base/ossimUnitTypeLut.h>
#include <ossim/imaging/ossimImageDataFactory.h>
#include <ossim/imaging/ossimImageGeometryRegistry.h>
#include <ossim/imaging/ossimTiffTileSource.h>
#include <ossim/projection/ossimBilinearProjection.h>
#include <ossim/projection/ossimMapProjection.h>
#include <ossim/projection/ossimPolynomProjection.h>
#include <ossim/projection/ossimProjection.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <ossim/projection/ossimRpcSolver.h>
#include <ossim/projection/ossimRpcProjection.h>
#include <ossim/support_data/ossimFgdcXmlDoc.h>

#include <gdal_priv.h>
#include <cpl_string.h>

#include <sstream>

RTTI_DEF1(ossimGdalTileSource, "ossimGdalTileSource", ossimImageHandler)

static ossimOgcWktTranslator wktTranslator;
static ossimTrace traceDebug("ossimGdalTileSource:debug");

static const char DRIVER_SHORT_NAME_KW[] = "driver_short_name";
static const char PRESERVE_PALETTE_KW[]  = "preserve_palette";


using namespace ossim;

//*******************************************************************
// Public Constructor:
//*******************************************************************
ossimGdalTileSource::ossimGdalTileSource()
   :
      ossimImageHandler(),
      theDataset(0),
      theTile(0),
      theSingleBandTile(0),
      theImageBound(),
      theMinPixValues(0),
      theMaxPixValues(0),
      theNullPixValues(0),
      theEntryNumberToRender(0),
      theSubDatasets(),
      theIsComplexFlag(false),
      theAlphaChannelFlag(false),
      m_preservePaletteIndexesFlag(false),
      m_outputBandList(0),
      m_isBlocked(false)
{
   // Pick up any default settings from preference file if set.
   getDefaults();
}

//*******************************************************************
// Destructor:
//*******************************************************************
ossimGdalTileSource::~ossimGdalTileSource()
{
   close();
}

void ossimGdalTileSource::close()
{
   if(theDataset)
   {
      GDALClose(theDataset);
      theDataset = 0;
   }

   theTile = 0;
   theSingleBandTile = 0;

   if(theMinPixValues)
   {
      delete [] theMinPixValues;
      theMinPixValues = 0;
   }
   if(theMaxPixValues)
   {
      delete [] theMaxPixValues;
      theMaxPixValues = 0;
   }
   if(theNullPixValues)
   {
      delete [] theNullPixValues;
      theNullPixValues = 0;
   }
   deleteRlevelCache();

   m_preservePaletteIndexesFlag = false;
   theAlphaChannelFlag = false;
   theIsComplexFlag = false;
}

//*******************************************************************
// Public Method:
//*******************************************************************
bool ossimGdalTileSource::open()
{
   
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << "ossimGdalTileSource::open() DEBUG: entered ..."
         << std::endl;
   }

   if(isOpen())
   {
      close();
   }
   ossimString driverNameTmp;

   if (theSubDatasets.size() == 0)
   {
      // Note:  Cannot feed GDALOpen a NULL string!
      if (theImageFile.size())
      {
         theDataset = GDALOpen(theImageFile.c_str(), GA_ReadOnly); 
         if( theDataset == 0 )
         {
            return false;
         }
      }
      else
      {
         return false;
      }

      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "ossimGdalTileSource::open DEBUG:"
            << "\nOpened image:  "
            << theImageFile.c_str()
            << std::endl;
      }

      // Check if it is nitf data for deciding whether or not open each sub-dataset
      //This will be removed eventually when ossim can handle 2GB nitf file.
      GDALDriverH driverTmp = GDALGetDatasetDriver(theDataset);
      bool isNtif = false;
      if (driverTmp != 0)
      {
         driverNameTmp = ossimString(GDALGetDriverShortName(driverTmp));
         driverNameTmp = driverNameTmp.upcase();
         if (driverNameTmp == "NITF")
         {
            isNtif = true;
         }
      }
      
      // Check for sub data sets...
      char** papszMetadata = GDALGetMetadata( theDataset, "SUBDATASETS" );
      if( CSLCount(papszMetadata) > 0 )
      {
         theSubDatasets.clear();
         
         for( int i = 0; papszMetadata[i] != 0; ++i )
         {
            ossimString os = papszMetadata[i];
            if (os.contains("_NAME="))
            {
               //Sub sets have already been set. Open each sub-dataset really slow down the process
               //specially for hdf data which sometimes has over 100 sub-datasets. Since following code
               //only for ntif cloud checking, so only open each sub-dataset here if the dataset is
               //nitf. This will be removed eventually when ossim can handle 2GB nitf file. 
               //Otherwise open a sub-dataset when setCurrentEntry() gets called.
               if (isNtif)
               {
                  GDALDatasetH subDataset = GDALOpen(filterSubDatasetsString(os),
                     GA_ReadOnly);
                  if ( subDataset != 0 )
                  {
                     // ESH 12/2008: Ticket #482 
                     // "Worldview ingest should ignore subimages when NITF_ICAT=CLOUD"
                     // Hack: Ignore NITF subimages marked as cloud layers.
                     ossimString nitfIcatTag( GDALGetMetadataItem( subDataset, "NITF_ICAT", "" ) );
                     if ( nitfIcatTag.contains("CLOUD") == false )
                     {
                        theSubDatasets.push_back(filterSubDatasetsString(os));
                     }
                  }
                  GDALClose(subDataset);
               }
               else
               {
                  theSubDatasets.push_back(filterSubDatasetsString(os));
               }
            }
         }

         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "ossimGdalTileSource::open DEBUG:" << std::endl;
            for (ossim_uint32 idx = 0; idx < theSubDatasets.size(); ++idx)
            {
               ossimNotify(ossimNotifyLevel_DEBUG)
                  << "Sub_data_set[" << idx << "] "
                  << theSubDatasets[idx] << std::endl;
            }
         }

         //---
         // Have multiple entries.  We're going to default to open the first
         // entry like cidcadrg.
         //---
         close();
         
         theDataset = GDALOpen(theSubDatasets[theEntryNumberToRender].c_str(),
                               GA_ReadOnly);
         if (theDataset == 0)
         {
            if(traceDebug())
            {
               ossimNotify(ossimNotifyLevel_WARN) << "Could not open sub dataset = " << theSubDatasets[theEntryNumberToRender] << "\n";
            }
            return false;
         }
         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "ossimGdalTileSource::open DEBUG:"
               << "\nOpened sub data set:  "
               << theSubDatasets[theEntryNumberToRender].c_str()
               << std::endl;
         }
         
      }  // End of has subsets block.
      
   }  // End of "if (theSubdatasets.size() == 0)"
   else
   {
      // Sub sets have already been set.
      theDataset = GDALOpen(theSubDatasets[theEntryNumberToRender].c_str(),
                            GA_ReadOnly);
      if (theDataset == 0)
      {
         return false;
      }
      
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "ossimGdalTileSource::open DEBUG:"
            << "\nOpened sub data set:  "
            << theSubDatasets[theEntryNumberToRender].c_str()
            << std::endl;
      }
   }

   // Set the driver.
   theDriver = GDALGetDatasetDriver( theDataset );

   if(!theDriver) return false;

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalTileSource::open Driver: "
         << GDALGetDriverShortName( theDriver )
         << "/" << GDALGetDriverLongName( theDriver ) << std::endl;
   }

   theGdtType = GDT_Byte;
   theOutputGdtType = GDT_Byte;

   if(getNumberOfInputBands() < 1 )
   {
      if(CSLCount(GDALGetMetadata(theDataset, "SUBDATASETS")))
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "ossimGdalTileSource::open WARNING:"
            << "\nHas multiple sub datasets and need to set the data before"
            << " we can get to the bands" << std::endl;
      }
      
      close();
      ossimNotify(ossimNotifyLevel_WARN)
      << "ossimGdalTileSource::open WARNING:"
      << "\nNo band data present in ossimGdalTileSource::open" << std::endl;
      return false;
   }
   
   ossim_int32 i = 0;
   GDALRasterBandH bBand = GDALGetRasterBand( theDataset, 1 );
   theGdtType  = GDALGetRasterDataType(bBand);

   // Establish raster-pixel alignment type:
   thePixelType = OSSIM_PIXEL_IS_POINT; //default
   char** papszMetadata = GDALGetMetadata( bBand, NULL );
   if (CSLCount(papszMetadata) > 0)
   {
      for(int i = 0; papszMetadata[i] != NULL; i++ )
      {
         ossimString metaStr = papszMetadata[i];
         if (metaStr.contains("AREA_OR_POINT"))
         {
            ossimString pixel_is_point_or_area = metaStr.split("=")[1];
            pixel_is_point_or_area.downcase();
            if (pixel_is_point_or_area.contains("area"))
               thePixelType = OSSIM_PIXEL_IS_AREA;
            break;
         }
      }
   }

   if(!isIndexed(1))
   {
      for(i = 0; i  < GDALGetRasterCount(theDataset); ++i)
      {
         if(theGdtType != GDALGetRasterDataType(GDALGetRasterBand( theDataset,
                                                                   i+1 )))
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << "ossimGdalTileSource::open WARNING"
               << "\nWe currently do not support different scalar type bands."
               << std::endl;
            close();
            return false;
         }
//          if(GDALGetRasterColorInterpretation(GDALGetRasterBand(
//                                                 theDataset,
//                                                 i+1 )) == GCI_PaletteIndex)
//          {
//             ossimNotify(ossimNotifyLevel_WARN)
//                << "ossimGdalTileSource::open WARNING"
//                << "Index palette bands are not supported." << endl;
//             close();
//             return false;
//          }
      }
   }
   theOutputGdtType = theGdtType;
   switch(theGdtType)
   {
      case GDT_CInt16:
      {
//          theOutputGdtType = GDT_Int16;
         theIsComplexFlag = true;
         break;
      }
      case GDT_CInt32:
      {
//          theOutputGdtType = GDT_Int32;
         theIsComplexFlag = true;
         break;
      }
      case GDT_CFloat32:
      {
//          theOutputGdtType = GDT_Float32;
         theIsComplexFlag = true;
         break;
      }
      case GDT_CFloat64:
      {
//          theOutputGdtType = GDT_Float64;
         theIsComplexFlag = true;
         break;
      }
      default:
      {
         theIsComplexFlag = false;
         break;
      }
   }

   if((ossimString(GDALGetDriverShortName( theDriver )) == "PNG")&&
      (getNumberOfInputBands() == 4))
   {
      theAlphaChannelFlag = true;
   }
   populateLut();
   computeMinMax();
   completeOpen();
   
   theTile = ossimImageDataFactory::instance()->create(this, this);
   theSingleBandTile = ossimImageDataFactory::instance()->create(this, getInputScalarType(), 1);

   if ( m_preservePaletteIndexesFlag )
   {
      theTile->setIndexedFlag(true);
      theSingleBandTile->setIndexedFlag(true);
   }

   theTile->initialize();
   theSingleBandTile->initialize();

   theGdalBuffer.resize(0);
   if(theIsComplexFlag)
   {
      theGdalBuffer.resize(theSingleBandTile->getSizePerBandInBytes()*2);
   }
   
   theImageBound = ossimIrect(0
                              ,0
                              ,GDALGetRasterXSize(theDataset)-1
                              ,GDALGetRasterYSize(theDataset)-1);
   
   if(traceDebug())
   {
      
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalTileSoruce::open\n"
         << "data type = " << theTile->getScalarType() << std::endl;
      
      for(ossim_uint32 i = 0; i < getNumberOfInputBands(); ++i)
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "min pix   " << i << ":" << getMinPixelValue(i)
            << "\nmax pix   " << i << ":" << getMaxPixelValue(i)
            << "\nnull pix  " << i << ":" << getNullPixelValue(i) << std::endl;
      }
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "theTile:\n" << *theTile
         << "theSingleBandTile:\n" << *theSingleBandTile
         << std::endl;
   }
   
   int xSize=0, ySize=0;
   GDALGetBlockSize(GDALGetRasterBand( theDataset, 1 ),
                    &xSize,
                    &ySize);
   // Garrett: 
   // For now,  I will just enable te block reads with the JPIP and JP2 tyope drivers
   // if it's a block encoded image
   //
   // 
   if(driverNameTmp.contains("JPIP")||driverNameTmp.contains("JP2"))
   {
      m_isBlocked = ((xSize > 1)&&(ySize > 1));
   }
   else
   {
      m_isBlocked = false;
   }
   if(m_isBlocked)
   {
      setRlevelCache();
   }
   return true;
}

ossimRefPtr<ossimImageData> ossimGdalTileSource::getTile(const  ossimIrect& tileRect,
                                                         ossim_uint32 resLevel)
{
   // This tile source bypassed, or invalid res level, return a blank tile.
   if (!isSourceEnabled() || !theDataset)
   {
      return ossimRefPtr<ossimImageData>();
   }

   // Check for intersect.
   ossimIrect imageBound = getBoundingRect(resLevel);
   if(!tileRect.intersects(imageBound))
   {
      theTile->setImageRectangle(tileRect);
      theTile->makeBlank();
      return theTile;
   }

   if(m_isBlocked)
   {
      return getTileBlockRead(tileRect, resLevel);
   }
  // Check for overview.
   if(resLevel)
   {
      if ( m_preservePaletteIndexesFlag )
      {
         // No mechanism for this coded for reduce resolution levels.
         throw ossimException( std::string("ossimGdalTileSource::getTile ERROR: Accessing reduced resolution level with the preservePaletteIndexesFlag set!") );
      }
         
      if(theOverview.valid() && theOverview->isValidRLevel(resLevel))
      {
         ossimRefPtr<ossimImageData> tileData = theOverview->getTile(tileRect, resLevel);
         tileData->setScalarType(getOutputScalarType());
         return tileData;
      }
      
#if 1
      //---
      // Note: This code was shut off since we currently don't utilize gdal overviews.
      // ossimGdalTileSource::getNumberOfDecimationLevels has been fixed accordingly.
      // drb - 20110503
      //---
      else if(GDALGetRasterCount(theDataset))
      {
         GDALRasterBandH band = GDALGetRasterBand(theDataset, 1);
         if(static_cast<int>(resLevel) > GDALGetOverviewCount(band))
         {
            return ossimRefPtr<ossimImageData>();
         }
      }
#endif
   }

   // Set the rectangle of the tile.
   theTile->setImageRectangle(tileRect);

   // Compute clip rectangle with respect to the image bounds.
   ossimIrect clipRect   = tileRect.clipToRect(imageBound);

   theSingleBandTile->setImageRectangle(clipRect);

   if (tileRect.completely_within(clipRect) == false)
   {
      // Not filling whole tile so blank it out first.
      theTile->makeBlank();
   }

   // Always blank the single band tile.
   theSingleBandTile->makeBlank();
   
   ossim_uint32 anOssimBandIndex = 0;
   ossim_uint32 aGdalBandIndex   = 1;

   ossim_uint32 rasterCount = GDALGetRasterCount(theDataset);
   if (m_outputBandList.size() > 0)
   {
      rasterCount = (ossim_uint32) m_outputBandList.size();
   }
   
   ossim_uint32 outputBandIndex = 0;
   for(ossim_uint32 aBandIndex =1; aBandIndex <= rasterCount; ++aBandIndex)
   {
      if (m_outputBandList.size() > 0)
      {
         aGdalBandIndex = m_outputBandList[outputBandIndex] + 1;
         outputBandIndex++;
      }
      else
      {
         aGdalBandIndex = aBandIndex;
      }
      GDALRasterBandH aBand = resolveRasterBand( resLevel, aGdalBandIndex );
      if ( aBand )
      {
         bool bReadSuccess;
         if(!theIsComplexFlag)
         {
            bReadSuccess = (GDALRasterIO(aBand
                                         , GF_Read
                                         , clipRect.ul().x
                                         , clipRect.ul().y
                                         , clipRect.width()
                                         , clipRect.height()
                                         , theSingleBandTile->getBuf()
                                         , clipRect.width()
                                         , clipRect.height()
                                         , theOutputGdtType
                                         , 0 
                                         , 0 ) == CE_None) ? true : false;

            if ( bReadSuccess == true )
            {
               if(isIndexed(aGdalBandIndex))
               {
                  if(isIndexTo3Band(aGdalBandIndex))
                  {
                     if ( m_preservePaletteIndexesFlag )
                     {
                        theTile->loadBand((void*)theSingleBandTile->getBuf(),
                                          clipRect, anOssimBandIndex);
                        anOssimBandIndex += 1;
                     }
                     else
                     {
                        loadIndexTo3BandTile(clipRect, aGdalBandIndex, anOssimBandIndex);
                        anOssimBandIndex+=3;
                     }
                  }
                  else if(isIndexTo1Band(aGdalBandIndex))
                  { 
                     // ??? Ignoring (drb)
                     anOssimBandIndex += 1;
                  }
               }
               else
               {
                  if(theAlphaChannelFlag&&(aGdalBandIndex==rasterCount))
                  {
                     theTile->nullTileAlpha((ossim_uint8*)theSingleBandTile->getBuf(),
                                            theSingleBandTile->getImageRectangle(),
                                            clipRect,
                                            false);
                  }
                  else
                  {
                     // Note fix rectangle to represent theBuffer's rect in image space.
                     theTile->loadBand((void*)theSingleBandTile->getBuf()
                                       , clipRect
                                       , anOssimBandIndex);
                     ++anOssimBandIndex;
                  }
               }
            }
            else
            {
               ++anOssimBandIndex;
            }
         } 
         else // matches if(!theIsComplexFlag){} 
         {
            bReadSuccess = (GDALRasterIO(aBand
                                         , GF_Read
                                         , clipRect.ul().x
                                         , clipRect.ul().y
                                         , clipRect.width()
                                         , clipRect.height()
                                         , &theGdalBuffer.front()
                                         , clipRect.width()
                                         , clipRect.height()
                                         , theOutputGdtType
                                         , 0
                                         , 0 ) == CE_None) ? true : false;
            if (  bReadSuccess == true )
            {
               ossim_uint32 byteSize = ossim::scalarSizeInBytes(theSingleBandTile->getScalarType());
               ossim_uint32 byteSize2 = byteSize*2;
               ossim_uint8* complexBufPtr = (ossim_uint8*)(&theGdalBuffer.front()); // start at first real part
               ossim_uint8* outBufPtr  = (ossim_uint8*)(theSingleBandTile->getBuf());
               ossim_uint32 idxMax = theSingleBandTile->getWidth()*theSingleBandTile->getHeight();
               ossim_uint32 idx = 0;
               for(idx = 0; idx < idxMax; ++idx)
               {
                  memcpy(outBufPtr, complexBufPtr, byteSize);
                  complexBufPtr += byteSize2;
                  outBufPtr     += byteSize;
               }
               theTile->loadBand((void*)theSingleBandTile->getBuf()
                                 , clipRect
                                 , anOssimBandIndex);
               ++anOssimBandIndex;
               complexBufPtr = (ossim_uint8*)(&theGdalBuffer.front()) + byteSize; // start at first imaginary part
               outBufPtr  = (ossim_uint8*)(theSingleBandTile->getBuf());
               for(idx = 0; idx < idxMax; ++idx)
               {
                  memcpy(outBufPtr, complexBufPtr, byteSize);
                  complexBufPtr += byteSize2;
                  outBufPtr     += byteSize;
               }
               theTile->loadBand((void*)theSingleBandTile->getBuf()
                                 , clipRect
                                 , anOssimBandIndex);
               ++anOssimBandIndex;
            }
            else
            {
               anOssimBandIndex += 2;
            }
         }
      }
   }

   theTile->validate();
   return theTile;
}

ossimRefPtr<ossimImageData> ossimGdalTileSource::getTileBlockRead(const ossimIrect& tileRect,
                                                                  ossim_uint32 resLevel)
{
   ossimRefPtr<ossimImageData> result;
   ossimIrect imageBound = getBoundingRect(resLevel);
   theTile->setImageRectangle(tileRect);
   
   // Compute clip rectangle with respect to the image bounds.
   ossimIrect clipRect   = tileRect.clipToRect(imageBound);
   
   if (tileRect.completely_within(clipRect) == false)
   {
      // Not filling whole tile so blank it out first.
      theTile->makeBlank();
   }
   if(m_isBlocked)
   {
      int xSize=0, ySize=0;
      GDALGetBlockSize(resolveRasterBand( resLevel, 1 ),
                       &xSize,
                       &ySize);
      ossimIrect blockRect = clipRect;
      blockRect.stretchToTileBoundary(ossimIpt(xSize, ySize));
      blockRect = blockRect.clipToRect(getBoundingRect(resLevel));
      
      // we need to cache here
      //
      ossim_int64 x = blockRect.ul().x;
      ossim_int64 y = blockRect.ul().y;
      ossim_int64 maxx = blockRect.lr().x;
      ossim_int64 maxy = blockRect.lr().y;
      ossim_uint32 aGdalBandIndex   = 1;
      ossim_uint32 rasterCount = GDALGetRasterCount(theDataset);
      if (m_outputBandList.size() > 0)
      {
         rasterCount = m_outputBandList.size();
      }
    
     ossim_uint32 outputBandIndex = 0;
      for(;x < maxx;x+=xSize)
      {
         for(;y < maxy;y+=ySize)
         {
            ossimIpt origin(x,y);
            
            ossimRefPtr<ossimImageData> cacheTile = ossimAppFixedTileCache::instance()->getTile(m_rlevelBlockCache[resLevel], origin);
            
            if(!cacheTile.valid())
            {
               ossimIrect rect(origin.x,origin.y,origin.x+xSize-1, origin.y+ySize-1);
               theSingleBandTile->setImageRectangle(rect);
               ossimIrect validRect = rect.clipToRect(imageBound);
               cacheTile = ossimImageDataFactory::instance()->create(this, this);
               cacheTile->setImageRectangle(validRect);
               cacheTile->initialize();                          
               for(ossim_uint32 aBandIndex =1; aBandIndex <= rasterCount; ++aBandIndex)
               {
                  if (m_outputBandList.size() > 0)
                  {
                     aGdalBandIndex = m_outputBandList[outputBandIndex] + 1;
                     outputBandIndex++;
                  }
                  else
                  {
                     aGdalBandIndex = aBandIndex;
                  }
                  GDALRasterBandH aBand = resolveRasterBand( resLevel, aGdalBandIndex );
                  if ( aBand )
                  {
                     try{
                        
                       bool bReadSuccess =  (GDALReadBlock(aBand, x/xSize, y/ySize, theSingleBandTile->getBuf() )== CE_None) ? true : false;
                        if(bReadSuccess)
                        {
                           cacheTile->loadBand(theSingleBandTile->getBuf(), theSingleBandTile->getImageRectangle(), aBandIndex-1);
                        }
                     }
                     catch(...)
                     {
                     }
                  }
               }
               cacheTile->validate();
               ossimAppFixedTileCache::instance()->addTile(m_rlevelBlockCache[resLevel], cacheTile.get(), false);
            }
            theTile->loadTile(cacheTile->getBuf(), cacheTile->getImageRectangle(), OSSIM_BSQ);
            result = theTile;
         }
      }
   }
   if(result.valid()) result->validate();
   
   return result;
}

//*******************************************************************
// Public Method:
//*******************************************************************
ossim_uint32 ossimGdalTileSource::getNumberOfInputBands() const
{
   if(isOpen())
   {
       if (m_outputBandList.size() > 0)
       {
          return (ossim_uint32) m_outputBandList.size();
       }

      if(isIndexed(1))
      {
         return getIndexBandOutputNumber(1);
      }

      if(!theIsComplexFlag)
      {
         return GDALGetRasterCount(theDataset);
      }
      return GDALGetRasterCount(theDataset)*2;
   }
   return 0;
}

/*!
 * Returns the number of bands in a tile returned from this TileSource.
 * Note: we are supporting sources that can have multiple data objects.
 * If you want to know the scalar type of an object you can pass in the 
 */
ossim_uint32 ossimGdalTileSource::getNumberOfOutputBands() const
{
   ossim_uint32 result = 0;
   if( isIndexTo3Band() )
   {
      if ( m_preservePaletteIndexesFlag )
      {
         result = 1; // Passing palette indexes not expanded rgb values.
      }
      else
      {
         result = 3;
      }
   }
   else if (theAlphaChannelFlag)
   {
      result = 3;
   }
   else
   {
      // the number of outputs will be the same as the number of inputs
      result = getNumberOfInputBands();
   }
   return result;
}

//*******************************************************************
// Public Method:
//*******************************************************************
ossim_uint32 ossimGdalTileSource::getNumberOfLines(ossim_uint32 reduced_res_level) const
{
   
   ossim_uint32 result = 0;
   if ( isOpen() && isValidRLevel(reduced_res_level) )
   {
      if(theOverview.valid() && theOverview->isValidRLevel(reduced_res_level))
      {
         result = theOverview->getNumberOfLines(reduced_res_level);
      }
      else
      {
         int x, y;
         getMaxSize(reduced_res_level, x, y);
         result = y;
      }
   }
   
   return result;
}

//*******************************************************************
// Public Method:
//*******************************************************************
ossim_uint32 ossimGdalTileSource::getNumberOfSamples(ossim_uint32 reduced_res_level) const
{
   ossim_uint32 result = 0;
   if ( isOpen() && isValidRLevel(reduced_res_level) )
   {
      if(theOverview.valid() && theOverview->isValidRLevel(reduced_res_level))
      {
         result = theOverview->getNumberOfLines(reduced_res_level);
      }
      else
      {
         int x, y;
         getMaxSize(reduced_res_level, x, y);
         result = x;
      }
   }
   return result;
}

//*******************************************************************
// Public Method:
//*******************************************************************
ossimIrect ossimGdalTileSource::getImageRectangle(ossim_uint32 reduced_res_level) const
{
   ossimIrect result;
   result.makeNan();
   
   int x, y;
   getMaxSize(reduced_res_level, x, y);
   if(x&&y)
   {
      return ossimIrect(0,
                        0,
                        x - 1,
                        y - 1);
   }

   if (result.hasNans())
   {
      return ossimImageHandler::getImageRectangle(reduced_res_level);
   }
   
   return result;
}

//*******************************************************************
// Public Method:
//*******************************************************************
void ossimGdalTileSource::getDecimationFactor(ossim_uint32 resLevel,
                                              ossimDpt& result) const
{
   if (resLevel == 0)
   {
      result.x = 1.0;
      result.y = 1.0;
   }
   else
   {
      /* 
         ESH 02/2009 -- No longer assume powers of 2 reduction 
         in linear size from resLevel 0 (Tickets # 467,529).
      */
      ossim_int32 x  = getNumberOfLines(resLevel);
      ossim_int32 x0 = getNumberOfLines(0);

      if ( x > 0 && x0 > 0 )
      {
         result.x = ((double)x) / x0;
      }
      else
      {
         result.x = 1.0 / (1<<resLevel);
      }

      result.y = result.x;
   }
}

GDALDriverH ossimGdalTileSource::getDriver()
{
   return theDriver;
}

//*******************************************************************
// Public method:
//*******************************************************************
bool ossimGdalTileSource::saveState(ossimKeywordlist& kwl,
                                    const char* prefix) const
{
   kwl.add(prefix, 
	   "entry",
	   theEntryNumberToRender,
	   true);
   if (theEntryNumberToRender < theSubDatasets.size())
   {
      kwl.add(prefix,
              "entry_string",
              theSubDatasets[theEntryNumberToRender].c_str(),
              true);
   }
   
   bool result = ossimImageHandler::saveState(kwl, prefix);

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "DEBUG:"
         << "\nossimGdalTileSource::saveState keywordlist:\n"
         << kwl
         << std::endl;
   }

   return result;
}

//*******************************************************************
// Public method:
//*******************************************************************
bool ossimGdalTileSource::loadState(const ossimKeywordlist& kwl,
                                    const char* prefix)
{
   // Call base class first to set "theImageFile".
   if (ossimImageHandler::loadState(kwl, prefix))
   {
      // Check for an entry.
      const char* lookup = kwl.find(prefix, "entry");
      if(lookup)
      {
         ossim_uint32 entry = ossimString(lookup).toUInt32();

         // setCurrentEntry calls open but does not have a return.
         setCurrentEntry(entry);
         return isOpen();
      }
      lookup = kwl.find(prefix, PRESERVE_PALETTE_KW);
      if ( lookup )
      {
         setPreservePaletteIndexesFlag(ossimString(lookup).toBool());
      }

      return open();
   }

   return false;
}

//*****************************************************************************
//! Returns the image geometry object associated with this tile source or
//! NULL if non defined.
//! The geometry contains full-to-local image transform as well as projection
//! (image-to-world).
//*****************************************************************************
ossimRefPtr<ossimImageGeometry> ossimGdalTileSource::getImageGeometry()
{
   if ( !theGeometry )
   {
      //---
      // Check factory for external geom:
      //---
      theGeometry = getExternalImageGeometry();

      if ( !theGeometry )
      {
         //---
         // WARNING:
         // Must create/set the geometry at this point or the next call to
         // ossimImageGeometryRegistry::extendGeometry will put us in an infinite loop
         // as it does a recursive call back to ossimImageHandler::getImageGeometry().
         //---
         theGeometry = new ossimImageGeometry();

         //---
         // And finally allow factories to extend the internal geometry.
         // This allows plugins for tagged formats with tags not know in the base
         // to extend the internal geometry.
         //
         // Plugins can do handler->getImageGeometry() then modify/extend.
         //---
         if( !ossimImageGeometryRegistry::instance()->extendGeometry( this ) )
         {
            // Check for internal, for geotiff, nitf and so on as last resort for getting
            // some kind of geometry
            // loaded
            theGeometry = getInternalImageGeometry();
         }

         if ( !theGeometry )
         {
            theGeometry = getExternalImageGeometryFromXml();
         }
      }

      // Set image things the geometry object should know about.
      initImageParameters( theGeometry.get() );
   }

  return theGeometry;
}

ossimRefPtr<ossimImageGeometry> ossimGdalTileSource::getExternalImageGeometryFromXml() const
{
  ossimRefPtr<ossimImageGeometry> geom = 0;

  ossimString fileBase = theImageFile.noExtension();
  ossimFilename xmlFile = ossimString(fileBase + ".xml");
  if (!xmlFile.exists())//try the xml file which includes the entire source file name
  {
    xmlFile = theImageFile + ".xml";
  }
  ossimFgdcXmlDoc* fgdcXmlDoc = new ossimFgdcXmlDoc;
  if ( fgdcXmlDoc->open(xmlFile) )
  {
     ossimRefPtr<ossimProjection> proj = fgdcXmlDoc->getProjection();
     if ( proj.valid() )
     {
        geom = new ossimImageGeometry;
        geom->setProjection( proj.get() );
     }
  }
  delete fgdcXmlDoc;
  fgdcXmlDoc = 0;
  
  return geom;
}

//*************************************************************************************************
//! Returns the image geometry object associated with this tile source or NULL if non defined.
//! The geometry contains full-to-local image transform as well as projection (image-to-world)
//*************************************************************************************************
ossimRefPtr<ossimImageGeometry> ossimGdalTileSource::getInternalImageGeometry() const
{
   static const char MODULE[] = "ossimGdalTileSource::getImageGeometry";
   
   if( !isOpen())
   {
      return ossimRefPtr<ossimImageGeometry>();
   }
   
   // GDAL Driver Name
   ossimString driverName = theDriver ? GDALGetDriverShortName( theDriver ) : "";
   
   // Projection Reference
   const char* wkt = GDALGetProjectionRef( theDataset );
   ossim_uint32 gcpCount = GDALGetGCPCount(theDataset);
   if(!ossimString(wkt).empty() && gcpCount < 4)
   {
      if(traceDebug())
      {
         CLOG << "WKT PROJECTION STRING = " << wkt << std::endl;
      }
   }
   else
   {
      // if ESAT driver we will for now try to creae a projection for it
      //
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "GDALGetGCPCount = " << gcpCount << std::endl;
      }
      
      if(gcpCount > 3)
      {
         ossimRefPtr<ossimProjection> projGcp = 0;
         wkt = GDALGetGCPProjection(theDataset);
         if (!ossimString(wkt).empty())
         {
            ossimKeywordlist kwl;
            
            if (wktTranslator.toOssimKwl(wkt, kwl))
            {
               projGcp = ossimProjectionFactoryRegistry::instance()->createProjection(kwl);
            }
         }
         
         bool isGeo = true;
         if (projGcp.valid())
         {
            ossimRefPtr<ossimMapProjection> mapProj = PTR_CAST(ossimMapProjection, projGcp.get());
            if (mapProj.valid())
            {
               if (!mapProj->isGeographic())
               {
                  isGeo = false;
               }
            }
         }
         ossim_uint32 idx = 0;
         const GDAL_GCP* gcpList = GDALGetGCPs(theDataset);
         ossimTieGptSet tieSet;
         if(!gcpList) return ossimRefPtr<ossimImageGeometry>();
         
         for(idx = 0; idx < gcpCount; ++idx)
         {
            //---
            // Modified to add all tie points if gcpCount is < than
            // 40 as dataset with five gcp's was failing because only one
            // tiepoint was set from "if (idx%10 == 0)" logic.
            // (drb - 20080615)
            //---
            if ((gcpCount < 40) || (idx%10 == 0))
            {
               ossimDpt dpt(gcpList[idx].dfGCPPixel, gcpList[idx].dfGCPLine);
               ossimGpt gpt;
               if (isGeo == false)
               {
                  ossimDpt dptEastingNorthing(gcpList[idx].dfGCPX, gcpList[idx].dfGCPY);
                  gpt = projGcp->inverse(dptEastingNorthing);
                  gpt.hgt = gcpList[idx].dfGCPZ;
               }
               else
               {
                  gpt = ossimGpt(gcpList[idx].dfGCPY, gcpList[idx].dfGCPX, gcpList[idx].dfGCPZ);
               }
               
               tieSet.addTiePoint(new ossimTieGpt(gpt, dpt, .5));
               
               if (traceDebug())
               {
                  ossimNotify(ossimNotifyLevel_DEBUG)
                     << "Added tie point for gcp[" << idx << "]: "
                     << dpt << ", " << gpt << std::endl;
               }
            }
         }
         
         ossimRefPtr<ossimBilinearProjection> bilinProj = new ossimBilinearProjection;
         bilinProj->optimizeFit(tieSet);
         
         // Set the projection and return this handler's image geometry:
         ossimRefPtr<ossimImageGeometry> geom = new ossimImageGeometry();
         geom->setProjection(bilinProj.get());
         return geom;
      }
   }
   
   // Geometric Transform
   double geoTransform[6];
   bool transOk = GDALGetGeoTransform( theDataset, geoTransform ) == CE_None;
   if (!transOk)
   {
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << MODULE << "DEBUG: GDALGetGeoTransform failed..." << std::endl;
      }
      return ossimRefPtr<ossimImageGeometry>();
   }
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << "DEBUG: Geo transform = <" 
         << geoTransform[0] << ", " << geoTransform[1] << ", " << geoTransform[2] << ", "
         << geoTransform[3] << ", " << geoTransform[2] << ", " << geoTransform[5] << ">" 
         << std::endl;
   }
  
   // Convert WKT string to OSSIM keywordlist
   ossimKeywordlist kwl;
   if ( !wktTranslator.toOssimKwl(wkt, kwl) )
   {
      // sometimes geographic is just encoded in the matrix transform.  We can do a crude test
      // to see if the tie points
      // are within geographic bounds and then assume geographic so we can still load 
      // projections
      //
      if(transOk&&(driverName=="AAIGrid"))
      {
         if(traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG) << "DEBUG:TESTING GEOGRAPHIC = <" 
               << geoTransform[0] << ", " << geoTransform[1] << ", " << geoTransform[2] << ", "
               << geoTransform[3] << ", " << geoTransform[2] << ", " << geoTransform[5] << ">" 
               << std::endl;
         }
         if((std::fabs(geoTransform[0]) < (180.0+FLT_EPSILON))&&
            (std::fabs(geoTransform[3]) < (90.0 + FLT_EPSILON)))
         {
            ossimTieGptSet tieSet;
            ossimIrect rect = getBoundingRect();
            if(rect.hasNans()) return ossimRefPtr<ossimImageGeometry>();
            // we will do an affine transform for the projeciton information
            //
            ossimDpt ul = rect.ul();
            tieSet.addTiePoint(new ossimTieGpt(ossimGpt(geoTransform[3] + ul.x*geoTransform[4] + ul.y*geoTransform[5],
               geoTransform[0] + ul.x*geoTransform[1] + ul.y*geoTransform[2]), 
               ul, .5));
            ossimDpt ur = rect.ur();
            tieSet.addTiePoint(new ossimTieGpt(ossimGpt(geoTransform[3] + ur.x*geoTransform[4] + ur.y*geoTransform[5],
               geoTransform[0] + ur.x*geoTransform[1] + ur.y*geoTransform[2]), 
               ur, .5));
            ossimDpt lr = rect.lr();
            tieSet.addTiePoint(new ossimTieGpt(ossimGpt(geoTransform[3] + lr.x*geoTransform[4] + lr.y*geoTransform[5],
               geoTransform[0] + lr.x*geoTransform[1] + lr.y*geoTransform[2]), 
               lr, .5));
            ossimDpt ll = rect.ll();
            tieSet.addTiePoint(new ossimTieGpt(ossimGpt(geoTransform[3] + ll.x*geoTransform[4] + ll.y*geoTransform[5],
               geoTransform[0] + ll.x*geoTransform[1] + ll.y*geoTransform[2]), 
               ll, .5));
            ossimRefPtr<ossimBilinearProjection> bilinProj = new ossimBilinearProjection;
            bilinProj->optimizeFit(tieSet);

            // Set the projection and return this handler's image geometry:
            ossimRefPtr<ossimImageGeometry> geom = new ossimImageGeometry();
            geom->setProjection(bilinProj.get());
            return geom;
         }
         else
         {
            return ossimRefPtr<ossimImageGeometry>();
         }
      }
      else
      {
         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << MODULE << "DEBUG:  wktTranslator.toOssimKwl failed..." << std::endl;
         }
         return ossimRefPtr<ossimImageGeometry>();
      }
   }

   // GARRETT! Where is this coming from?
   // Projection, Datum, and Units
   const char* prefix = 0; // legacy code. KWL is used internally now so don't need prefix
   ossimString proj_type  = kwl.find(prefix, ossimKeywordNames::TYPE_KW);
   ossimString datum_type = kwl.find(prefix, ossimKeywordNames::DATUM_KW);
   ossimString units      = kwl.find(prefix, ossimKeywordNames::UNITS_KW);

   ossimDpt gsd(fabs(geoTransform[1]), fabs(geoTransform[5]));

   ossimUnitType savedUnitType = static_cast<ossimUnitType>(ossimUnitTypeLut::instance()->getEntryNumber(units)); 
   ossimUnitType unitType = savedUnitType;
   if(unitType == OSSIM_UNIT_UNKNOWN)
   {
      unitType = OSSIM_METERS;
      units = "meters";
   }
   
   // AIG driver considered pixel is area:
   if(driverName == "AIG")
      thePixelType = OSSIM_PIXEL_IS_AREA;

   ossimDpt tie(geoTransform[0], geoTransform[3]);

   if (thePixelType == OSSIM_PIXEL_IS_AREA)
   {
      tie.x += gsd.x/2.0;
      tie.y -= gsd.y/2.0;
   }

   geoTransform[0] = tie.x;
   geoTransform[3] = tie.y;

   if(proj_type == "ossimLlxyProjection" ||
      proj_type == "ossimEquDistCylProjection")
   {
      if(savedUnitType == OSSIM_UNIT_UNKNOWN)
      {
         unitType = OSSIM_DEGREES;
         units = "degrees";
      }
   }
   kwl.add(prefix, ossimKeywordNames::PIXEL_SCALE_XY_KW,    gsd.toString(), true);
   kwl.add(prefix, ossimKeywordNames::PIXEL_SCALE_UNITS_KW, units,          true);
   kwl.add(prefix, ossimKeywordNames::TIE_POINT_XY_KW,      tie.toString(), true);
   kwl.add(prefix, ossimKeywordNames::TIE_POINT_UNITS_KW,   units,          true);
   kwl.add(prefix, ossimKeywordNames::TIE_POINT_UNITS_KW,   units,          true);
   std::stringstream mString;
   mString << ossimString::toString(geoTransform[1], 20) << " " << ossimString::toString(geoTransform[2], 20)
           << " " << 0 << " " << ossimString::toString(geoTransform[0], 20)
           << " " << ossimString::toString(geoTransform[4], 20) << " " << ossimString::toString(geoTransform[5], 20)
           << " " << 0 << " " << ossimString::toString(geoTransform[3], 20)
           << " " << 0 << " " << 0 << " " << 1 << " " << 0
           << " " << 0 << " " << 0 << " " << 0 << " " << 1;
   
   kwl.add(prefix, ossimKeywordNames::IMAGE_MODEL_TRANSFORM_MATRIX_KW, mString.str().c_str(), true);
   kwl.add(prefix, ossimKeywordNames::IMAGE_MODEL_TRANSFORM_UNIT_KW, units, true);
   
   //---
   // SPECIAL CASE:  ArcGrid in British National Grid
   //---
   if(driverName == "AIG" && datum_type.contains("OGB"))
   {
      ossimFilename prj_file = theImageFile.path() + "/prj.adf";
      
      if(prj_file.exists())
      {
         ossimKeywordlist prj_kwl(' ');
         prj_kwl.addFile(prj_file);
         
         ossimString proj = prj_kwl.find("Projection");
         
         // Reset projection and Datum correctly for BNG.
         if(proj.upcase().contains("GREATBRITAIN"))
         {

            kwl.add(prefix, ossimKeywordNames::TYPE_KW,
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

               kwl.add(prefix, ossimKeywordNames::DATUM_KW, 
                       datum, true);
            }
         }
      }
      else
      {
         if (traceDebug())
         {
            ossimNotify(ossimNotifyLevel_DEBUG)
               << "MODULE DEBUG:"
               << "\nUnable to accurately support ArcGrid due to missing"
               << " prj.adf file." << std::endl;
         }
         return ossimRefPtr<ossimImageGeometry>();
      }
      
   } // End of "if(driverName == "AIG" && datum_type == "OSGB_1936")"
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE <<  " DEBUG:"
         << "\nwktTranslator keyword list:\n"
         << kwl
         << std::endl;
   }

   // Save for next time...
   ossimProjection* proj = ossimProjectionFactoryRegistry::instance()->createProjection(kwl);
   ossimRefPtr<ossimImageGeometry> geom = new ossimImageGeometry();
   geom->setProjection(proj); // assumes ownership via ossiRefPtr mechanism

   return geom;
}

//*******************************************************************
// Public method:
//*******************************************************************
ossim_uint32 ossimGdalTileSource::getTileWidth() const
{
   return ( theTile.valid() ? theTile->getWidth() : 0 );
}

//*******************************************************************
// Public method:
//*******************************************************************
ossim_uint32 ossimGdalTileSource::getTileHeight() const
{
   return ( theTile.valid() ? theTile->getHeight() : 0 );
}

ossimScalarType ossimGdalTileSource::getOutputScalarType() const
{   
   ossimScalarType result = getInputScalarType();

   //---
   // If theLut is valid typical output is eight bit RGB unless the flag is set to preserve
   // palette indexes in which case it is typically 16 bit.
   //---
   if ( theLut.valid() && !m_preservePaletteIndexesFlag )
   {
      // Input different than output.
      result = OSSIM_UINT8;
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalTileSource::getOutputScalarType debug:"
         << "\nOutput scalar: " << result << std::endl;
   }

   return result;
}

ossimScalarType ossimGdalTileSource::getInputScalarType() const
{   
   ossimScalarType result = OSSIM_SCALAR_UNKNOWN;


   switch(theGdtType)
   {
      case GDT_Byte:
      {
         result = OSSIM_UINT8;
         break;
      }
      case GDT_UInt16:
      {
         result = OSSIM_USHORT16;
         break;
      }
      case GDT_Int16:
      {
         result = OSSIM_SSHORT16;
         break;
      }
      case GDT_UInt32:
      {
         result = OSSIM_UINT32;
         break;
         
      }
      case GDT_Int32:
      {
         ossim_int32 sizeType = GDALGetDataTypeSize(theGdtType)/8;
         if(sizeType == 2)
         {
            result = OSSIM_SSHORT16;
            theGdtType = GDT_Int16;
         }
         else
         {
            result = OSSIM_SINT32;
            theGdtType = GDT_Int32;
         }
         break;
      }
      case GDT_Float32:
      {
         result = OSSIM_FLOAT;
         break;
      }
      case GDT_Float64:
      {
         result = OSSIM_DOUBLE;
         break;
      }
      case GDT_CInt16:
      {
         result = OSSIM_SINT16;
         break;
      }
      case  GDT_CInt32:
      {
         result = OSSIM_SINT32;
         break;
      }
      case GDT_CFloat32:
      {
         result = OSSIM_FLOAT32;
         break;
      }
      case GDT_CFloat64:
      {
         result = OSSIM_FLOAT64;
         break;
      }
      default:
         break;
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalTileSource::getInputScalarType debug:"
         << "\nGDAL Type:    " << theGdtType
         << "\nInput scalar: " << result << std::endl;
   }

   return result;
}

double ossimGdalTileSource::getNullPixelValue(ossim_uint32 band)const
{
   double result = ossim::defaultNull(getOutputScalarType());

   if ( theLut.valid() )
   {
      //---
      // If serving up lut data zero may not be null.
      // If m_preservePaletteIndexesFlag is set use the first alpha set to zero in the lut.
      //---
      ossim_int32 np = theLut->getNullPixelIndex();
      if ( np != -1 ) // -1 means the lut does not know a null value.
      {
         result = theLut->getNullPixelIndex();
      }
   }
   else if(theMetaData.getNumberOfBands())
   {
      result = ossimImageHandler::getNullPixelValue(band);
   }
   else if (theNullPixValues && (band < getNumberOfOutputBands()) )
   {
      result = theNullPixValues[band];
   }
   return result;
}

double ossimGdalTileSource::getMinPixelValue(ossim_uint32 band)const
{
   double result = ossim::defaultMin(getOutputScalarType());
   if (  theLut.valid() )
   {
      result = 0;
   }
   else if(theMetaData.getNumberOfBands())
   {
      result = ossimImageHandler::getMinPixelValue(band);
   }
   else if (theMinPixValues && (band < getNumberOfOutputBands()) )
   {
      result = theMinPixValues[band];
   }
   return result;
}

double ossimGdalTileSource::getMaxPixelValue(ossim_uint32 band)const
{
   double result = ossim::defaultMax(getOutputScalarType());
   if ( m_preservePaletteIndexesFlag && theLut.valid() && theLut->getNumberOfEntries() )
   {
      result = theLut->getNumberOfEntries() - 1;
   }
   else if(theMetaData.getNumberOfBands())
   {
      result = ossimImageHandler::getMaxPixelValue(band);
   }
   else if ( theMaxPixValues && (band < getNumberOfOutputBands()) )
   {
      result = theMaxPixValues[band];
   }
   return result;
}

void ossimGdalTileSource::computeMinMax()
{
   ossim_uint32 bands = GDALGetRasterCount(theDataset);

   if(theMinPixValues)
   {
      delete [] theMinPixValues;
      theMinPixValues = 0;
   }
   if(theMaxPixValues)
   {
      delete [] theMaxPixValues;
      theMaxPixValues = 0;
   }
   if(theNullPixValues)
   {
      delete [] theNullPixValues;
      theNullPixValues = 0;
   }
   if(isIndexTo3Band())
   {
      int i = 0;
      theMinPixValues  = new double[3];
      theMaxPixValues  = new double[3];
      theNullPixValues = new double[3];

      for(i = 0; i < 3; ++i)
      {
         theMinPixValues[i] = 1;
         theMaxPixValues[i] = 255;
         theNullPixValues[i] = 0;
      }
   }
   else if(isIndexTo1Band())
   {
      theMinPixValues  = new double[1];
      theMaxPixValues  = new double[1];
      theNullPixValues = new double[1];
      
      *theNullPixValues = 0;
      *theMaxPixValues = 255;
      *theMinPixValues = 1;
   }
   else
   {
      if(!theMinPixValues && !theMaxPixValues&&bands)
      {
         theMinPixValues = new double[bands];
         theMaxPixValues = new double[bands];
         theNullPixValues = new double[bands];
      }
      for(ossim_int32 band = 0; band < (ossim_int32)bands; ++band)
      {
         GDALRasterBandH aBand=0;
         
         aBand = GDALGetRasterBand(theDataset, band+1);
         
         int minOk=1;
         int maxOk=1;
         int nullOk=1;
         
         if(aBand)
         {
	   if(hasMetaData())
           {
              theMinPixValues[band] = theMetaData.getMinPix(band);
              theMaxPixValues[band] = theMetaData.getMaxPix(band);
              theNullPixValues[band] = theMetaData.getNullPix(band);
           }
	   else 
           {
              ossimString driverName = theDriver ? GDALGetDriverShortName( theDriver ) : "";

              // ESH 12/2008 -- Allow OSSIM to rescale the image data 
              // to the min/max values found in the raster data only 
              // if it was not acquired with the following drivers:
              if ( driverName.contains("JP2KAK")   ||
                   driverName.contains("JPEG2000") ||
                   driverName.contains("NITF") )
              {
                 theMinPixValues[band] = ossim::defaultMin(getOutputScalarType());
                 theMaxPixValues[band] = ossim::defaultMax(getOutputScalarType());
                 theNullPixValues[band] = ossim::defaultNull(getOutputScalarType());
              }
              else
              {
                 theMinPixValues[band]  = GDALGetRasterMinimum(aBand, &minOk);
                 theMaxPixValues[band]  = GDALGetRasterMaximum(aBand, &maxOk);
                 theNullPixValues[band] = GDALGetRasterNoDataValue(aBand, &nullOk);
              }
              
              if(!nullOk)
              {
                 theNullPixValues[band] = ossim::defaultNull(getOutputScalarType());
              }
           }

           if(!minOk||!maxOk)
           {
               theMinPixValues[band] = ossim::defaultMin(getOutputScalarType());
               theMaxPixValues[band] = ossim::defaultMax(getOutputScalarType());
            }
         }
         else
         {
            theMinPixValues[band] = ossim::defaultMin(getOutputScalarType());
            theMaxPixValues[band] = ossim::defaultMax(getOutputScalarType());
         }
      }
   }
}

ossim_uint32 ossimGdalTileSource::getNumberOfDecimationLevels() const
{
   ossim_uint32 result = 1;
   //---
   // Note: This code was shut off since we currently don't utilize gdal overviews.  Returning
   // this causes the overview builder will incorrectly start at the wrong res level.
   // drb - 20110503
   //---
   if(theDataset&&!theOverview)
   {
      if(GDALGetRasterCount(theDataset))
      {
         GDALRasterBandH band = GDALGetRasterBand(theDataset, 1);
         if(GDALGetOverviewCount(band))
         {
            result += (GDALGetOverviewCount(band));
         }
      }
   }
   if (theOverview.valid())
   {
      result += theOverview->getNumberOfDecimationLevels();
   }
   return result;
}

void ossimGdalTileSource::loadIndexTo3BandTile(const ossimIrect& clipRect,
                                               ossim_uint32 aGdalBandStart,
                                               ossim_uint32 anOssimBandStart)
{
   // Typical case 16 bit indexes, eight bit out, NOT 16 bit out.
   ossimScalarType inScalar  = getInputScalarType();
   ossimScalarType outScalar = getOutputScalarType();

   if ( ( inScalar == OSSIM_UINT8 ) && ( outScalar == OSSIM_UINT8 ) )
   {
      loadIndexTo3BandTileTemplate(ossim_uint8(0), // input type
                                   ossim_uint8(0), // output type
                                   clipRect,
                                   aGdalBandStart,
                                   anOssimBandStart);
   }
   else if ( ( inScalar == OSSIM_UINT16 ) && ( outScalar == OSSIM_UINT8 ) )
   {
      loadIndexTo3BandTileTemplate(ossim_uint16(0), // input type
                                   ossim_uint8(0),  // output type
                                   clipRect,
                                   aGdalBandStart,
                                   anOssimBandStart);
   }

   else if ( ( inScalar == OSSIM_UINT16 ) && ( outScalar == OSSIM_UINT16 ) )
   {
      loadIndexTo3BandTileTemplate(ossim_uint16(0), // input type
                                   ossim_uint16(0),  // output type
                                   clipRect,
                                   aGdalBandStart,
                                   anOssimBandStart);
   }
   else if ( ( inScalar == OSSIM_SINT16 ) && ( outScalar == OSSIM_SINT16 ) )
   {
      loadIndexTo3BandTileTemplate(ossim_sint16(0), // input type
                                   ossim_sint16(0),  // output type
                                   clipRect,
                                   aGdalBandStart,
                                   anOssimBandStart);
   }
   else if ( ( inScalar == OSSIM_FLOAT32 ) && ( outScalar == OSSIM_FLOAT32 ) )
   {
      loadIndexTo3BandTileTemplate(ossim_float32(0.0), // input type
                                   ossim_float32(0.0),  // output type
                                   clipRect,
                                   aGdalBandStart,
                                   anOssimBandStart);
   }
   else if ( ( inScalar == OSSIM_FLOAT64 ) && ( outScalar == OSSIM_FLOAT64 ) )
   {
      loadIndexTo3BandTileTemplate(ossim_float64(0.0), // input type
                                   ossim_float64(0.0),  // output type
                                   clipRect,
                                   aGdalBandStart,
                                   anOssimBandStart);
   }
   else if ( ( inScalar == OSSIM_FLOAT64 ) && ( outScalar == OSSIM_UINT8 ) )
   {
      loadIndexTo3BandTileTemplate(ossim_float64(0.0), // input type
                                   ossim_uint8(0.0),  // output type
                                   clipRect,
                                   aGdalBandStart,
                                   anOssimBandStart);
   }
   else
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << __FILE__ << __LINE__
         << " ossimGdalTileSource::loadIndexTo3BandTile WARNING!\n"
         << "Unsupported scalar types:\nInupt scalar: " << inScalar
         << "\nOutput scalar: " << outScalar
         << std::endl;
   }
}

template<class InputType, class OutputType>
void ossimGdalTileSource::loadIndexTo3BandTileTemplate(InputType /* in */,
                                                       OutputType /* out */,
                                                       const ossimIrect& clipRect,
                                                       ossim_uint32 aGdalBandStart,
                                                       ossim_uint32 anOssimBandStart)
{
   const InputType* s = reinterpret_cast<const InputType*>(theSingleBandTile->getBuf());
   GDALRasterBandH aBand=0;
   aBand = GDALGetRasterBand(theDataset, aGdalBandStart);
   GDALColorTableH table = GDALGetRasterColorTable(aBand);
   
   // ossim_uint32 rasterCount = GDALGetRasterCount(theDataset); 
   ossim_uint32 anOssimBandEnd = anOssimBandStart + 2; 
   if(getNumberOfOutputBands() < anOssimBandEnd)
   {
      return;
   }
   // Get the width of the buffers.
   ossim_uint32 s_width = theSingleBandTile->getWidth();
   ossim_uint32 d_width = theTile->getWidth();
   ossimIrect src_rect  = theSingleBandTile->getImageRectangle();
   ossimIrect img_rect  = theTile->getImageRectangle();
   
   // Move the pointers to the first valid pixel.
   s += (clipRect.ul().y - src_rect.ul().y) * s_width +
   clipRect.ul().x - src_rect.ul().x;
   ossim_uint32 clipHeight = clipRect.height();
   ossim_uint32 clipWidth  = clipRect.width();

   OutputType* d[3];
   d[0]= static_cast<OutputType*>(theTile->getBuf(anOssimBandStart));
   d[1]= static_cast<OutputType*>(theTile->getBuf(anOssimBandStart + 1));
   d[2]= static_cast<OutputType*>(theTile->getBuf(anOssimBandStart + 2));

#if 0 /* Code shut off to treat all indexes as valid. */   
   OutputType np[3];
   np[0] = (OutputType)theTile->getNullPix(0);
   np[1] = (OutputType)theTile->getNullPix(1);
   np[2] = (OutputType)theTile->getNullPix(2);
   
   OutputType minp[3];
   minp[0] = (OutputType)theTile->getMinPix(0);
   minp[1] = (OutputType)theTile->getMinPix(1);
   minp[2] = (OutputType)theTile->getMinPix(2);
#endif
   
   ossim_uint32 offset = (clipRect.ul().y - img_rect.ul().y) * d_width +
   clipRect.ul().x  - img_rect.ul().x;
   d[0] += offset;
   d[1] += offset;
   d[2] += offset;
   
   // Copy the data.

   GDALPaletteInterp interp = GDALGetPaletteInterpretation(table);

   for (ossim_uint32 line = 0; line < clipHeight; ++line)
   {
      for (ossim_uint32 sample = 0; sample < clipWidth; ++sample)
      {
         GDALColorEntry entry;
         if(GDALGetColorEntryAsRGB(table, s[sample], &entry))
         {
            if(interp == GPI_RGB)
            {
               if(!entry.c4)
               {
                  d[0][sample] = 0;
                  d[1][sample] = 0;
                  d[2][sample] = 0;
               }
               else
               {
#if 0 /* Code shut off to treat all indexes as valid. */
                  d[0][sample] = entry.c1==np[0]?minp[0]:entry.c1;
                  d[1][sample] = entry.c2==np[1]?minp[1]:entry.c2;
                  d[2][sample] = entry.c3==np[2]?minp[2]:entry.c3;
#endif
                  d[0][sample] = entry.c1;
                  d[1][sample] = entry.c2;
                  d[2][sample] = entry.c3;
                  
               }
            }
            else
            {
               d[0][sample] = entry.c1;
               d[1][sample] = entry.c2;
               d[2][sample] = entry.c3;
            }
         }
         else
         {
            d[0][sample] = 0;
            d[1][sample] = 0;
            d[2][sample] = 0;
         }
      }
      
      s += s_width;
      d[0] += d_width;
      d[1] += d_width;
      d[2] += d_width;
   }
   
}

bool ossimGdalTileSource::isIndexTo3Band(int bandNumber)const
{
   GDALRasterBandH band = GDALGetRasterBand(theDataset, bandNumber);
   if(GDALGetRasterColorInterpretation(band)==GCI_PaletteIndex)
   {
      GDALColorTableH table = GDALGetRasterColorTable(band);
      GDALPaletteInterp interp = GDALGetPaletteInterpretation(table);
      if( (interp == GPI_RGB) ||
          (interp == GPI_HLS)||
          (interp == GPI_CMYK))
      {
         return true;
      }
   }

   return false;
}

bool ossimGdalTileSource::isIndexTo1Band(int bandNumber)const
{
   GDALRasterBandH band = GDALGetRasterBand(theDataset, bandNumber);
   if(GDALGetRasterColorInterpretation(band)==GCI_PaletteIndex)
   {
      GDALColorTableH table = GDALGetRasterColorTable(band);
      GDALPaletteInterp interp = GDALGetPaletteInterpretation(table);
      if(interp == GPI_Gray)
      {
         return true;
      }
   }

   return false;
}

ossim_uint32 ossimGdalTileSource::getIndexBandOutputNumber(int bandNumber)const
{
   if(isIndexed(bandNumber))
   {
      GDALRasterBandH band = GDALGetRasterBand(theDataset, bandNumber);
      if(GDALGetRasterColorInterpretation(band)==GCI_PaletteIndex)
      {
         GDALColorTableH table = GDALGetRasterColorTable(band);
         GDALPaletteInterp interp = GDALGetPaletteInterpretation(table);
         switch(interp)
         {
            case GPI_CMYK:
            case GPI_RGB:
            case GPI_HLS:
            {
               return 3;
            }
            case GPI_Gray:
            {
               return 1;
            }
         }
      }
   }

   return 0;
}

bool ossimGdalTileSource::isIndexed(int aGdalBandNumber)const
{
   if(aGdalBandNumber <= GDALGetRasterCount(theDataset))
   {
      GDALRasterBandH band = GDALGetRasterBand(theDataset, aGdalBandNumber);
      if(!band) return false;
      if(GDALGetRasterColorInterpretation(band)==GCI_PaletteIndex)
      {
         return true;
      }
   }
   
   return false;
}

// Temp "return 128x128;" until I can figure out how to tell if tiled (gcp)
ossim_uint32 ossimGdalTileSource::getImageTileWidth() const
{
   return 128;
}

ossim_uint32 ossimGdalTileSource::getImageTileHeight() const
{
   return 128;
}

void ossimGdalTileSource::getMaxSize(ossim_uint32 resLevel,
                                     int& maxX,
                                     int& maxY)const
{
   int aGdalBandIndex = 0;
   maxX = 0;
   maxY = 0;
   
   if(theOverview.valid() && theOverview->isValidRLevel(resLevel))
   {
      ossimIrect rect = theOverview->getBoundingRect(resLevel);
      if(!rect.hasNans())
      {
         maxX = rect.width();
         maxY = rect.height();
      }
      return;
   }
   
   for(aGdalBandIndex=1;
       (int)aGdalBandIndex <= (int)GDALGetRasterCount(theDataset);
       ++aGdalBandIndex)
   {
      GDALRasterBandH aBand = resolveRasterBand(resLevel, aGdalBandIndex);
      if(aBand)
      {
         maxY = ossim::max<int>((int)GDALGetRasterBandYSize(aBand), maxY);
         maxX = ossim::max<int>((int)GDALGetRasterBandXSize(aBand), maxX);
      }
      else
      {
         break;
      }
   }
}

GDALRasterBandH ossimGdalTileSource::resolveRasterBand( ossim_uint32 resLevel,
                                                        int aGdalBandIndex ) const
{
   GDALRasterBandH aBand = GDALGetRasterBand( theDataset, aGdalBandIndex );

   if( resLevel > 0 )
   {
      int overviewIndex = resLevel-1;

      GDALRasterBandH overviewBand = GDALGetOverview( aBand, overviewIndex );
      if ( overviewBand )
      {
         aBand = overviewBand;
      }
   }

   return aBand;
}

ossimString ossimGdalTileSource::getShortName()const
{
   ossimString result = "gdal";
   if ( theDriver )
   {
      const char* driver = GDALGetDriverShortName(theDriver);
      if ( driver )
      {
         result += "_";
         result += driver;
      }
   }
   return result;
}

ossimString ossimGdalTileSource::getLongName()const
{
   return ossimString("gdal reader");
}

ossimString ossimGdalTileSource::getClassName()const
{
   return ossimString("ossimGdalTileSource");
}

bool ossimGdalTileSource::isOpen()const
{
   return (theDataset != 0);
}

ossim_uint32 ossimGdalTileSource::getCurrentEntry()const
{
   return theEntryNumberToRender;
}

bool ossimGdalTileSource::setCurrentEntry(ossim_uint32 entryIdx)
{
   if ( isOpen() && (theEntryNumberToRender == entryIdx) )
   {
      return true; // Nothing to do...
   }
   theDecimationFactors.clear();
   theGeometry = 0;
   theOverview = 0;
   theOverviewFile.clear();
   m_outputBandList.clear();
   theEntryNumberToRender = entryIdx;
   return open();
}

void ossimGdalTileSource::getEntryList(std::vector<ossim_uint32>& entryList) const
{
   entryList.clear();
   if (theSubDatasets.size())
   {
      for (ossim_uint32 i = 0; i < theSubDatasets.size(); ++i)
      {
         entryList.push_back(i);
      }
   }
   else
   {
      entryList.push_back(0);
   }
}

void ossimGdalTileSource:: getEntryNames(std::vector<ossimString>& entryStringList) const
{
   if (theSubDatasets.size())
   {
      entryStringList = theSubDatasets;
   }
   else
   {
      ossimImageHandler::getEntryNames(entryStringList);
   }
}

ossimString ossimGdalTileSource::filterSubDatasetsString(const ossimString& subString) const
{
   //---
   // Skip up to and including '=' and filter out the quotes '"'.
   //---
   ossimString s;
   bool atStart = false;
   for (ossim_uint32 pos = 0; pos < subString.size(); ++pos)
   {
      if ( *(subString.begin()+pos) == '=')
      {
         atStart = true; // Start recording after this.
         continue;       // Skip the '='.
      }
      if (atStart)
      {
         //if (*(subString.begin()+pos) == '\"')
         //{
          //  continue;  // Skip the '='.
         //}
         s.push_back(*(subString.begin()+pos)); // Record the character.
      }
   }
   return s;
}

bool ossimGdalTileSource::isBlocked( int /* band */ )const
{
   return m_isBlocked;
}

void ossimGdalTileSource::populateLut()
{
   theLut = 0; // ossimRefPtr not a leak.
   
   if(isIndexed(1)&&theDataset)
   {
      GDALColorTableH aTable = GDALGetRasterColorTable(GDALGetRasterBand( theDataset, 1 ));
      GDALPaletteInterp interp = GDALGetPaletteInterpretation(aTable);
      if(aTable && ( (interp == GPI_Gray) || (interp == GPI_RGB)))
      {
         GDALColorEntry colorEntry;
         ossim_uint32 numberOfElements = GDALGetColorEntryCount(aTable);
         ossim_uint32 idx = 0;
         if(numberOfElements)
         {
            // GPI_Gray Grayscale (in GDALColorEntry.c1)
            // GPI_RGB Red, Green, Blue and Alpha in (in c1, c2, c3 and c4)
            theLut = new ossimNBandLutDataObject(numberOfElements,
                                                 4,
                                                 OSSIM_UINT8,
                                                 -1);

            bool nullSet = false;
            for(idx = 0; idx < numberOfElements; ++idx)
            {
               switch(interp)
               {
                  case GPI_RGB:
                  {
                     if(GDALGetColorEntryAsRGB(aTable, idx, &colorEntry))
                     {
                        (*theLut)[idx][0] = colorEntry.c1;
                        (*theLut)[idx][1] = colorEntry.c2;
                        (*theLut)[idx][2] = colorEntry.c3;
                        (*theLut)[idx][3] = colorEntry.c4;

                        if ( !nullSet )
                        {
                           if ( m_preservePaletteIndexesFlag )
                           {
                              // If preserving palette set the null to the fix alpha of 0.
                              if ( (*theLut)[idx][3] == 0 )
                              {
                                 theLut->setNullPixelIndex(idx);
                                 nullSet = true;
                              }
                           }
                           else
                           {
                              //---
                              // Not using alpha.
                              // Since the alpha is currently not used, look for the null
                              // pixel index and set if we find. If at some point the alpha
                              // is taken out this can be removed.
                              //---
                              if ( ( (*theLut)[idx][0] == 0 ) &&
                                   ( (*theLut)[idx][1] == 0 ) &&
                                   ( (*theLut)[idx][2] == 0 ) )
                              {
                                 theLut->setNullPixelIndex(idx);
                                 nullSet = true;
                              }
                           }
                        }
                     }
                     else
                     {
                        (*theLut)[idx][0] = 0;
                        (*theLut)[idx][1] = 0;
                        (*theLut)[idx][2] = 0;
                        (*theLut)[idx][3] = 0;

                        // Look for the null pixel index and set if we find.
                        if ( !nullSet )
                        {
                           if ( (*theLut)[idx][0] == 0 )
                           {
                              theLut->setNullPixelIndex(idx);
                           }
                        }
                     }
                     break;
                  }
                  case GPI_Gray:
                  {
                     const GDALColorEntry* constEntry =  GDALGetColorEntry(aTable, idx);
                     if(constEntry)
                     {
                        (*theLut)[idx][0] = constEntry->c1;
                     }
                     else
                     {
                        (*theLut)[idx][0] = 0;
                     }
                     break;
                  }
                  default:
                  {
                     break;
                  }
               }
            }
         }
      }

      ossim_uint32 rasterCount = GDALGetRasterCount(theDataset);
      for(ossim_uint32 aGdalBandIndex=1; aGdalBandIndex <= rasterCount; ++aGdalBandIndex)
      {
         GDALRasterBandH aBand = GDALGetRasterBand( theDataset, aGdalBandIndex );
         if (aBand)
         {
            GDALRasterAttributeTableH hRAT = GDALGetDefaultRAT(aBand);
            int colCount = GDALRATGetColumnCount(hRAT);
            for (ossim_int32 col = 0; col < colCount; col++)
            {
               const char* colName = GDALRATGetNameOfCol(hRAT, col);
               if (colName)
               {
                  if (strcmp(colName, "Class_Names") == 0)
                  {
                     std::vector<ossimString> entryLabels;
                     ossim_int32 rowCount = GDALRATGetRowCount(hRAT);
                     for (ossim_int32 row = 0; row < rowCount; row++)
                     {
                        const char* className = GDALRATGetValueAsString(hRAT, row, col);
                        ossimString entryLabel(className);
                        entryLabels.push_back(entryLabel);
                     }
                     theLut->setEntryLables(aGdalBandIndex-1, entryLabels);
                  }
               }
            }
         }
      }
   }
}

void ossimGdalTileSource::setProperty(ossimRefPtr<ossimProperty> property)
{
   if ( property->getName() == PRESERVE_PALETTE_KW )
   {
      ossimString s;
      property->valueToString(s);

      // Go through set method as it will adjust theTile bands if need be.
      setPreservePaletteIndexesFlag(s.toBool());
   }
   else
   {
      ossimImageHandler::setProperty(property);
   }
}

ossimRefPtr<ossimProperty> ossimGdalTileSource::getProperty(const ossimString& name)const
{
   ossimRefPtr<ossimProperty> result = 0;
   
   if( (name == DRIVER_SHORT_NAME_KW) && isOpen() )
   {
      ossimString driverName = GDALGetDriverShortName( theDriver );
      result = new ossimStringProperty(name, driverName);
   }
   else if ( (name == "imag") && isOpen() )
   {
      if (theDataset)
      {
         ossimString nitfImagTag( GDALGetMetadataItem( theDataset, "NITF_IMAG", "" ) );
         if (!nitfImagTag.empty())
         {
            result = new ossimStringProperty(name, nitfImagTag);
         }
      }
   }
   else if ( name == PRESERVE_PALETTE_KW )
   {
      result = new ossimBooleanProperty(name, m_preservePaletteIndexesFlag);
   }
   else
   {
     result = ossimImageHandler::getProperty(name);
   }
   return result;
}

void ossimGdalTileSource::getPropertyNames(std::vector<ossimString>& propertyNames)const
{
   propertyNames.push_back(DRIVER_SHORT_NAME_KW);
   propertyNames.push_back(PRESERVE_PALETTE_KW);
   ossimImageHandler::getPropertyNames(propertyNames);
}

bool ossimGdalTileSource::setOutputBandList(const vector<ossim_uint32>& outputBandList)
{
   m_outputBandList.clear();
   if (outputBandList.size())
   {
      ossim_uint32 bandCount = GDALGetRasterCount(theDataset);
      for (ossim_uint32 i = 0; i < outputBandList.size(); i++)
      {
         if (outputBandList[i] > bandCount)//check if it is a valid band
         {
            return false;
         }
      }
      m_outputBandList = outputBandList;  // Assign the new list.
      return true;
   }
   return false;
}

void ossimGdalTileSource::setPreservePaletteIndexesFlag(bool flag)
{
   bool stateChanged = (flag && !m_preservePaletteIndexesFlag);

   //---
   // Affects: ossimGdalTileSource::getNumberOfOutputBands
   //          ossimGdalTileSource::getOutputScalarType
   //---
   m_preservePaletteIndexesFlag = flag; 
   
   if ( isOpen() && stateChanged )
   {
      //---
      // Already went through open so we must change the output tile from 3 band to 1 and the
      // output scalar type may have also changed.
      // Note the ossimImageDataFactory::create will call back to us for bands and scalr type
      // which have now changed.
      //---
      theTile = ossimImageDataFactory::instance()->create(this, this);
      theTile->setIndexedFlag(true);
      theTile->initialize();

      theSingleBandTile = ossimImageDataFactory::instance()->create(this, this);
      theSingleBandTile->setIndexedFlag(true);
      theSingleBandTile->initialize();

      if ( m_preservePaletteIndexesFlag && theLut.valid() )
      {
         ossim_int32 nullIndex = theLut->getFirstNullAlphaIndex();
         if ( nullIndex > -1 ) // Returns -1 if not found.
         {
            // This will be used for fill in leiu of theTile->makeBlank().
            theLut->setNullPixelIndex(nullIndex);
         }
      }
   }
}

bool ossimGdalTileSource::getPreservePaletteIndexesFlag() const
{
   return m_preservePaletteIndexesFlag;
}

//---
// Overrides ossimImageSource::isIndexedData to palette indicate we are passing palette indexes
// not rgb pixel data down the stream.
//---
bool ossimGdalTileSource::isIndexedData() const
{
   return m_preservePaletteIndexesFlag;
}

void ossimGdalTileSource::getDefaults()
{
   // Look for the preserve_palette flag:
   const char* lookup = ossimPreferences::instance()->findPreference(PRESERVE_PALETTE_KW);
   if (lookup)
   {
      setPreservePaletteIndexesFlag(ossimString(lookup).toBool());
   }
}
void ossimGdalTileSource::deleteRlevelCache()
{
   //ossimAppFixedTileCache::instance()->deleteCache(m_blockCacheId);
   ossim_uint32 idx = 0;
   for(idx = 0; idx < m_rlevelBlockCache.size();++idx)
   {
      ossimAppFixedTileCache::instance()->deleteCache(m_rlevelBlockCache[idx]);
   }
   m_rlevelBlockCache.clear();
}

void ossimGdalTileSource::setRlevelCache()
{
   if(m_isBlocked)
   {
      if(m_rlevelBlockCache.size() > 0) deleteRlevelCache();
      ossim_uint32 nLevels = getNumberOfDecimationLevels();
      ossim_uint32 idx = 0;
      int xSize=0, ySize=0;
      m_rlevelBlockCache.resize(nLevels);
      for(idx =0; idx < nLevels; ++idx)
      {
         GDALGetBlockSize(resolveRasterBand( idx, 1 ),
                          &xSize,
                          &ySize);
         ossimIpt blockSize(xSize, ySize);
         ossimIrect rectBounds = getBoundingRect(idx);
         rectBounds.stretchToTileBoundary(blockSize);
         m_rlevelBlockCache[idx] = ossimAppFixedTileCache::instance()->newTileCache(rectBounds, blockSize); 
      }
   }
}
