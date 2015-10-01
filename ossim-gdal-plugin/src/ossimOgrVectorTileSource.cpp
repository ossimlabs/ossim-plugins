//*******************************************************************
//
// License:  See top level LICENSE.txt file.
//
// Author:  Mingjie Su
//
// Description:
//
// Contains class implementaiton for the class "ossimOgrVectorTileSource".
//
//*******************************************************************
//  $Id: ossimOgrVectorTileSource.cpp 1347 2010-08-23 17:03:06Z oscar.kramer $

#include <ossimOgrVectorTileSource.h>
#include <ossimOgcWktTranslator.h>
#include <ossimGdalType.h>

#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/imaging/ossimImageGeometry.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <ossim/projection/ossimEquDistCylProjection.h>

#include <ogr_api.h>

RTTI_DEF1(ossimOgrVectorTileSource,
          "ossimOgrVectorTileSource",
          ossimImageHandler);

static ossimOgcWktTranslator wktTranslator;
static ossimTrace traceDebug("ossimOgrVectorTileSource:debug");

class ossimOgrVectorLayerNode
{
public:
   ossimOgrVectorLayerNode(const ossimDrect& bounds)
      : theBoundingRect(bounds)
   {
   }
   ~ossimOgrVectorLayerNode()
   {
      theGeoImage.release();
      theGeoImage = 0;
   }

   bool intersects(const ossimDrect& rect)const
   {
      return theBoundingRect.intersects(rect);
   }

   bool intersects(double minX, double minY,
                   double maxX, double maxY)const
   {
      return theBoundingRect.intersects(ossimDrect(minX, minY, maxX, maxY));
   }
 
   void setGeoImage(ossimRefPtr<ossimImageGeometry> image)
   {
      theGeoImage = image;
   }

   ossimDrect theBoundingRect;  //world
   ossimRefPtr<ossimImageGeometry> theGeoImage;
};

//*******************************************************************
// Public Constructor:
//*******************************************************************
ossimOgrVectorTileSource::ossimOgrVectorTileSource()
   :ossimImageHandler(),
    theDataSource(0),
    theImageBound(),
    theBoundingExtent()
{
}

//*******************************************************************
// Destructor:
//*******************************************************************
ossimOgrVectorTileSource::~ossimOgrVectorTileSource()
{
   close();
}

//*******************************************************************
// Public Method:
//*******************************************************************
bool ossimOgrVectorTileSource::open()
{
   const char* MODULE = "ossimOgrVectorTileSource::open";

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_NOTICE)
         << MODULE << " entered...\nFile: " << theImageFile.c_str() << std::endl;
   }

   if(isOpen())
   {
      close();
   }
  
   if ( isOgrVectorDataSource() )
   {
      //---
      // Old interface removed in gdal. 
      // theDataSource = OGRSFDriverRegistrar::Open(theImageFile,
      //                                            false);
      //---
      theDataSource = (OGRDataSource*) OGROpen( theImageFile.c_str(), false, NULL );
      if (theDataSource)
      {
         int layerCount = theDataSource->GetLayerCount();
         theLayerVector.resize(layerCount);
         if(layerCount)
         {
            for(int i = 0; i < layerCount; ++i)
            {
               OGRLayer* layer = theDataSource->GetLayer(i);
               if(layer)
               {
                  OGRSpatialReference* spatialReference = layer->GetSpatialRef();
                  
                  if(!spatialReference)
                  {
                     if(traceDebug())
                     {
                        ossimNotify(ossimNotifyLevel_NOTICE)
                           << MODULE
                           << " No spatial reference given, assuming geographic"
                           << endl;
                     }
                  }
               }
               else
               {
                  if(traceDebug())
                  {
                     ossimNotify(ossimNotifyLevel_NOTICE)
                        
                        << MODULE
                        << " layer " << i << " is null." << endl;
                  }
               }
               
               if (layer)
               {
                  layer->GetExtent(&theBoundingExtent, true);
                  
                  ossimRefPtr<ossimProjection> proj = createProjFromReference(layer->GetSpatialRef());
                  ossimRefPtr<ossimImageGeometry> imageGeometry = 0;
                  bool isDefaultProjection = false;
                  if(proj.valid())
                  {
                     imageGeometry = new ossimImageGeometry(0, proj.get());
                  }
                  
                  ossimMapProjection* mapProj = 0;
                  if(imageGeometry.valid())
                  {
                     mapProj = PTR_CAST(ossimMapProjection, imageGeometry->getProjection());
                  }
                  else
                  {
                     mapProj = createDefaultProj();
                     imageGeometry = new ossimImageGeometry(0, mapProj);
                     isDefaultProjection = true;
                  }

                  if(mapProj)
                  {
                     ossimDrect rect(theBoundingExtent.MinX,
                                     theBoundingExtent.MaxY,
                                     theBoundingExtent.MaxX,
                                     theBoundingExtent.MinY,
                                     OSSIM_RIGHT_HANDED);
            
                     std::vector<ossimGpt> points;
                     if (isDefaultProjection || mapProj->isGeographic())
                     {
                        ossimGpt g1(rect.ul().y, rect.ul().x);
                        ossimGpt g2(rect.ur().y, rect.ur().x);
                        ossimGpt g3(rect.lr().y, rect.lr().x);
                        ossimGpt g4(rect.ll().y, rect.ll().x);

                        points.push_back(g1);
                        points.push_back(g2);
                        points.push_back(g3);
                        points.push_back(g4);
                     }
                     else
                     {
                        ossimGpt g1 = mapProj->inverse(rect.ul());
                        ossimGpt g2 = mapProj->inverse(rect.ur());
                        ossimGpt g3 = mapProj->inverse(rect.lr());
                        ossimGpt g4 = mapProj->inverse(rect.ll());

                        points.push_back(g1);
                        points.push_back(g2);
                        points.push_back(g3);
                        points.push_back(g4);
                     }
            
                     std::vector<ossimDpt> rectTmp;
                     rectTmp.resize(4);

                     for(std::vector<ossimGpt>::size_type index=0; index < 4; ++index)
                     {
                        imageGeometry->worldToLocal(points[(int)index], rectTmp[(int)index]);
                     }

                     ossimDrect rect2 = ossimDrect(rectTmp[0],
                                                   rectTmp[1],
                                                   rectTmp[2],
                                                   rectTmp[3]);

                     theLayerVector[i] = new ossimOgrVectorLayerNode(rect2);
                     theLayerVector[i]->setGeoImage(imageGeometry);

                     // Initialize the image geometry to the first layer's geometry:
                     if (i == 0)
                        theImageGeometry = imageGeometry;
                  }
               }
            }
               
         } // if(layerCount)
         else
         {
            OGR_DS_Destroy( theDataSource );
            theDataSource = 0;
         }
            
      } // Matches: if (theDataSource)
         
   } // Matches: if ( isOgrVectorDataSource() )
   
   return ( theDataSource ? true : false );
}

//*******************************************************************
// Public method:
//*******************************************************************
bool ossimOgrVectorTileSource::saveState(ossimKeywordlist& kwl,
                                         const char* prefix) const
{
   for (unsigned int i = 0; i < theLayerVector.size(); i++)
   {
      ossimRefPtr<ossimImageGeometry> imageGeometry = theLayerVector[i]->theGeoImage;
      if(theImageGeometry.valid())
      {
         theImageGeometry->saveState(kwl, prefix);
      }
   }

   return ossimImageHandler::saveState(kwl, prefix);
}

//*******************************************************************
// Public method:
//*******************************************************************
bool ossimOgrVectorTileSource::loadState(const ossimKeywordlist& kwl, const char* prefix)
{
   bool success = ossimImageHandler::loadState(kwl, prefix);
   if (success)
   {
      theImageGeometry = new ossimImageGeometry();
      success = theImageGeometry->loadState(kwl, prefix);
   }
   return success;
}

ossim_uint32 ossimOgrVectorTileSource::getImageTileWidth() const
{
   return 0;
}

ossim_uint32 ossimOgrVectorTileSource::getImageTileHeight() const
{
   return 0;
}

ossimRefPtr<ossimImageGeometry> ossimOgrVectorTileSource::getInternalImageGeometry() const
{
   return theImageGeometry;
}

void ossimOgrVectorTileSource::close()
{
   for(ossim_uint32 i = 0; i < theLayerVector.size(); ++i)
   {
      if(theLayerVector[i])
      {
         delete theLayerVector[i];
      }
   }
   
   theLayerVector.clear();
   
   if(theDataSource)
   {
      delete theDataSource;
      theDataSource = 0;
   }
}

ossimRefPtr<ossimImageData> ossimOgrVectorTileSource::getTile(
   const ossimIrect& /* tileRect */, ossim_uint32 /* resLevel */)
{
   return 0;
}

ossim_uint32 ossimOgrVectorTileSource::getNumberOfInputBands() const
{
   return 0;
}

ossim_uint32 ossimOgrVectorTileSource::getNumberOfOutputBands() const
{
   return 0;
}

ossim_uint32 ossimOgrVectorTileSource::getNumberOfLines(ossim_uint32 /* reduced_res_level */ ) const
{
   if(theImageBound.hasNans())
   {
      return theImageBound.ul().x;
   }
   
   return theImageBound.height();
}

ossim_uint32 ossimOgrVectorTileSource::getNumberOfSamples(ossim_uint32 /* reduced_res_level */ ) const
{
   if(theImageBound.hasNans())
   {
      return theImageBound.ul().y;
   }
   
   return theImageBound.width();
}

ossim_uint32 ossimOgrVectorTileSource::getNumberOfDecimationLevels() const
{
   return 32;
}

ossimIrect ossimOgrVectorTileSource::getImageRectangle(ossim_uint32 /* reduced_res_level */ ) const
{
   return theImageBound;
}

ossimRefPtr<ossimImageGeometry> ossimOgrVectorTileSource::getImageGeometry()
{
   return theImageGeometry;
}

ossimScalarType ossimOgrVectorTileSource::getOutputScalarType() const
{
   return OSSIM_SCALAR_UNKNOWN;
}

ossim_uint32 ossimOgrVectorTileSource::getTileWidth() const
{
   return 0;         
}

ossim_uint32 ossimOgrVectorTileSource::getTileHeight() const
{
   return 0;
}

bool ossimOgrVectorTileSource::isOpen()const
{
   return (theDataSource!=0);
}

bool ossimOgrVectorTileSource::isOgrVectorDataSource()const
{
   if (!theImageFile.empty())
   {
      if (theImageFile.before(":", 3).upcase() != "SDE" && 
          theImageFile.before(":", 4).upcase() != "GLTP" && 
          theImageFile.ext().downcase() != "mdb")
      {
         return false;
      }
   }
   else
   {
      return false;
   }
   
   return true;
}

double ossimOgrVectorTileSource::getNullPixelValue(ossim_uint32 /* band */)const
{
   return 0.0;
}

double ossimOgrVectorTileSource::getMinPixelValue(ossim_uint32 /* band */)const
{
   return 0.0;
}

double ossimOgrVectorTileSource::getMaxPixelValue(ossim_uint32 /* band */)const
{
   return 0.0;
}

bool ossimOgrVectorTileSource::setCurrentEntry(ossim_uint32 entryIdx)
{
   if(theLayerVector.size() > 0)
   {
      if (theLayerVector.size() > entryIdx)
      {
         theImageGeometry = 0;
         theImageBound.makeNan();
         theImageGeometry = theLayerVector[entryIdx]->theGeoImage;
         theImageBound = theLayerVector[entryIdx]->theBoundingRect;
         if (theImageGeometry.valid())
         {
            return true;
         }
      }
   }
   return false;
}

ossim_uint32 ossimOgrVectorTileSource::getNumberOfEntries() const
{
   return theLayerVector.size();
}

void ossimOgrVectorTileSource::getEntryList(std::vector<ossim_uint32>& entryList) const
{
   if (theLayerVector.size() > 0)
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

ossimProjection* ossimOgrVectorTileSource::createProjFromReference(OGRSpatialReference* reference)const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << "ossimOgrVectorTileSource::createProjFromReference:   entered........" << std::endl;
   }
   if(!reference)
   {
      if(traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG) << "ossimOgrVectorTileSource::createProjFromReference:   leaving........" << std::endl;
      }
      return 0;
   }
   char* wktString = 0;
   ossimKeywordlist kwl;
   reference->exportToWkt(&wktString);
   wktTranslator.toOssimKwl(wktString,
                            kwl);
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << "wktString === " << wktString << std::endl;
      ossimNotify(ossimNotifyLevel_DEBUG) << "KWL === " << kwl << std::endl;
   }
   OGRFree(wktString);
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << "ossimOgrVectorTileSource::createProjFromReference:   returning........" << std::endl;
   }
   return ossimProjectionFactoryRegistry::instance()->createProjection(kwl);
}

ossimMapProjection* ossimOgrVectorTileSource::createDefaultProj()
{
   ossimEquDistCylProjection* proj = new ossimEquDistCylProjection;
   
   ossim_float64 centerLat = (theBoundingExtent.MaxY + theBoundingExtent.MinY ) / 2.0;
   ossim_float64 centerLon = (theBoundingExtent.MaxX + theBoundingExtent.MinX ) / 2.0;
   ossim_float64 deltaLat  = theBoundingExtent.MaxY - theBoundingExtent.MinY;
   
   // Scale that gives 1024 pixel in the latitude direction.
   ossim_float64 scaleLat = deltaLat / 1024.0;
   ossim_float64 scaleLon = scaleLat*ossim::cosd(std::fabs(centerLat)); 
   ossimGpt origin(centerLat, centerLon, 0.0);
   ossimDpt scale(scaleLon, scaleLat);
   
   // Set the origin.
   proj->setOrigin(origin);
   
  // Set the tie point.
   proj->setUlGpt( ossimGpt(theBoundingExtent.MaxY,
                            theBoundingExtent.MinX) );
   
   // Set the scale.  Note this will handle computing meters per pixel.
   proj->setDecimalDegreesPerPixel(scale);
   
   return proj;
}
