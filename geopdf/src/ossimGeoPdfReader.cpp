//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Mingjie Su
//
// Description:  Class definition for GeoPdf reader.
//
//----------------------------------------------------------------------------
// $Id: ossimGeoPdfReader.cpp 22844 2014-07-26 19:41:01Z dburken $

#include <fstream>
#include <iostream>
#include <string>

//PoDoFo includes
#include <podofo/doc/PdfMemDocument.h>
#include <podofo/base/PdfName.h>
#include <podofo/base/PdfObject.h>

//ossim includes
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimGrect.h>
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
#include <ossim/projection/ossimAlbersProjection.h>
#include <ossim/projection/ossimAzimEquDistProjection.h>
#include <ossim/projection/ossimBonneProjection.h>
#include <ossim/projection/ossimCassiniProjection.h>
#include <ossim/projection/ossimCylEquAreaProjection.h>
#include <ossim/projection/ossimEckert4Projection.h>
#include <ossim/projection/ossimEckert6Projection.h>
#include <ossim/projection/ossimEquDistCylProjection.h>
#include <ossim/projection/ossimGnomonicProjection.h>
#include <ossim/projection/ossimLambertConformalConicProjection.h>
#include <ossim/projection/ossimMercatorProjection.h>
#include <ossim/projection/ossimMillerProjection.h>
#include <ossim/projection/ossimMollweidProjection.h>
#include <ossim/projection/ossimNewZealandMapGridProjection.h>
#include <ossim/projection/ossimObliqueMercatorProjection.h>
#include <ossim/projection/ossimPolarStereoProjection.h>
#include <ossim/projection/ossimPolyconicProjection.h>
#include <ossim/projection/ossimSinusoidalProjection.h>
#include <ossim/projection/ossimStereographicProjection.h>
#include <ossim/projection/ossimTransMercatorProjection.h>
#include <ossim/projection/ossimTransCylEquAreaProjection.h>
#include <ossim/projection/ossimUtmProjection.h>
#include <ossim/projection/ossimVanDerGrintenProjection.h>
#include <ossim/projection/ossimUpsProjection.h>
#include <ossim/support_data/ossimAuxFileHandler.h>

#include "ossimGeoPdfReader.h"

#ifdef OSSIM_ID_ENABLED
static const char OSSIM_ID[] = "$Id";
#endif
 
static ossimTrace traceDebug("ossimGeoPdfReader:debug");
static ossimTrace traceDump("ossimGeoPdfReader:dump");
//static ossimOgcWktTranslator wktTranslator;

RTTI_DEF1_INST(ossimGeoPdfReader,
               "ossimGeoPdfReader",
               ossimImageHandler);

static ossimString originLatKey = "OriginLatitude";
static ossimString centralMeridianKey = "CentralMeridian";
static ossimString standardParallelOneKey = "StandardParallelOne";
static ossimString standardParallelTwoKey = "StandardParallelTwo";
static ossimString falseEastingKey = "FalseEasting";
static ossimString falseNorthingKey = "FalseNorthing";
static ossimString scaleFactorKey = "ScaleFactor";
static ossimString latitudeOneKey = "LatitudeOne";
static ossimString longitudeOneKey = "LongitudeOne";
static ossimString latitudeTwoKey = "LatitudeTwo";
static ossimString longitudeTwoKey = "LongitudeTwo";
static ossimString zoneKey = "Zone";
static ossimString hemisphereKey = "Hemisphere";

class ossimGeoPdfVectorPageNode
{
public:
   ossimGeoPdfVectorPageNode()
      : m_boundingRect(),
        m_geoImage(0)
      {};
   
   ~ossimGeoPdfVectorPageNode()
      {
         if (m_geoImage.valid())
         {
            m_geoImage = 0;
         }
      }
   
   void setGeoImage(ossimRefPtr<ossimImageGeometry> image)
      {
         m_geoImage = image;
      }
   
   void setBoundingBox(const ossimDrect& bounds)
      {
         m_boundingRect = bounds;
      }
   
   ossimDrect m_boundingRect;  //world
   ossimRefPtr<ossimImageGeometry> m_geoImage;
};

ossimString getValueFromMap(std::map<ossimString, ossimString, ossimStringLtstr> info,
                            ossimString key)
{
   ossimString valueStr;
   std::map<ossimString, ossimString, ossimStringLtstr>::iterator it = info.find(key);
   if (it != info.end())
   {
      valueStr = it->second;
   }
   return valueStr;
}
 
ossimGeoPdfReader::ossimGeoPdfReader()
   : ossimImageHandler(),
     m_imageRect(OSSIM_INT_NAN, OSSIM_INT_NAN, OSSIM_INT_NAN, OSSIM_INT_NAN),
     m_numberOfBands(0),
     m_numberOfLines(0),
     m_numberOfSamples(0),
     m_numOfFramesVertical(0),
     m_numOfFramesHorizontal(0),
     m_currentRow(-1),
     m_currentCol(-1),
     m_frameWidthVector(),
     m_frameHeightVector(),
     m_scalarType(OSSIM_UINT8),
     m_tile(0),
     m_pdfMemDocument(NULL),
     m_isLGIDict(false),
     m_isJpeg(true),
     m_pageVector(0),
     m_cacheTile(0),
     m_cacheSize(0),
     m_cacheId(-1),
     m_frameEntryArray(0)
{
}

ossimGeoPdfReader::~ossimGeoPdfReader()
{
   closeEntry();
}

void ossimGeoPdfReader::resetCacheBuffer(ossimFrameEntryData entry)
{
   ossim_int32 row = entry.theRow;
   ossim_int32 col = entry.theCol;
   ossim_int32 width = m_frameWidthVector[col];
   ossim_int32 height = m_frameHeightVector[row];
   ossim_int32 pixelRow = entry.thePixelRow;
   ossim_int32 pixelCol = entry.thePixelCol;

   ossimIrect imageRect(pixelCol, pixelRow, pixelCol+width-1, pixelRow+height-1);

   ossimIrect cacheRect(pixelCol, pixelRow, pixelCol+m_cacheSize.x-1, pixelRow+m_cacheSize.y-1);

   ossimIrect clipRect = cacheRect.clipToRect(imageRect);

   m_cacheTile->setImageRectangle(clipRect);

   if (!cacheRect.completely_within(imageRect))
   {
      m_cacheTile->makeBlank();
   }

   m_cacheTile->validate();

   void* lineBuffer = 0;
   std::vector<PoDoFo::PdfObject*> imageObjects = entry.theFrameEntry;
   if (imageObjects.size() == 1)
   {
      PoDoFo::PdfMemStream* pStream = dynamic_cast<PoDoFo::PdfMemStream*>(imageObjects[0]->GetStream());
      pStream->Uncompress();
      PoDoFo::pdf_long length = pStream->GetLength(); 
      pStream->GetCopy((char**)&lineBuffer, &length);
      if (lineBuffer)
      {
         m_cacheTile->loadTile(lineBuffer, imageRect, clipRect, OSSIM_BIP);
         delete [] (char*)lineBuffer;
         lineBuffer = 0;
      }
   }
   else
   {
      for (ossim_uint32 band = 0; band < m_numberOfBands; ++band)
      {
         PoDoFo::PdfMemStream* pStream = dynamic_cast<PoDoFo::PdfMemStream*>(imageObjects[band]->GetStream());
         pStream->Uncompress();
         PoDoFo::pdf_long length = pStream->GetLength(); 
         pStream->GetCopy((char**)&lineBuffer, &length);

         if (lineBuffer)
         {
            m_cacheTile->loadBand(lineBuffer, imageRect,clipRect, band);
            delete [] (char*)lineBuffer;
            lineBuffer = 0;
         }
      }
   }
   if (lineBuffer != NULL)
   {
      delete [] (char*)lineBuffer;
      lineBuffer = 0;
   }
}

ossimString ossimGeoPdfReader::getShortName()const
{
   return ossimString("ossim_geopdf_reader");
}

ossimString ossimGeoPdfReader::getLongName()const
{
   return ossimString("ossim geopdf reader");
}

ossimString ossimGeoPdfReader::getClassName()const
{
   return ossimString("ossimGeoPdfReader");
}

ossim_uint32 ossimGeoPdfReader::getNumberOfLines(
   ossim_uint32 resLevel) const
{
   if (resLevel == 0)
   {
      return m_numberOfLines;
   }
   else if ( theOverview.valid() )
   {
      return theOverview->getNumberOfLines(resLevel);
   }

   return 0;
}

ossim_uint32 ossimGeoPdfReader::getNumberOfSamples(
   ossim_uint32 resLevel) const
{
   if (resLevel == 0)
   {
      return m_numberOfSamples;
   }
   else if ( theOverview.valid() )
   {
      return theOverview->getNumberOfSamples(resLevel);
   }

   return 0;
}

bool ossimGeoPdfReader::open()
{
   static const char MODULE[] = "ossimGeoPdfReader::open";
   
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " entered...\n"
         << "image: " << theImageFile << "\n";
   }
   
   bool result = false;
   closeEntry();
   
   if ( theImageFile.size() )
   {
      PoDoFo::PdfError::EnableDebug(false);//do not print out debug info
      
      try
      {
         m_pdfMemDocument = new PoDoFo::PdfMemDocument(theImageFile.c_str());
      }
      catch (...)
      {
         return false;
      }
   
      if (m_pdfMemDocument != NULL)
      {
         int count = m_pdfMemDocument->GetPageCount();
         for(int i = 0; i < count; ++i)
         {
            PoDoFo::PdfPage* pdfPage = m_pdfMemDocument->GetPage(i);
            if( pdfPage ) 
            {
               PoDoFo::PdfObject* pdfObject = pdfPage->GetObject();
               PoDoFo::PdfObject* geoPdfObject = NULL;
               if( pdfObject->GetDictionary().HasKey(PoDoFo::PdfName("LGIDict") ) )
               {
                  geoPdfObject = pdfObject->GetDictionary().GetKey(PoDoFo::PdfName("LGIDict"));
                  m_isLGIDict = true;
               }
               else if (pdfObject->GetDictionary().HasKey(PoDoFo::PdfName("VP") ))
               {
                  geoPdfObject = pdfObject->GetDictionary().GetKey(PoDoFo::PdfName("VP"));
               }
            
               if (geoPdfObject)
               {
                  std::string imagInfo; //try to get tile structure information
                  geoPdfObject->ToString(imagInfo);
                  ossimString imagInfoStr = imagInfo;
               
                  if (imagInfoStr.downcase().contains("tile") == false)//if not, try to get tile structure info from source object
                  {
                     PoDoFo::PdfObject* pSource = pdfPage->GetResources();
                     pSource->ToString(imagInfo);
                     imagInfoStr = imagInfo;
                  }
                  std::vector<ossimString> imageInfoVector;
                  imageInfoVector.push_back(imagInfoStr);
                  parseTileStructure(imageInfoVector);
                  setPodofoImageInfo();
                  buildFrameEntryArray();
                  setPodofoInfo(geoPdfObject);
               }
               else
               {
                  continue;
               }

               // Create the geometry.
               ossimRefPtr<ossimImageGeometry> geoImage = new ossimImageGeometry();
               geoImage->setProjection(getGeoProjection());
               geoImage->setImageSize(ossimIpt(m_numberOfSamples, m_numberOfLines));

               // Create the node.
               ossimGeoPdfVectorPageNode* node = new ossimGeoPdfVectorPageNode();
               node->setGeoImage(geoImage);
               node->setBoundingBox(computeBoundingRect(geoImage));

               // Add to array.
               m_pageVector.push_back(node);
            
               m_pdfMemDocument->FreeObjectMemory(pdfObject);
            }
         
         } // Matches: for(int i = 0; i < count; ++i)
      
         if ( m_pageVector.size() )
         {
            result = true;
         }
      }
   
      m_tile = ossimImageDataFactory::instance()->create(this, this);
      m_tile->initialize();
      m_cacheTile = (ossimImageData*)m_tile->dup();
      m_cacheTile->initialize();
   
      m_cacheSize.x = m_frameWidthVector[0];
      m_cacheSize.y = m_frameHeightVector[0];
   
      ossimAppFixedTileCache::instance()->deleteCache(m_cacheId);
      m_cacheId = ossimAppFixedTileCache::instance()->newTileCache(getImageRectangle(), m_cacheSize);
   
      completeOpen();
   
      if (result == false)
      {
         closeEntry();
      }
      
   } // Matches: if ( theImageFile.size() )
   
   return result;
}

void ossimGeoPdfReader::parseTileStructure(std::vector<ossimString> tileInfo)
{
   m_podofoTileInfo.clear();
   std::vector<ossimString> tileInfoVector;
   for (ossim_uint32 index = 0; index < tileInfo.size(); ++index)
   {
      std::vector<ossimString> tmpVector = tileInfo[index].split("\n");
      for (ossim_uint32 i = 0; i < tmpVector.size(); i++)
      {
         ossimString tmpStr = tmpVector[i];
         if (tmpStr.downcase().contains("layer_"))
         {
            buildTileInfo(tmpStr);
         }
         else if (tmpStr.downcase().contains("tile"))
         {
            tileInfoVector.push_back(tmpStr);
         }
      }
   }

   if (m_podofoTileInfo.size() == 0)//does not layer information, try tile information
   {
      for (ossim_uint32 index = 0; index < tileInfoVector.size(); ++index)
      {
         buildTileInfo(tileInfoVector[index]);
      }
   }
  
   tileInfoVector.clear();
}

void ossimGeoPdfReader::buildTileInfo(ossimString tileInfo)
{
   std::vector<ossimString> tmpKeyValue = tileInfo.split(" ");

   if (tmpKeyValue.size() > 1)
   {
      std::vector<ossimString> layerNameVector = tmpKeyValue[0].split("_");
      ossim_int32 col = layerNameVector[layerNameVector.size()-2].toInt();
      ossim_int32 row = layerNameVector[layerNameVector.size()-1].toInt();
      std::pair<ossim_int32, ossim_int32> tileRowCol;
      tileRowCol = make_pair(row, col);
      ossim_int32 tileIndex = tmpKeyValue[1].toInt();

      m_podofoTileInfo[tileIndex] = tileRowCol;
   }
}

ossimDrect ossimGeoPdfReader::computeBoundingRect(ossimRefPtr<ossimImageGeometry> geoImage)
{
   // ossim_uint32 entry = 0;
   ossimKeywordlist kwl; 
   ossimString pfx = "image";
   pfx += ossimString::toString(0);
   pfx += ".";

   double ll_lat = 0.0;
   double ll_lon = 0.0;
   double lr_lat = 0.0;
   double lr_lon = 0.0;
   double ul_lat = 0.0;
   double ul_lon = 0.0;
   double ur_lat = 0.0;
   double ur_lon = 0.0;
   ossimDrect rect2(ossimDpt(0,0), ossimDpt(0,0),ossimDpt(0,0),ossimDpt(0,0));

   ossimProjection* proj = geoImage->getProjection();
   if (m_isLGIDict)
   {
      ossimString bBoxStr = getValueFromMap(m_podofoProjInfo, "Registration");
      std::vector<ossimString> tmpVector = bBoxStr.split(")");
      std::vector<ossimString> tmpVector1;
      for (ossim_uint32 i = 0; i < tmpVector.size(); i++)
      {
         ossimString tmpStr = tmpVector[i];
         if (tmpStr.contains("(") || tmpStr.contains(")"))
         {
            tmpStr = tmpStr.substitute("[", "", true).trim();
            tmpStr = tmpStr.substitute("]", "", true).trim();
            tmpStr = tmpStr.substitute("(", "", true).trim();
            tmpStr = tmpStr.substitute(")", "", true).trim();
            tmpVector1.push_back(tmpStr);
         }
      }

      if (tmpVector1.size() == 8)
      {
         ll_lon = tmpVector1[2].toDouble();
         ll_lat = tmpVector1[3].toDouble();
         ur_lon = tmpVector1[6].toDouble();
         ur_lat = tmpVector1[7].toDouble();

         lr_lon = tmpVector1[6].toDouble();
         lr_lat = tmpVector1[3].toDouble();
         ul_lon = tmpVector1[2].toDouble();
         ul_lat = tmpVector1[7].toDouble();
      }
      else if (tmpVector1.size() == 16)
      {
         ll_lon = tmpVector1[2].toDouble();
         ll_lat = tmpVector1[3].toDouble();
         lr_lon = tmpVector1[6].toDouble();
         lr_lat = tmpVector1[7].toDouble();

         ur_lon = tmpVector1[10].toDouble();
         ur_lat = tmpVector1[11].toDouble();
         ul_lon = tmpVector1[14].toDouble();
         ul_lat = tmpVector1[15].toDouble();
      }
   }
   else
   {
      std::vector<ossimString> tmpVector;
      ossimString coodStr = getValueFromMap(m_podofoProjInfo, "GPTS");
      if (!coodStr.empty())
      {
         std::vector<ossimString> coodVector = coodStr.split(" ");

         for (ossim_uint32 i = 0; i < coodVector.size(); i++)
         {
            ossimString tmp = coodVector[i].trim();
            if (!tmp.empty() && tmp.contains("[") == false &&  tmp.contains("]") == false)
            {
               tmpVector.push_back(tmp);
            }
         }

         ll_lat = tmpVector[0].toDouble();
         ll_lon = tmpVector[1].toDouble();
         lr_lat = tmpVector[2].toDouble();
         lr_lon = tmpVector[3].toDouble();
         ur_lat = tmpVector[4].toDouble();
         ur_lon = tmpVector[5].toDouble();
         ul_lat = tmpVector[6].toDouble();
         ul_lon = tmpVector[7].toDouble();
      }
   }

   if(proj)
   {
      ossimMapProjection* mapProj = dynamic_cast<ossimMapProjection*>(proj);
  
      ossimDrect rect(ossimDpt(ul_lon, ul_lat),
                      ossimDpt(ur_lon, ur_lat),
                      ossimDpt(lr_lon, lr_lat),
                      ossimDpt(ll_lon, ll_lat), OSSIM_RIGHT_HANDED); 

      std::vector<ossimGpt> points;
      if (mapProj->isGeographic())
      {
         ossimGpt g1(ul_lat, ul_lon);
         ossimGpt g2(ur_lat, ur_lon);
         ossimGpt g3(lr_lat, lr_lon);
         ossimGpt g4(ll_lat, ll_lon);

         points.push_back(g1);
         points.push_back(g2);
         points.push_back(g3);
         points.push_back(g4);
         ossimGpt tie(ul_lat, ul_lon);
         mapProj->setUlTiePoints(tie);
         if (m_numberOfLines != 0 && m_numberOfSamples != 0)
         {
            ossim_float64 scaleLat = (ul_lat - lr_lat)/m_numberOfLines;
            ossim_float64 scaleLon = (lr_lon - ul_lon)/m_numberOfSamples;

            ossimDpt gsd(fabs(scaleLon), fabs(scaleLat));
            mapProj->setDecimalDegreesPerPixel(gsd);
         }
      }
      else if (!mapProj->isGeographic() && m_isLGIDict == false)
      {
         ossimGpt g1(ul_lat, ul_lon);
         ossimGpt g2(ur_lat, ur_lon);
         ossimGpt g3(lr_lat, lr_lon);
         ossimGpt g4(ll_lat, ll_lon);

         points.push_back(g1);
         points.push_back(g2);
         points.push_back(g3);
         points.push_back(g4);
        
         if (m_numberOfLines != 0 && m_numberOfSamples != 0)
         {
            ossimDpt ulEn = mapProj->forward(g1);
            ossimDpt lrEn = mapProj->forward(g3);

            mapProj->setUlEastingNorthing(ulEn);

            ossim_float64 scaleLat = (ulEn.y - lrEn.y)/m_numberOfLines;
            ossim_float64 scaleLon = (lrEn.x - ulEn.x)/m_numberOfSamples;

            ossimDpt gsd(fabs(scaleLon), fabs(scaleLat));
            mapProj->setMetersPerPixel(gsd);
         }
      }
      else
      {
         ossimGpt g1 = proj->inverse(rect.ul());
         ossimGpt g2 = proj->inverse(rect.ur());
         ossimGpt g3 = proj->inverse(rect.lr());
         ossimGpt g4 = proj->inverse(rect.ll());

         points.push_back(g1);
         points.push_back(g2);
         points.push_back(g3);
         points.push_back(g4);

         ossimDpt tie(ul_lon, ul_lat);
         if (mapProj)
         {
            mapProj->setUlTiePoints(tie);
            if (m_numberOfLines != 0 && m_numberOfSamples != 0)
            {
               ossim_float64 scaleLat = (ul_lat - lr_lat)/m_numberOfLines;
               ossim_float64 scaleLon = (ur_lon - ll_lon)/m_numberOfSamples; 
               ossimDpt gsd(fabs(scaleLon), fabs(scaleLat));
               mapProj->setMetersPerPixel(gsd);
            }
         }
      }

      std::vector<ossimDpt> rectTmp;
      rectTmp.resize(4);

      for(std::vector<ossimGpt>::size_type index=0; index < 4; ++index)
      {
         geoImage->worldToLocal(points[(int)index], rectTmp[(int)index]);
      }

      rect2 = ossimDrect(rectTmp);
     
   }
   return rect2;
}

bool ossimGeoPdfReader::setCurrentEntry(ossim_uint32 entryIdx)
{
   if (m_pageVector.size() > entryIdx)
   {
      theGeometry = 0;
      m_imageRect.makeNan();
      theGeometry = m_pageVector[entryIdx]->m_geoImage;
      m_imageRect = m_pageVector[entryIdx]->m_boundingRect;
      if (theGeometry.valid())
      {
         return true;
      }
   }
   return false;
}

bool ossimGeoPdfReader::saveState(ossimKeywordlist& kwl,
                                  const char* prefix) const
{
   for (ossim_uint32 i = 0; i < m_pageVector.size(); i++)
   {
      ossimRefPtr<ossimImageGeometry> imageGeometry = m_pageVector[i]->m_geoImage;
      if(imageGeometry.valid())
      {
         imageGeometry->saveState(kwl, prefix);
      }
   }

   return ossimImageHandler::saveState(kwl, prefix);
}

bool ossimGeoPdfReader::isOpen()const
{
   ossimString ext = theImageFile.ext().downcase();

   if(ext == "pdf")
   {
      return true;
   }
   else
   {
      return false;
   }
}

void ossimGeoPdfReader::closeEntry()
{
   m_tile = 0;
   m_cacheTile = 0;
   m_podofoProjInfo.clear();
   m_podofoTileInfo.clear();
   m_frameWidthVector.clear();
   m_frameHeightVector.clear();
   
   if (m_pdfMemDocument)
   {
      std::map<int, PoDoFo::PdfObject*>::iterator it = m_podofoImageObjs.begin();
      while (it != m_podofoImageObjs.end())
      {
         m_pdfMemDocument->FreeObjectMemory(it->second);
         it++;
      }
   }
   m_podofoImageObjs.clear();

   if (m_frameEntryArray.size() > 0)
   {
      for (ossim_uint32 i = 0; i < m_frameEntryArray.size(); i++)
      {
         std::vector< std::vector<PoDoFo::PdfObject*> > tmpVector = m_frameEntryArray[i];
         for (ossim_uint32 j = 0; j < tmpVector.size(); j++)
         {
            std::vector<PoDoFo::PdfObject*> tmpVector1 = tmpVector[j];
            for (ossim_uint32 k = 0; k < tmpVector1.size(); k++)
            {
               m_pdfMemDocument->FreeObjectMemory(tmpVector1[k]);
            }
         }
      }
   }
  m_frameEntryArray.clear();
   
   if (m_pdfMemDocument != NULL)
   {
      delete m_pdfMemDocument;
      m_pdfMemDocument = 0;
   }
   
   for(ossim_uint32 i = 0; i < m_pageVector.size(); ++i)
   {
      if(m_pageVector[i])
      {
         delete m_pageVector[i];
      }
   }
   m_pageVector.clear();
   
   ossimImageHandler::close();
}

ossim_uint32 ossimGeoPdfReader::getNumberOfEntries() const
{
   return (ossim_uint32)m_pageVector.size();
}

void ossimGeoPdfReader::getEntryList(std::vector<ossim_uint32>& entryList) const
{
   if (m_pageVector.size() > 0)
   {
      for (ossim_uint32 i = 0; i < getNumberOfEntries(); i++)
      {
         entryList.push_back(i);
      }
   }
   else
   {
      ossimImageHandler::getEntryList(entryList);
   }
}

ossimRefPtr<ossimImageData> ossimGeoPdfReader::getTile(
   const ossimIrect& rect, ossim_uint32 resLevel)
{
   if (m_tile.valid())
   {
      // Image rectangle must be set prior to calling getTile.
      m_tile->setImageRectangle(rect);

      if ( getTile( m_tile.get(), resLevel ) == false )
      {
         if (m_tile->getDataObjectStatus() != OSSIM_NULL)
         {
            m_tile->makeBlank();
         }
      }
   }

   return m_tile;
}

bool ossimGeoPdfReader::getTile(ossimImageData* result,
                                ossim_uint32 resLevel)
{
   bool status = false;

   //---
   // Not open, this tile source bypassed, or invalid res level,
   // return a blank tile.
   //---
   if( isOpen() && isSourceEnabled() && isValidRLevel(resLevel) &&
       result && (result->getNumberOfBands() == getNumberOfOutputBands()) )
   {
      result->ref();  // Increment ref count.

      //---
      // Check for overview tile.  Some overviews can contain r0 so always
      // call even if resLevel is 0.  Method returns true on success, false
      // on error.
      //---
      status = getOverviewTile(resLevel, result);

      if (!status) // Did not get an overview tile.
      {
         status = true;

         ossimIrect tile_rect = result->getImageRectangle();     

         if (getImageRectangle().intersects(tile_rect))
         {
            // Make a clip rect.
            ossimIrect clip_rect = tile_rect.clipToRect(getImageRectangle());

            std::vector<ossimFrameEntryData> frames = getIntersectingEntries(clip_rect);

            if (!tile_rect.completely_within(getImageRectangle()) )
            {
               result->makeBlank();
            }

            for(ossim_uint32 idx = 0; idx < frames.size(); ++idx)
            {
               resetCacheBuffer(frames[idx]);
               m_currentRow = frames[idx].theRow;

               fillTile(ossim_uint8(0), clip_rect, result);
            }
         }
         else // No intersection...
         {
            result->makeBlank();
         }
      }

      result->unref();  // Decrement ref count.
   }

   return status;
}

template <class T>
void ossimGeoPdfReader::fillTile(T, // dummy template variable
                                 const ossimIrect& clip_rect,
                                 ossimImageData* tile)
{ 
   if (m_isJpeg == true)
   {
      tile->loadTile(m_cacheTile.get());
   }
   else
   {
      const ossimIrect img_rect = tile->getImageRectangle();
      ossimIrect src_rect = m_cacheTile->getImageRectangle();
      if (!img_rect.intersects(src_rect) )
      {
         return; // Nothing to do here.
      }

      // Check the clip rect.
      if (!clip_rect.completely_within(img_rect))
      {
         return;
      }

      ossimIrect clipRect = clip_rect.clipToRect(src_rect);
      void* src = (void*)m_cacheTile->getBuf();
      if (!src)
      {
         // Set the error...
         ossimSetError(getClassName(),
                       ossimErrorCodes::OSSIM_ERROR,
                       "%s File %s line %d\nNULL pointer passed to method!");
         return;
      }

      // Check the status and allocate memory if needed.
      if (tile->getDataObjectStatus() == OSSIM_NULL)
      {
         tile->initialize();
      }

      // Get the width of the buffers.
      ossim_uint32 num_bands = tile->getNumberOfBands();
      ossim_uint32 s_width = src_rect.width();
      ossim_uint32 d_width = tile->getWidth();
      ossim_uint32 s_band_offset = s_width * src_rect.height();

      const T* s = static_cast<const T*>(src);

      ossim_uint32 destinationOffset = (clipRect.ul().y - img_rect.ul().y) * d_width +
         (clipRect.ul().x - img_rect.ul().x);

      ossim_uint32 destinationIndex = destinationOffset;

      ossim_uint32 clipHeight = clipRect.height();
      ossim_uint32 clipWidth = clipRect.width();

      ossim_uint32 sourceOffset = (src_rect.ll().y - clipRect.ul().y) * s_width +
         (clipRect.ul().x - src_rect.ul().x);
      ossim_uint32 sourceIndex = sourceOffset;

      // Copy the data.
      for (ossim_uint32 band=0; band<num_bands; band++)
      {
         T* destinationBand = static_cast<T*>(tile->getBuf(band));
         destinationIndex = destinationOffset;
         sourceIndex = sourceOffset + s_band_offset*band;

         for (ossim_uint32 line = 0; line < clipHeight; ++line)
         {
            for (ossim_uint32 sample = 0; sample < clipWidth; ++sample)
            {
               destinationBand[destinationIndex + sample]
                  = s[sourceIndex+sample];
            }
            sourceIndex -= s_width;
            destinationIndex += d_width;
         }
      }
   }
   tile->validate();
}

ossim_uint32 ossimGeoPdfReader::getNumberOfInputBands() const
{
   return m_numberOfBands;
}

ossim_uint32 ossimGeoPdfReader::getNumberOfOutputBands()const
{
   return m_numberOfBands;
}

ossim_uint32 ossimGeoPdfReader::getImageTileWidth() const
{
   return 0;
}

ossim_uint32 ossimGeoPdfReader::getImageTileHeight() const
{
   return 0;
}

ossimScalarType ossimGeoPdfReader::getOutputScalarType() const
{
   return m_scalarType;
}

ossimRefPtr<ossimImageGeometry> ossimGeoPdfReader::getImageGeometry()
{
   if ( !theGeometry )
   {
      // Check for external geom:
      theGeometry = getExternalImageGeometry();
      
      if ( !theGeometry.valid() )
      {
         // Internnal geom:
         theGeometry = getInternalImageGeometry();
         
         if ( !theGeometry.valid() )
         {
            theGeometry = new ossimImageGeometry();
         }

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

ossimRefPtr<ossimImageGeometry> ossimGeoPdfReader::getInternalImageGeometry() const
{
   ossimRefPtr<ossimImageGeometry> geom = 0;
   if (m_pageVector.size() > 0)
   {
      geom = m_pageVector[0]->m_geoImage;
   }
   return geom;
}

bool ossimGeoPdfReader::loadState(const ossimKeywordlist& kwl, const char* prefix)
{
   bool result = false;
   ossimString type = kwl.find(prefix, ossimKeywordNames::TYPE_KW);
   if( (type == "ossimGeoPdfReader") || (type == "ossimImageHandler") )
   {
      if ( ossimImageHandler::loadState(kwl, prefix) )
      {
         result = open();
      }
   }
   return result;
}

ossimProjection* ossimGeoPdfReader::getGeoProjection()
{
   if (m_isLGIDict)
   {
      return getLGIDictGeoProjection();
   }
   else
   {
      return getVPGeoProjection();
   }
   return NULL;
}

ossimProjection* ossimGeoPdfReader::getVPGeoProjection()
{
   ossimKeywordlist kwl; 
   ossimString pfx = "image";
   pfx += ossimString::toString(0);
   pfx += ".";

   ossimProjection* proj = NULL;

   ossimString projCode = getValueFromMap(m_podofoProjInfo, "EPSG");
   if (!projCode.empty())
   {
      kwl.add(pfx.c_str(), ossimKeywordNames::PCS_CODE_KW, projCode, true);
      projCode = ossimString("EPSG:") + projCode;
      proj = ossimEpsgProjectionFactory::instance()->createProjection(projCode);
   }
   return proj;
}

ossimProjection* ossimGeoPdfReader::getLGIDictGeoProjection()
{
   ossimProjection* proj = NULL;
   ossimString projContents = getValueFromMap(m_podofoProjInfo, "Projection");
   if (!projContents.empty())
   {
      proj = getProjectionFromStr(projContents);
   }
   return proj;
}

void ossimGeoPdfReader::setPodofoInfo(PoDoFo::PdfObject* object)
{
   m_podofoProjInfo.clear();
   if (object->IsReference())
   {
      setPodofoRefInfo(object);
   }
   else if (object->IsDictionary())
   {
      setPodofoDictInfo(object);
   }
   else if (object->IsArray())
   {
      setPodofoArrayInfo(object);
   }
}

void ossimGeoPdfReader::setPodofoArrayInfo(PoDoFo::PdfObject* object)
{
   PoDoFo::PdfArray arrayObj = object->GetArray();
   for (ossim_uint32 i = 0; i < arrayObj.size(); i++)
   {
      PoDoFo::PdfObject objTmp = arrayObj[i];
      if (objTmp.IsDictionary())
      {
         setPodofoDictInfo(&objTmp);
      }
      else if (objTmp.IsReference())
      {
         setPodofoRefInfo(&objTmp);
      }
   }
}

void ossimGeoPdfReader::setPodofoRefInfo(PoDoFo::PdfObject* object)
{
   PoDoFo::PdfReference pdfReference = object->GetReference();
   PoDoFo::PdfVecObjects objVector = m_pdfMemDocument->GetObjects();
   if (pdfReference.IsIndirect())
   {
      PoDoFo::PdfObject* refObj = objVector.GetObject(pdfReference);
      if (refObj->IsDictionary())
      {
         setPodofoDictInfo(refObj);
      }
   }
}

void ossimGeoPdfReader::setPodofoDictInfo(PoDoFo::PdfObject* object)
{
   std::string valueStr;
   PoDoFo::PdfDictionary pdfDictionary = object->GetDictionary();
   PoDoFo::TKeyMap keyMap = pdfDictionary.GetKeys();
   PoDoFo::TKeyMap::iterator it = keyMap.begin();
   while (it != keyMap.end())
   {
      ossimString refName = ossimString(it->first.GetName());
      PoDoFo::PdfObject* refObj = it->second;
      if (refObj->IsReference())
      {
         setPodofoRefInfo(refObj);
      }
      else
      {
         refObj->ToString(valueStr);
         if (valueStr != "")
         {
            m_podofoProjInfo[refName] = valueStr;
         }
      }
      it++;
   }
}

void ossimGeoPdfReader::setPodofoImageInfo()
{
   PoDoFo::PdfObject*  pObj  = NULL;
   m_podofoImageObjs.clear();
   std::vector<ossimString> tmpForm;
   int count = 1;
   int rowCount = 0;
   bool isDctDecode = false;
   PoDoFo::TCIVecObjects it = m_pdfMemDocument->GetObjects().begin();
   while( it != m_pdfMemDocument->GetObjects().end() )
   {
      if ((*it)->IsDictionary())
      {
         pObj = (*it)->GetDictionary().GetKey( PoDoFo::PdfName::KeyType );
         if( pObj && pObj->IsName() && ( pObj->GetName().GetName() == "XObject" ))
         {
            pObj = (*it)->GetDictionary().GetKey( PoDoFo::PdfName::KeySubtype );
            if( pObj && pObj->IsName() && ( pObj->GetName().GetName() == "Form" ))
            {
               std::string imageTileInfo;
               (*it)->ToString(imageTileInfo);
               ossimString imageTileInfoStr = imageTileInfo;
               if (imageTileInfoStr.downcase().contains("tile"))
               {
                  tmpForm.push_back(imageTileInfoStr);
               }
            }
            else if( pObj && pObj->IsName() && ( pObj->GetName().GetName() == "Image" ))
            {
               pObj = (*it)->GetDictionary().GetKey( PoDoFo::PdfName::KeyFilter );
               if( pObj->IsName() && ( pObj->GetName().GetName() == "DCTDecode" ))
               {
                  m_podofoImageObjs[count] = (*it);
                  isDctDecode = true;
               }
               else if( pObj->IsArray() && pObj->GetArray().GetSize() == 1 && 
                        pObj->GetArray()[0].IsName() && (pObj->GetArray()[0].GetName().GetName() == "DCTDecode"))
               {
                  m_podofoImageObjs[count] = (*it);
                  isDctDecode = true;
               }
               else if (pObj->IsName() && (pObj->GetName().GetName() == "FlateDecode") )
               {
                  if (isDctDecode == false)
                  {
                     std::pair<ossim_int32, ossim_int32> tileRowCol;
                     tileRowCol = make_pair(rowCount, 0);
                     m_podofoTileInfo[count] = tileRowCol;
                     m_podofoImageObjs[count] = (*it);
                     m_isJpeg = false;
                  }
                  rowCount++;
               }
            }
         }
      }
      ++it;
      ++count;
   }

   if (m_isJpeg == false)
   {
      std::map<ossim_int32, ossim_int32> widthMap;
      std::map<ossim_int32, PoDoFo::PdfObject*>::iterator it = m_podofoImageObjs.begin();
      while (it != m_podofoImageObjs.end())
      {
         PoDoFo::PdfObject* tmpObj = it->second;
         PoDoFo::PdfImage image(tmpObj);
         int width = image.GetWidth();
         widthMap[it->first] = width;
         it++;
      }

      std::map<ossim_int32, ossim_int32>::iterator itWidth =  widthMap.begin();
      ossim_int32 firstId = itWidth->first;
      ossim_int32 firstWidth = itWidth->second;
      widthMap.erase(itWidth);

      itWidth = widthMap.end();
      itWidth--;
      ossim_int32 lastId = itWidth->first;
      ossim_int32 lastWidth = itWidth->second;
    
      itWidth = widthMap.begin();
      ossim_int32 invalidId = 0;
      while (itWidth != widthMap.end())
      {
         ossim_int32 tmpWidth = itWidth->second;
         ossim_int32 tmpId = itWidth->first;
         if (firstWidth == tmpWidth)
         {
            invalidId = lastId;
         }
         else if ( lastWidth == tmpWidth )
         {
            invalidId = firstId;
         }
         else
         {
            invalidId = tmpId;
         }
         break;
         itWidth++;
      }

      it = m_podofoImageObjs.find(invalidId);
      if (it != m_podofoImageObjs.end())
      {
         m_podofoImageObjs.erase(it);
      }
   
      std::map<ossim_int32, std::pair<ossim_int32, ossim_int32> >::iterator itTile = m_podofoTileInfo.find(invalidId);
      if (itTile != m_podofoTileInfo.end())
      {
         m_podofoTileInfo.erase(itTile);
      }

      if (invalidId == firstId)
      {
         itTile = m_podofoTileInfo.begin();
         while (itTile != m_podofoTileInfo.end())
         {
            ossim_int32 index = itTile->first;
            ossim_int32 tileRow = itTile->second.first;
            if (tileRow > 0)
            {
               std::pair<ossim_int32, ossim_int32> tileRowCol;
               tileRowCol = make_pair(tileRow-1, 0);
               m_podofoTileInfo[index] = tileRowCol;
            }
            else
            {
               break;
            }
            itTile++;
         }
      }
      widthMap.clear();
   }
  
   if (tmpForm.size() > 0 && m_podofoTileInfo.size() == 0)
   {
      parseTileStructure(tmpForm);
   }
   tmpForm.clear();
}

void ossimGeoPdfReader::buildFrameEntryArray()
{
   m_frameWidthVector.clear();
   m_frameHeightVector.clear();
   ossim_int32 row = 0;
   ossim_int32 col = 0;
   if (m_podofoImageObjs.size() > 0 && m_podofoTileInfo.size() > 0)
   {
      //Podofo libary may skip some objects when reading file which cause the count of objects 
      //different between tile info and image info. need offset to correct it.
      std::map<ossim_int32, std::pair<ossim_int32, ossim_int32> >::iterator itTileEnd;
      itTileEnd = m_podofoTileInfo.end();
      itTileEnd--;

      std::map<ossim_int32, PoDoFo::PdfObject*>::iterator itObjeEnd;
      itObjeEnd = m_podofoImageObjs.end();
      itObjeEnd--;

      ossim_int32 indexOffset = itTileEnd->first - itObjeEnd->first;

      std::map<ossim_int32, std::pair<ossim_int32, ossim_int32> >::iterator it = m_podofoTileInfo.begin();
      //get number of vertical frames and number of horizontal frames
      while (it != m_podofoTileInfo.end())
      {
         ossim_int32 index = it->first;
         if (indexOffset > 0)
         {
            index = index - indexOffset;
         }
         std::map<ossim_int32, PoDoFo::PdfObject*>::iterator itObj = m_podofoImageObjs.find(index);
         if (itObj != m_podofoImageObjs.end())
         {
            row = it->second.first;
            col = it->second.second;
            if (m_numOfFramesHorizontal <= col)
            {
               m_numOfFramesHorizontal = col;
            }
            if (m_numOfFramesVertical <= row)
            {
               m_numOfFramesVertical = row;
            }
         }
         it++;
      }

      //set up array size
      m_numOfFramesHorizontal = m_numOfFramesHorizontal + 1;
      m_numOfFramesVertical = m_numOfFramesVertical + 1;

      m_frameEntryArray.resize(m_numOfFramesVertical);
      for(ossim_uint32 index = 0; index < m_frameEntryArray.size(); index++)
      {
         m_frameEntryArray[index].resize(m_numOfFramesHorizontal);
      }

      //build array
      it = m_podofoTileInfo.begin();
      std::vector<PoDoFo::PdfObject*> tmpObjVector;
      while (it != m_podofoTileInfo.end())
      {
         ossim_int32 index = it->first;
         if (indexOffset > 0)
         {
            index = index - indexOffset;
         }
         std::map<ossim_int32, PoDoFo::PdfObject*>::iterator itObj = m_podofoImageObjs.find(index);
         if (itObj != m_podofoImageObjs.end())
         {
            ossim_int32 rowCount = it->second.first;
            ossim_int32 colCount = it->second.second;
            m_frameEntryArray[rowCount][colCount].push_back(itObj->second);
         }
         it++;
      }
   
      //calculate lines and samples
      for (row = 0; row < m_numOfFramesVertical; ++row)
      {
         PoDoFo::PdfObject* tmpObj = m_frameEntryArray[row][0][0];
         PoDoFo::PdfImage image(tmpObj);
         int height = image.GetHeight();
         m_frameHeightVector[row] = height;
      }

      for (col = 0; col < m_numOfFramesHorizontal; ++col)
      {
         PoDoFo::PdfObject* tmpObj = m_frameEntryArray[0][col][0];
         PoDoFo::PdfImage image(tmpObj);
         int width = image.GetWidth();
         m_frameWidthVector[col] = width;
      }
   
      std::map<ossim_int32, ossim_int32>::iterator  itWidth = m_frameWidthVector.begin();
      while (itWidth != m_frameWidthVector.end())
      {
         m_numberOfSamples = m_numberOfSamples + itWidth->second;
         itWidth++;
      }

      std::map<ossim_int32, ossim_int32>::iterator  itHeight = m_frameHeightVector.begin();
      while (itHeight != m_frameHeightVector.end())
      {
         m_numberOfLines = m_numberOfLines + itHeight->second;
         itHeight++;
      }
      m_imageRect = ossimIrect(0, 0, m_numberOfSamples-1, m_numberOfLines-1);
  
      //get the number of bands
      if (m_frameEntryArray[0][0].size() > 2)
      {
         m_numberOfBands = (ossim_uint32) m_frameEntryArray[0][0].size();
      }
      else
      {
         PoDoFo::PdfObject* tmpObject = m_frameEntryArray[0][0][0];
         PoDoFo::PdfMemStream* pStream = dynamic_cast<PoDoFo::PdfMemStream*>(tmpObject->GetStream());
         pStream->Uncompress();
         ossim_int32 length = pStream->GetLength();
         m_numberOfBands = length/(m_frameHeightVector[0]*m_frameWidthVector[0]);
      }
   }
}

std::vector<ossimGeoPdfReader::ossimFrameEntryData> ossimGeoPdfReader::getIntersectingEntries(const ossimIrect& rect)
{
   std::vector<ossimFrameEntryData> result;

   // make sure we have the Toc entry to render
   if(!isOpen()) return result;

   ossimIrect imageRect = getImageRectangle();
   if(rect.intersects(imageRect))
   {
      ossimIrect clipRect  = rect.clipToRect(imageRect);
      ossimIrect frameRect(clipRect.ul().x/m_frameWidthVector[0],
                           clipRect.ul().y/m_frameHeightVector[0],
                           clipRect.lr().x/m_frameWidthVector[0],
                           clipRect.lr().y/m_frameHeightVector[0]);

      for(ossim_int32 row = frameRect.ul().y; row <= frameRect.lr().y; ++row)
      {
         for(ossim_int32 col = frameRect.ul().x; col <= frameRect.lr().x; ++col)
         {
            ossim_int32 pixelRow = 0;
            ossim_int32 pixelCol = 0;
            for (ossim_int32 iRow = 0; iRow < row; ++iRow)
            {
               pixelRow = pixelRow + m_frameHeightVector[iRow];
            }
            for (ossim_int32 iCol = 0; iCol < col; ++iCol)
            {
               pixelCol = pixelCol + m_frameWidthVector[iCol];
            }
            std::vector<PoDoFo::PdfObject*> tempEntry = m_frameEntryArray[row][col];
            if(tempEntry.size() > 0)
            {
               result.push_back(ossimFrameEntryData(row, 
                                                    col, 
                                                    pixelRow,
                                                    pixelCol,
                                                    tempEntry));
            }
         }
      }
   }

   return result;
}

ossimProjection* ossimGeoPdfReader::getProjectionFromStr(ossimString projContents)
{
   std::vector<ossimString> tmpVector = projContents.split("\n");
   std::map<ossimString, ossimString, ossimStringLtstr> projectionMap;

   for (ossim_uint32 i = 0; i < tmpVector.size(); i++)
   {
      ossimString tmpStr = tmpVector[i];
      if (tmpStr.contains("/"))
      {
         std::vector<ossimString> tmpKeyValue = tmpStr.split(" ");
         ossimString key;
         ossimString valueStr;
         for (ossim_uint32 j = 0; j < tmpKeyValue.size(); j++)
         {
            if (tmpKeyValue[j].contains("/"))
            {
               key = tmpKeyValue[j].substitute("/", "", true).trim();
            }
            else 
            {
               valueStr = tmpKeyValue[j].substitute(")", "", true).trim();
               valueStr = valueStr.substitute("(", "", true).trim();
            }
         }
         if (!key.empty() && !valueStr.empty())
         {
            projectionMap[key] = valueStr;
         }
      }
   }

   ossimString projCode = getValueFromMap(projectionMap, "ProjectionType");
   ossimString datumCode = getValueFromMap(projectionMap, "Datum");

   ossimString originLatitudeStr = getValueFromMap(projectionMap, originLatKey); 
   ossimString centralMeridianStr = getValueFromMap(projectionMap, centralMeridianKey);
   ossimString standardParallelOneStr = getValueFromMap(projectionMap, standardParallelOneKey); 
   ossimString standardParallelTwoStr = getValueFromMap(projectionMap, standardParallelTwoKey); 
   ossimString falseEastingStr = getValueFromMap(projectionMap, falseEastingKey); 
   ossimString falseNorthingStr = getValueFromMap(projectionMap, falseNorthingKey);
   ossimString scaleFactorStr = getValueFromMap(projectionMap, scaleFactorKey);

   if (projCode == "GEODETIC" || projCode == "GEOGRAPHIC")
   {
      ossimString codeName = "EPSG:4326";
      if (datumCode == "WD")
      {
         codeName = "EPSG:4322";
      }
      ossimProjection* proj = ossimEpsgProjectionFactory::instance()->createProjection(codeName);
      return proj;
   }
   else if (projCode == "UT")
   {
      ossimString zoneStr = getValueFromMap(projectionMap, zoneKey);
      ossimString hemisphereStr = getValueFromMap(projectionMap, hemisphereKey);
      ossimUtmProjection* utm = new ossimUtmProjection(zoneStr.toInt32());
      if (!datumCode.empty())
      {
         utm->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!zoneStr.empty())
      {
         utm->setZone(zoneStr.toInt32());
      }

      if (!hemisphereStr.empty())
      {
         utm->setHemisphere(*hemisphereStr.c_str());
      }

      return utm;
   }
   else if (projCode == "UP")
   {
      ossimString hemiStr = getValueFromMap(projectionMap, hemisphereKey);
      ossimUpsProjection* ups = new ossimUpsProjection();

      if (!datumCode.empty())
      {
         ups->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!hemiStr.empty())
      {
         ups->setHemisphere(*hemiStr.c_str());
      }

      return ups;
   }
   else if (projCode == "SPCS")
   {
      ossimString zoneStr = getValueFromMap(projectionMap, zoneKey);

      ossimString spcsCode;
      if (datumCode.upcase().contains("NAR"))
      {
         spcsCode = ossimString("NAD83_" + projCode + "_" + zoneStr);
      }
      else if (datumCode.upcase().contains("NAS"))
      {
         spcsCode = ossimString("NAD83_SPAF_" + zoneStr);
      }
   
      // State plane projections are represented in the EPSG database. Search for corresponding proj
      // by name. Name needs to be exact match as found in the EPSG database files:
      ossimProjection* stateProj = ossimEpsgProjectionFactory::instance()->createProjection(spcsCode);
      if (stateProj)
      {
         return stateProj;
      }
   }
   else if (projCode == "AC")
   {
      ossimGpt origin(originLatitudeStr.toDouble(), centralMeridianStr.toDouble());

      ossimAlbersProjection* albers = new ossimAlbersProjection();

      if (!datumCode.empty())
      {
         albers->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!origin.isNan())
      {
         albers->setOrigin(origin);
      }

      if (!standardParallelOneStr.empty())
      {
         albers->setStandardParallel1(standardParallelOneStr.toDouble());
      }

      if (!standardParallelTwoStr.empty())
      {
         albers->setStandardParallel2(standardParallelTwoStr.toDouble());
      }

      if (!falseEastingStr.empty())
      {
         albers->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         albers->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return albers;
   }
   else if (projCode == "AL")
   {
      ossimAzimEquDistProjection* azimEqud = new ossimAzimEquDistProjection();

      ossimGpt origin(originLatitudeStr.toDouble(), centralMeridianStr.toDouble());

      if (!datumCode.empty())
      {
         azimEqud->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!origin.isNan())
      {
         azimEqud->setOrigin(origin);
      }

      if (!falseEastingStr.empty())
      {
         azimEqud->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         azimEqud->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return azimEqud;
   }
   else if (projCode == "BF")
   {
      ossimBonneProjection* bonne = new ossimBonneProjection();

      ossimGpt origin(originLatitudeStr.toDouble(), centralMeridianStr.toDouble());

      if (!datumCode.empty())
      {
         bonne->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!origin.isNan())
      {
         bonne->setOrigin(origin);
      }

      if (!falseEastingStr.empty())
      {
         bonne->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         bonne->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return bonne;
   }
   else if (projCode == "CS")
   {
      ossimCassiniProjection* cassin = new ossimCassiniProjection();

      ossimGpt origin(originLatitudeStr.toDouble(), centralMeridianStr.toDouble());

      if (!datumCode.empty())
      {
         cassin->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!origin.isNan())
      {
         cassin->setOrigin(origin);
      }

      if (!falseEastingStr.empty())
      {
         cassin->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         cassin->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return cassin;
   }
   else if (projCode == "LI")
   {
      ossimCylEquAreaProjection* cyeq = new ossimCylEquAreaProjection();

      ossimGpt origin(originLatitudeStr.toDouble(), centralMeridianStr.toDouble());

      if (!datumCode.empty())
      {
         cyeq->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!origin.isNan())
      {
         cyeq->setOrigin(origin);
      }

      if (!falseEastingStr.empty())
      {
         cyeq->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         cyeq->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return cyeq;
   }
   else if (projCode == "EF")
   {
      ossimEckert4Projection* ecker4 = new ossimEckert4Projection();

      ossimGpt origin(originLatitudeStr.toDouble(), centralMeridianStr.toDouble());

      if (!datumCode.empty())
      {
         ecker4->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!origin.isNan())
      {
         ecker4->setOrigin(origin);
      }

      if (!falseEastingStr.empty())
      {
         ecker4->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         ecker4->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return ecker4;
   }
   else if (projCode == "ED")
   {
      ossimEckert6Projection* ecker6 = new ossimEckert6Projection();

      if (!datumCode.empty())
      {
         ecker6->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!centralMeridianStr.empty())
      {
         ecker6->setCentralMeridian(centralMeridianStr.toDouble());
      }

      if (!falseEastingStr.empty())
      {
         ecker6->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         ecker6->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return ecker6;
   }
   else if (projCode == "GN")
   {
      ossimGnomonicProjection* gnomon = new ossimGnomonicProjection();

      ossimGpt origin(originLatitudeStr.toDouble(), centralMeridianStr.toDouble());

      if (!datumCode.empty())
      {
         gnomon->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!origin.isNan())
      {
         gnomon->setOrigin(origin);
      }

      if (!falseEastingStr.empty())
      {
         gnomon->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         gnomon->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return gnomon;
   }
   else if (projCode == "LE")
   {
      ossimGpt origin(originLatitudeStr.toDouble(), centralMeridianStr.toDouble());

      ossimLambertConformalConicProjection* lamber = new ossimLambertConformalConicProjection();

      if (!datumCode.empty())
      {
         lamber->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!origin.isNan())
      {
         lamber->setOrigin(origin);
      }

      if (!standardParallelOneStr.empty())
      {
         lamber->setStandardParallel1(standardParallelOneStr.toDouble());
      }

      if (!standardParallelTwoStr.empty())
      {
         lamber->setStandardParallel2(standardParallelTwoStr.toDouble());
      }

      if (!falseEastingStr.empty())
      {
         lamber->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         lamber->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return lamber;
   }
   else if (projCode == "MC")
   {
      ossimMercatorProjection* mercator = new ossimMercatorProjection();

      ossimGpt origin(originLatitudeStr.toDouble(), centralMeridianStr.toDouble());

      if (!datumCode.empty())
      {
         mercator->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!origin.isNan())
      {
         mercator->setOrigin(origin);
      }

      if (!scaleFactorStr.empty())
      {
         mercator->setScaleFactor(scaleFactorStr.toDouble());
      }

      if (!falseEastingStr.empty())
      {
         mercator->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         mercator->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return mercator;
   }
   else if (projCode == "MH")
   {
      ossimMillerProjection* miller = new ossimMillerProjection();

      if (!datumCode.empty())
      {
         miller->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!centralMeridianStr.empty())
      {
         miller->setCentralMeridian(centralMeridianStr.toDouble());
      }

      if (!falseEastingStr.empty())
      {
         miller->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         miller->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return miller;
   }
   else if (projCode == "MP")
   {
      ossimMollweidProjection* mollweid = new ossimMollweidProjection();

      if (!datumCode.empty())
      {
         mollweid->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!centralMeridianStr.empty())
      {
         mollweid->setCentralMeridian(centralMeridianStr.toDouble());
      }

      if (!falseEastingStr.empty())
      {
         mollweid->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         mollweid->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return mollweid;
   }
   else if (projCode == "NT")
   {
      ossimNewZealandMapGridProjection* newzealand = new ossimNewZealandMapGridProjection();
      if (!datumCode.empty())
      {
         newzealand->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }
      return newzealand;
   }
   else if (projCode == "OC")
   {
      ossimString latOne = getValueFromMap(projectionMap, latitudeOneKey);
      ossimString lonOne = getValueFromMap(projectionMap, longitudeOneKey);
      ossimString latTwo = getValueFromMap(projectionMap, latitudeTwoKey);
      ossimString lonTwo = getValueFromMap(projectionMap, longitudeTwoKey);

      ossimObliqueMercatorProjection* oblique = new ossimObliqueMercatorProjection();

      ossimGpt origin1(latOne.toDouble(), lonOne.toDouble());
      ossimGpt origin2(latTwo.toDouble(), lonTwo.toDouble());

      if (!datumCode.empty())
      {
         oblique->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!origin1.isNan())
      {
         oblique->setCentralPoint1(origin1);
      }

      if (!origin2.isNan())
      {
         oblique->setCentralPoint2(origin2);
      }

      if (!scaleFactorStr.empty())
      {
         oblique->setScaleFactor(scaleFactorStr.toDouble());
      }

      if (!falseEastingStr.empty())
      {
         oblique->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         oblique->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return oblique;
   }
   else if (projCode == "PG")
   {
      ossimString originLat = getValueFromMap(projectionMap, "LatitudeTrueScale");
      ossimString originLon = getValueFromMap(projectionMap, "LongitudeDownFromPole");
      ossimGpt origin(originLat.toDouble(), originLon.toDouble());

      ossimPolarStereoProjection* polar = new ossimPolarStereoProjection();

      if (!datumCode.empty())
      {
         polar->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!origin.isNan())
      {
         polar->setOrigin(origin);
      }

      if (!falseEastingStr.empty())
      {
         polar->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         polar->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return polar;
   }
   else if (projCode == "PH")
   {
      ossimPolyconicProjection* polyconic = new ossimPolyconicProjection();

      ossimGpt origin(originLatitudeStr.toDouble(), centralMeridianStr.toDouble());

      if (!datumCode.empty())
      {
         polyconic->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!origin.isNan())
      {
         polyconic->setOrigin(origin);
      }

      if (!falseEastingStr.empty())
      {
         polyconic->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         polyconic->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return polyconic;
   }
   else if (projCode == "SA")
   {
      ossimSinusoidalProjection* sinusoidal = new ossimSinusoidalProjection();

      if (!datumCode.empty())
      {
         sinusoidal->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!centralMeridianStr.empty())
      {
         sinusoidal->setCentralMeridian(centralMeridianStr.toDouble());
      }

      if (!falseEastingStr.empty())
      {
         sinusoidal->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         sinusoidal->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return sinusoidal;
   }
   else if (projCode == "SD")
   {
      ossimStereographicProjection* stereo = new ossimStereographicProjection();

      ossimGpt origin(originLatitudeStr.toDouble(), centralMeridianStr.toDouble());

      if (!datumCode.empty())
      {
         stereo->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!origin.isNan())
      {
         stereo->setOrigin(origin);
      }

      if (!falseEastingStr.empty())
      {
         stereo->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         stereo->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return stereo;
   }
   else if (projCode == "TC")
   {
      ossimTransMercatorProjection* trans = new ossimTransMercatorProjection();

      ossimGpt origin(originLatitudeStr.toDouble(), centralMeridianStr.toDouble());

      if (!datumCode.empty())
      {
         trans->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!origin.isNan())
      {
         trans->setOrigin(origin);
      }

      if (!scaleFactorStr.empty())
      {
         trans->setScaleFactor(scaleFactorStr.toDouble());
      }

      if (!falseEastingStr.empty())
      {
         trans->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         trans->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return trans;
   }
   else if (projCode == "TX")
   {
      ossimTransCylEquAreaProjection* transCyl = new ossimTransCylEquAreaProjection();

      ossimGpt origin(originLatitudeStr.toDouble(), centralMeridianStr.toDouble());

      if (!datumCode.empty())
      {
         transCyl->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!origin.isNan())
      {
         transCyl->setOrigin(origin);
      }

      if (!scaleFactorStr.empty())
      {
         transCyl->setScaleFactor(scaleFactorStr.toDouble());
      }

      if (!falseEastingStr.empty())
      {
         transCyl->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         transCyl->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return transCyl;
   }
   else if (projCode == "VA")
   {
      ossimVanDerGrintenProjection* vander = new ossimVanDerGrintenProjection();

      if (!datumCode.empty())
      {
         vander->setDatum(ossimDatumFactory::instance()->create(datumCode));
      }

      if (!centralMeridianStr.empty())
      {
         vander->setCentralMeridian(centralMeridianStr.toDouble());
      }

      if (!falseEastingStr.empty())
      {
         vander->setFalseEasting(falseEastingStr.toDouble());
      }

      if (!falseNorthingStr.empty())
      {
         vander->setFalseNorthing(falseNorthingStr.toDouble());
      }

      return vander;
   }
   else 
   {
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "ossimGeoPdfReader: No projection information. \n"
            << std::endl;
      }
   }

   return NULL;
}
