//*******************************************************************
// License:  See top level LICENSE.txt file.
//
// Author:  Garrett Potts
//
//*******************************************************************
//  $Id: ossimGdalOgrVectorAnnotation.cpp 22897 2014-09-25 16:16:13Z dburken $

#include <ossimGdalOgrVectorAnnotation.h>
#include <ossimOgcWktTranslator.h>
#include <ossimGdalType.h>
#include <ossimOgcWktTranslator.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimPreferences.h>
#include <ossim/base/ossimColorProperty.h>
#include <ossim/base/ossimBooleanProperty.h>
#include <ossim/base/ossimTextProperty.h>
#include <ossim/base/ossimNumericProperty.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimGeoPolygon.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/imaging/ossimImageData.h>
#include <ossim/imaging/ossimAnnotationLineObject.h>
#include <ossim/imaging/ossimAnnotationMultiLineObject.h>
#include <ossim/imaging/ossimAnnotationPolyObject.h>
#include <ossim/imaging/ossimGeoAnnotationPolyObject.h>
#include <ossim/imaging/ossimGeoAnnotationPolyLineObject.h>
#include <ossim/imaging/ossimGeoAnnotationEllipseObject.h>
#include <ossim/imaging/ossimGeoAnnotationMultiEllipseObject.h>
#include <ossim/imaging/ossimGeoAnnotationMultiPolyObject.h>
#include <ossim/base/ossimPolyLine.h>
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimErrorContext.h>
#include <ossim/imaging/ossimRgbImage.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <ossim/projection/ossimProjection.h>
#include <ossim/projection/ossimEquDistCylProjection.h>
#include <ossim/projection/ossimImageProjectionModel.h>
#include <ossim/base/ossimUnitTypeLut.h>
#include <ossim/base/ossimUnitConversionTool.h>
#include <ossim/support_data/ossimFgdcXmlDoc.h>
#include <ogr_api.h>
#include <sstream>

RTTI_DEF2(ossimGdalOgrVectorAnnotation,
          "ossimGdalOgrVectorAnnotation",
          ossimAnnotationSource,
          ossimViewInterface);

static ossimOgcWktTranslator wktTranslator;
static ossimTrace traceDebug("ossimGdalOgrVectorAnnotation:debug");

static const char SHAPEFILE_COLORS_AUTO_KW[] =
   "shapefile_colors_auto";

static const char NORMALIZED_RGB_BRUSH_COLOR_KW[] =
   "shapefile_normalized_rgb_brush_color";

static const char NORMALIZED_RGB_PEN_COLOR_KW[] =
   "shapefile_normalized_rgb_pen_color";

static const char POINT_SIZE_KW[] =
   "shapefile_point_size";


bool doubleLess(double first, double second, double epsilon, bool orequal = false) 
{
   if (fabs(first - second) < epsilon) 
   {
      return (orequal);
   }
   return (first < second);
}

bool doubleGreater(double first, double second, double epsilon, bool orequal = false) 
{
   if (fabs(first - second) < epsilon) 
   {
      return (orequal);
   }
   return (first > second);
}

/** container class for rgb value. */
class ossimRgbColor
{
public:
   /** Default constructor (green) */
   ossimRgbColor() : r(1), g(255), b(1) {}

   /** Constructor that takes an rgb. */
   ossimRgbColor(ossim_uint8 r, ossim_uint8 g, ossim_uint8 b)
      : r(r), g(g), b(b) {}
   
   ossim_uint8 r;
   ossim_uint8 g;
   ossim_uint8 b;
};

// The index of the current color.
static ossim_uint32 currentAutoColorArrayIndex = 0;

// Array count of colors.
static const ossim_uint32 AUTO_COLOR_ARRAY_COUNT = 9;

// Static array to index for auto colors.
static const ossimRgbColor autoColorArray[AUTO_COLOR_ARRAY_COUNT] =
{
   ossimRgbColor(255,   1,   1), // red
   ossimRgbColor(  1, 255,   1), // green
   ossimRgbColor(  1,   1, 255), // blue
   
   ossimRgbColor(1,   255, 255),  // cyan
   ossimRgbColor(255,   1, 255),  // magenta
   ossimRgbColor(255, 255,   1),  // yellow

   ossimRgbColor(255, 165,   1), // orange
   ossimRgbColor(160,  32, 240), // purple
   ossimRgbColor(238, 130, 238) // violet
};

class ossimOgrGdalFeatureNode
{
public:
   ossimOgrGdalFeatureNode(long id,
                           const ossimDrect& rect)
      :theId(id),
       theBoundingRect(rect)
      {
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
   long theId;
   ossimDrect theBoundingRect;
};

class ossimOgrGdalLayerNode
{
public:
   ossimOgrGdalLayerNode(const ossimDrect& bounds)
      : theBoundingRect(bounds)
      {
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
   void getIdList(std::list<long>& idList,
                  const ossimDrect& aoi)const;
   std::vector<ossimOgrGdalFeatureNode> theFeatureList;

   ossimDrect theBoundingRect;
};
void ossimOgrGdalLayerNode::getIdList(std::list<long>& idList,
                                      const ossimDrect& aoi)const
{
   if(!intersects(aoi))
   {
      return;
   }
   if(theBoundingRect.completely_within(aoi))
   {
      for(ossim_uint32 i = 0; i < theFeatureList.size(); ++i)
      {
         idList.push_back(theFeatureList[i].theId);
      }
   }
   else
   {
      for(ossim_uint32 i = 0; i < theFeatureList.size(); ++i)
      {
         if(theFeatureList[i].intersects(aoi))
         {
            idList.push_back(theFeatureList[i].theId);
         }
      }
   }
}

ossimGdalOgrVectorAnnotation::ossimGdalOgrVectorAnnotation(ossimImageSource* inputSource)
   :ossimAnnotationSource(inputSource),
    ossimViewInterface(),
    theDataSource(0),
    theDriver(0),
    theFilename(),
    theBoundingExtent(),
    theLayersToRenderFlagList(),
    theLayerTable(),
    thePenColor(255,255,255),
    theBrushColor(255,255,255),
    theFillFlag(false),
    theThickness(1),
    thePointWidthHeight(1, 1),
    theBorderSize(0.0),
    theBorderSizeUnits(OSSIM_DEGREES),
    theImageBound(),
    theIsExternalGeomFlag(false),
    m_query(""),
    m_needPenColor(false),
    m_geometryDistance(0.0),
    m_geometryDistanceType(OSSIM_UNIT_UNKNOWN),
    m_layerName("")
{
   // Pick up colors from preference file if set.
   getDefaults();
   
   theObject = this;
   theImageBound.makeNan();
   ossimAnnotationSource::setNumberOfBands(3);
}

ossimGdalOgrVectorAnnotation::~ossimGdalOgrVectorAnnotation()
{
   ossimViewInterface::theObject = 0;
   close();
}

void ossimGdalOgrVectorAnnotation::close()
{
   deleteTables();
   if(theDataSource)
   {
      delete theDataSource;
      theDataSource = 0;
   }
   if (theImageGeometry.valid())
   {
      theImageGeometry = 0;
   }
}
ossimFilename ossimGdalOgrVectorAnnotation::getFilename()const
{
   return theFilename;
}

void ossimGdalOgrVectorAnnotation::setQuery(const ossimString& query)
{
  m_query = query;
  open(theFilename);
}

void ossimGdalOgrVectorAnnotation::setGeometryBuffer(ossim_float64 distance, ossimUnitType type)
{
   m_geometryDistance = distance;
   m_geometryDistanceType = type;
}

bool ossimGdalOgrVectorAnnotation::open(const ossimFilename& file)
{
   const char* MODULE = "ossimGdalOgrVectorAnnotation::open";
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_NOTICE) << MODULE << " entered...\nfile: " << file << "\n";
   }
   
   if(isOpen())
   {
      close();
   }
   m_layerNames.clear();
   if(file == "") return false;

#if 0 /* Commented out but please leave until I test. drb - 15 July 2011 */
   ossimString ext = file.ext().downcase();
   if ( ext != "shp" )
   {
      //---
      // OGRSFDriverRegistrar::Open very touchy and crashes if given a file it cannot handle.
      // Only allow one type of extension to open file.
      //---
      return false;
   }
#endif
   
   // theDataSource = OGRSFDriverRegistrar::Open( file.c_str(), false, &theDriver );
   theDataSource = (OGRDataSource*) OGROpen( file.c_str(), false, NULL );
   if ( theDataSource )
   {
      theDriver  = (OGRSFDriver*) theDataSource->GetDriver();
   }
   
   if ( !theDataSource || !theDriver )
   {
      if(traceDebug())
      {
         ossimNotify(ossimNotifyLevel_NOTICE) << "OGRSFDriverRegistrar::Open failed...\n";
      }
      return false;   
   }
   
   // Capture the file.  Need here for loadExternalGeometryFile method.
   theFilename = file;

   // This will set theViewProjection if "file.geom" is present and valid.
   loadExternalGeometryFile();

   // This will load external pen, brush and point attributes.
   loadOmdFile();
   
   theLayersToRenderFlagList.clear();
   vector<ossimGpt> points;
   if(isOpen())
   {
      int i = 0;
      int layerCount = 0;
      if (!m_query.empty())
      {
        layerCount = 1;
      }
      else if (!m_layerName.empty())
      {
         layerCount = 1;
      }
      else
      {
        layerCount = theDataSource->GetLayerCount();
      }
      
      bool successfulTest = true;
      if(layerCount)
      {
         theLayersToRenderFlagList.resize(layerCount);
         for(i = 0; (i < layerCount)&&successfulTest; ++i)
         {
            OGRLayer* layer = NULL;
            if (!m_query.empty())
            {
              layer = theDataSource->ExecuteSQL(m_query, NULL, NULL);
            }
            else if (!m_layerName.empty())
            {
               layer = theDataSource->GetLayerByName(m_layerName.c_str());
            }
            else
            {
              layer = theDataSource->GetLayer(i);
            }
            
            if(layer)
            {
               OGRSpatialReference* spatialReference = layer->GetSpatialRef();
               theLayersToRenderFlagList[i] = true;
               m_layerNames.push_back(ossimString(layer->GetLayerDefn()->GetName()));
               
               if(!spatialReference)
               {
                  //try xml file
                  if (!theImageGeometry.valid())
                  {
                     loadExternalImageGeometryFromXml();
                  }

                  if(traceDebug())
                  {
                     ossimNotify(ossimNotifyLevel_NOTICE)
                     << MODULE
                     << " No spatial reference given, assuming geographic"
                     << endl;
                  }
               }
               else if(spatialReference->IsLocal())
               {
                  if(traceDebug())
                  {
                     ossimNotify(ossimNotifyLevel_NOTICE)
                     << MODULE
                     << " Only geographic vectors and  projected vectors "
                     << "are supported, layer " << i << " is local" << endl;
                  }
                  successfulTest = false;
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
               successfulTest = false;
            }
            
            if(successfulTest&&layer)
            {
               if (!theImageGeometry.valid())
               {
                  ossimRefPtr<ossimProjection> proj = createProjFromReference(layer->GetSpatialRef());
                  if(proj.valid())
                  {
                     theImageGeometry = new ossimImageGeometry(0, proj.get());
                  }
               }
               ossimMapProjection* mapProj = 0;
               if(theImageGeometry.valid())
               {
                 mapProj = PTR_CAST(ossimMapProjection, theImageGeometry->getProjection());
               }
               if (!mapProj)
               {
                  theImageGeometry = 0; 
               }
               
               if(i == 0)
               {
                  layer->GetExtent(&theBoundingExtent, true);
                  if(mapProj)
                  {
                     if (layer->GetSpatialRef())
                     {
                        double unitValue = layer->GetSpatialRef()->GetLinearUnits();
                        theBoundingExtent.MinX = theBoundingExtent.MinX * unitValue;
                        theBoundingExtent.MaxY = theBoundingExtent.MaxY * unitValue;
                        theBoundingExtent.MaxX = theBoundingExtent.MaxX * unitValue;
                        theBoundingExtent.MinY = theBoundingExtent.MinY * unitValue;
                     }

                     ossimDrect rect(theBoundingExtent.MinX,
                                     theBoundingExtent.MaxY,
                                     theBoundingExtent.MaxX,
                                     theBoundingExtent.MinY,
                                     OSSIM_RIGHT_HANDED);
                     
                     ossimGpt g1 = mapProj->inverse(rect.ul());
                     ossimGpt g2 = mapProj->inverse(rect.ur());
                     ossimGpt g3 = mapProj->inverse(rect.lr());
                     ossimGpt g4 = mapProj->inverse(rect.ll());
                     ossimDrect rect2 = ossimDrect(ossimDpt(g1),
                                                   ossimDpt(g2),
                                                   ossimDpt(g3),
                                                   ossimDpt(g4));
                     
                     theBoundingExtent.MinX = rect2.ul().x;
                     theBoundingExtent.MinY = rect2.ul().y;
                     theBoundingExtent.MaxX = rect2.lr().x;
                     theBoundingExtent.MaxY = rect2.lr().y;

                     //insert the bounding values to points to convert to bounding rect
                     points.push_back(g1);
                     points.push_back(g2);
                     points.push_back(g3);
                     points.push_back(g4);
                  }
               }
               else
               {
                  OGREnvelope extent;
                  layer->GetExtent(&extent, true);
                  if(mapProj)
                  {
                     ossimDrect rect(extent.MinX,
                                     extent.MaxY,
                                     extent.MaxX,
                                     extent.MinY,
                                     OSSIM_RIGHT_HANDED);
                  
                     ossimGpt g1 = mapProj->inverse(rect.ul());
                     ossimGpt g2 = mapProj->inverse(rect.ur());
                     ossimGpt g3 = mapProj->inverse(rect.lr());
                     ossimGpt g4 = mapProj->inverse(rect.ll());
                     ossimDrect rect2 = ossimDrect(ossimDpt(g1),
                                                   ossimDpt(g2),
                                                   ossimDpt(g3),
                                                   ossimDpt(g4));
                     extent.MinX = rect2.ul().x;
                     extent.MinY = rect2.ul().y;
                     extent.MaxX = rect2.lr().x;
                     extent.MaxY = rect2.lr().y;

                     //compare the current values of points with the first layer to
                     //get the MBR of the datasource
                     if (points.size() == 4)
                     {
                        if (doubleLess(g1.lon, points[0].lon, 0.0001))
                        {
                           points[0].lon = g1.lon;
                        }
                        if (doubleGreater(g1.lat, points[0].lat, 0.0001))
                        {
                           points[0].lat = g1.lat;
                        }
                        if (doubleGreater(g2.lon, points[1].lon, 0.0001))
                        {
                           points[1].lon = g2.lon;
                        }
                        if (doubleGreater(g2.lat, points[1].lat, 0.0001))
                        {
                           points[1].lat = g2.lat;
                        }
                        if (doubleGreater(g3.lon, points[2].lon, 0.0001))
                        {
                           points[2].lon = g3.lon;
                        }
                        if (doubleLess(g3.lat, points[2].lat, 0.0001))
                        {
                           points[2].lat = g3.lat;
                        }
                        if (doubleLess(g4.lon, points[3].lon, 0.0001))
                        {
                           points[3].lon = g4.lon;
                        }
                        if (doubleLess(g4.lat, points[3].lat, 0.0001))
                        {
                           points[3].lat = g4.lat;
                        }
                     }
                  }
                  theBoundingExtent.MinX = std::min(extent.MinX,
                                                    theBoundingExtent.MinX);
                  theBoundingExtent.MinY = std::min(extent.MinY,
                                                    theBoundingExtent.MinY);
                  theBoundingExtent.MaxX = std::max(extent.MaxX,
                                                    theBoundingExtent.MaxX);
                  theBoundingExtent.MaxY = std::max(extent.MaxY,
                                                    theBoundingExtent.MaxY);
               }
            }

            //if an OGRLayer pointer representing a results set from the query, this layer is 
            //in addition to the layers in the data store and must be destroyed with 
            //OGRDataSource::ReleaseResultSet() before the data source is closed (destroyed).
            if (!m_query.empty() && layer != NULL)
            {
              theDataSource->ReleaseResultSet(layer);
            }  
         }
      }
      if(!successfulTest)
      {
         delete theDataSource;
         theDataSource = NULL;
         theLayersToRenderFlagList.clear();
         
         return false;
      }
   }
   if(traceDebug())
   {
      CLOG << "Extents = "
           << theBoundingExtent.MinX << ", "
           << theBoundingExtent.MinY << ", "
           << theBoundingExtent.MaxX << ", "
           << theBoundingExtent.MaxY << endl;
   }
   if(!theImageGeometry.valid())
   {
      computeDefaultView();
   }
   else
   {
      verifyViewParams();
   }

   //initialize the bounding rect
   initializeBoundingRec(points);
  
   //only initializeTables when need to draw features. This eliminate the memory allocate problem
   //when only do ossim-info for a large shape file
   //initializeTables();

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " DEBUG:"
         << "\ntheViewProjection:"
         << endl;
      if(theImageGeometry.valid())
      {
         theImageGeometry->print(ossimNotify(ossimNotifyLevel_DEBUG));
      }
      print(ossimNotify(ossimNotifyLevel_DEBUG));
   }

   return (theDataSource!=NULL);
}

void ossimGdalOgrVectorAnnotation::initializeBoundingRec(vector<ossimGpt> points)
{
   theImageBound.makeNan();

   //if the projection is default or geographic, uses the bounding of the OGR Layer
   if (points.size() == 0)
   {
      points.push_back(ossimGpt(theBoundingExtent.MaxY, theBoundingExtent.MinX));
      points.push_back(ossimGpt(theBoundingExtent.MaxY, theBoundingExtent.MaxX));
      points.push_back(ossimGpt(theBoundingExtent.MinY, theBoundingExtent.MaxX));
      points.push_back(ossimGpt(theBoundingExtent.MinY, theBoundingExtent.MinX));
   }

   if(theImageGeometry.valid())
   {
      std::vector<ossimDpt> rectTmp;
      rectTmp.resize(points.size());
      for(std::vector<ossimGpt>::size_type index=0; index < points.size(); ++index)
      {
         theImageGeometry->worldToLocal(points[(int)index], rectTmp[(int)index]);
      }

      if (rectTmp.size() > 3)
      {
         ossimDrect rect2 = ossimDrect(rectTmp[0],
            rectTmp[1],
            rectTmp[2],
            rectTmp[3]);

         theImageBound = rect2;
      }
   }
}

bool ossimGdalOgrVectorAnnotation::setView(ossimObject* baseObject)
{
   bool result = false;

   if(baseObject)
   {
      // Test for projection...
      ossimProjection* p = PTR_CAST(ossimProjection, baseObject);
      if (p)
      {
         if(!theImageGeometry)
         {
            theImageGeometry = new ossimImageGeometry(0, p);
         }
         else
         {
            theImageGeometry->setProjection(p);
         }
         
         // Reproject the points to the current new projection.
         transformObjectsFromView();
         result = true;
         
      }
      else
      {
         ossimImageGeometry* geom = dynamic_cast<ossimImageGeometry*> (baseObject);
         if(geom)
         {
            theImageGeometry = geom;
            // Reproject the points to the current new projection.
            transformObjectsFromView();
            result = true;
         }
      }
   } // if (baseObject)

   return result;
}

ossimObject* ossimGdalOgrVectorAnnotation::getView()
{
   return theImageGeometry.get();
}

//! Returns the image geometry object associated with this tile source or NULL if non defined.
//! The geometry contains full-to-local image transform as well as projection (image-to-world)
ossimRefPtr<ossimImageGeometry> ossimGdalOgrVectorAnnotation::getImageGeometry() const
{
   return theImageGeometry;
}

void ossimGdalOgrVectorAnnotation::computeDefaultView()
{
   if (theImageGeometry.valid())
      return;

   if(!isOpen())
      return;

   // double horizontal = fabs(theBoundingExtent.MinX - theBoundingExtent.MaxX);
   // double vertical   = fabs(theBoundingExtent.MinY - theBoundingExtent.MaxY);

   //    if((horizontal > 0.0) && (vertical > 0.0))
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

   // Capture the projection.
   theImageGeometry = new ossimImageGeometry(0, proj);
}

ossimIrect ossimGdalOgrVectorAnnotation::getBoundingRect(ossim_uint32 /* resLevel */ )const
{
   return theImageBound;
}

void ossimGdalOgrVectorAnnotation::computeBoundingRect()
{
   std::multimap<long, ossimAnnotationObject*>::iterator iter = theFeatureCacheTable.begin();
   
   theImageBound.makeNan();
   while(iter != theFeatureCacheTable.end())
   {
      ossimGeoAnnotationObject* obj = PTR_CAST(ossimGeoAnnotationObject,
                                               (*iter).second);
      
      if(obj)
      {
         ossimDrect rect;
         obj->getBoundingRect(rect);
         
         if(theImageBound.hasNans())
         {
            theImageBound = rect;
         }
         else
         {
            if(!rect.hasNans())
            {
               theImageBound = theImageBound.combine(rect);
            }
         }
      }

      ++iter;
   }

   theImageBound.stretchOut();
}

void ossimGdalOgrVectorAnnotation::drawAnnotations(
   ossimRefPtr<ossimImageData> tile)
{
   if (theFeatureCacheTable.size() == 0)
   {
      initializeTables();
   }

   if( theImageGeometry.valid())
   {
      list<long> featuresToRender;
      ossimIrect tileRect = tile->getImageRectangle();
      
      getFeatures(featuresToRender, tileRect);
      
      list<long>::iterator current = featuresToRender.begin();
      
      ossimRefPtr<ossimRgbImage> image = new ossimRgbImage;
      
      image->setCurrentImageData(tile);
      vector<ossimAnnotationObject*> objectList;
      
      while(current!=featuresToRender.end())
      {
         getFeature(objectList, *current);
         ++current;
      }
      
      for(ossim_uint32 i = 0; i < objectList.size();++i)
      {
         objectList[i]->draw(*image.get());

        if (theFillFlag && m_needPenColor) //need to draw both the brush and line (pen) for a polygon
        {
          ossimObject* objectDup = objectList[i]->dup();
          ossimGeoAnnotationPolyObject* polyObject = PTR_CAST(ossimGeoAnnotationPolyObject, objectDup);
          if (polyObject)//check if it is the polygon object
          {
            polyObject->setColor(thePenColor.getR(), thePenColor.getG(), thePenColor.getB());
            polyObject->setThickness(theThickness);
            polyObject->setFillFlag(false);
            polyObject->draw(*image.get());
          }
          delete objectDup;
        }
      }
      
      tile->validate();
   }
}

void ossimGdalOgrVectorAnnotation::updateAnnotationSettings()
{
   std::multimap<long, ossimAnnotationObject*>::iterator iter = theFeatureCacheTable.begin();

   while(iter != theFeatureCacheTable.end())
   {
      iter->second->setThickness(theThickness);

      iter->second->setColor(thePenColor.getR(),
                             thePenColor.getG(),
                             thePenColor.getB());
      
      if(PTR_CAST(ossimGeoAnnotationPolyObject, iter->second))
      {
         ossimGeoAnnotationPolyObject* poly =
            (ossimGeoAnnotationPolyObject*)(iter->second);
         poly->setFillFlag(theFillFlag);
      }
      else if(PTR_CAST(ossimGeoAnnotationMultiPolyObject, iter->second))
      {
         ossimGeoAnnotationMultiPolyObject* poly =
            (ossimGeoAnnotationMultiPolyObject*)(iter->second);
         poly->setFillFlag(theFillFlag);
      }
      else if(PTR_CAST(ossimGeoAnnotationEllipseObject, iter->second))
      {
         ossimGeoAnnotationEllipseObject* ell = (ossimGeoAnnotationEllipseObject*)(iter->second);

         ell->setWidthHeight(thePointWidthHeight);
         ell->setFillFlag(theFillFlag);
         ell->transform(theImageGeometry.get());
      }
      if(theFillFlag)
      {
         iter->second->setColor(theBrushColor.getR(),
                                theBrushColor.getG(),
                                theBrushColor.getB());
      }
      ++iter;
   }
}

void ossimGdalOgrVectorAnnotation::setProperty(ossimRefPtr<ossimProperty> property)
{
   if(!property.valid()) return;

   ossimString name  = property->getName();
   ossimString value = property->valueToString();

   if(name == ossimKeywordNames::PEN_COLOR_KW)
   {
      int r;  
      int g;  
      int b;
      std::istringstream in(value);
      in >> r >> g >> b;
      thePenColor.setR((unsigned char)r);
      thePenColor.setG((unsigned char)g);
      thePenColor.setB((unsigned char)b);
      updateAnnotationSettings();
   }
   else if(name == ossimKeywordNames::BRUSH_COLOR_KW)
   {
      int r;  
      int g;  
      int b;
      std::istringstream in(value);
      in >> r >> g >> b;
      theBrushColor.setR((unsigned char)r);
      theBrushColor.setG((unsigned char)g);
      theBrushColor.setB((unsigned char)b);
      updateAnnotationSettings();
   }
   else if(name == ossimKeywordNames::FILL_FLAG_KW)
   {
      theFillFlag = value.toBool();
      updateAnnotationSettings();
   }
   else if(name == ossimKeywordNames::THICKNESS_KW)
   {
      setThickness(value.toInt32());
      updateAnnotationSettings();
   }
   else if(name == ossimKeywordNames::BORDER_SIZE_KW)
   {
   }
   else if(name == ossimKeywordNames::POINT_WIDTH_HEIGHT_KW)
   {
      std::istringstream in(value);
      in >> thePointWidthHeight.x;
      in >> thePointWidthHeight.y;
      updateAnnotationSettings();
   }
   else
   {
      ossimAnnotationSource::setProperty(property);
   }
}

ossimRefPtr<ossimProperty> ossimGdalOgrVectorAnnotation::getProperty(const ossimString& name)const
{
   ossimRefPtr<ossimProperty> result;
   if(name == ossimKeywordNames::PEN_COLOR_KW)
   {
      result = new ossimColorProperty(name,
                                      thePenColor);
      result->setCacheRefreshBit();
   }
   else if(name == ossimKeywordNames::BRUSH_COLOR_KW)
   {
      result = new ossimColorProperty(name,
                                      theBrushColor);
      result->setCacheRefreshBit();
   }
   else if(name == ossimKeywordNames::FILL_FLAG_KW)
   {
      result = new ossimBooleanProperty(name,
                                        theFillFlag);
      result->setCacheRefreshBit();
      
   }
   else if(name == ossimKeywordNames::THICKNESS_KW)
   {
      ossimNumericProperty* prop =
         new ossimNumericProperty(name,
                                  ossimString::toString(getThickness()),
                                  1.0,
                                  255.0);
      prop->setNumericType(ossimNumericProperty::ossimNumericPropertyType_INT);
      result = prop;
      result->setFullRefreshBit();
   }
   else if(name == ossimKeywordNames::BORDER_SIZE_KW)
   {
   }
   else if(name == ossimKeywordNames::POINT_WIDTH_HEIGHT_KW)
   {
      result = new ossimTextProperty(name,
                                     ossimString::toString(thePointWidthHeight.x) +
                                     " " +
                                     ossimString::toString(thePointWidthHeight.y));
      result->setFullRefreshBit();
   }
   else
   {
      result = ossimAnnotationSource::getProperty(name);
   }
   
   return result;
}

void ossimGdalOgrVectorAnnotation::getPropertyNames(std::vector<ossimString>& propertyNames)const
{
   
   propertyNames.push_back(ossimKeywordNames::PEN_COLOR_KW);
   propertyNames.push_back(ossimKeywordNames::BRUSH_COLOR_KW);
   propertyNames.push_back(ossimKeywordNames::FILL_FLAG_KW);
   propertyNames.push_back(ossimKeywordNames::THICKNESS_KW);
   propertyNames.push_back(ossimKeywordNames::BORDER_SIZE_KW);
   propertyNames.push_back(ossimKeywordNames::POINT_WIDTH_HEIGHT_KW);
}


bool ossimGdalOgrVectorAnnotation::saveState(ossimKeywordlist& kwl,
                                             const char* prefix)const
{
   ossimString s;
   
   kwl.add(prefix,
           ossimKeywordNames::FILENAME_KW,
           theFilename.c_str(),
           true);


   s = ossimString::toString((int)thePenColor.getR()) + " " +
       ossimString::toString((int)thePenColor.getG()) + " " +
       ossimString::toString((int)thePenColor.getB());
   
   kwl.add(prefix,
           ossimKeywordNames::PEN_COLOR_KW,
           s.c_str(),
           true);

   s = ossimString::toString((int)theBrushColor.getR()) + " " +
       ossimString::toString((int)theBrushColor.getG()) + " " +
       ossimString::toString((int)theBrushColor.getB());
   
   kwl.add(prefix,
           ossimKeywordNames::BRUSH_COLOR_KW,
           s.c_str(),
           true);

   kwl.add(prefix,
           ossimKeywordNames::FILL_FLAG_KW,
           (int)theFillFlag,
           true);

   kwl.add(prefix,
           ossimKeywordNames::THICKNESS_KW,
           getThickness(),
           true);

   ossimString border;
   border = ossimString::toString(theBorderSize);
   border += " degrees";
   kwl.add(prefix,
           ossimKeywordNames::BORDER_SIZE_KW,
           border,
           true);

   kwl.add(prefix,
           ossimString(ossimString(ossimKeywordNames::BORDER_SIZE_KW)+
                       "."+
                       ossimKeywordNames::UNITS_KW).c_str(),
           ossimUnitTypeLut::instance()->getEntryString(theBorderSizeUnits),
           true);
   
   s = ossimString::toString((int)thePointWidthHeight.x) + " " +
       ossimString::toString((int)thePointWidthHeight.y);
   
   kwl.add(prefix,
           ossimKeywordNames::POINT_WIDTH_HEIGHT_KW,
           s.c_str(),
           true);

   if (!m_query.empty())
   {
     kwl.add(prefix,
       ossimKeywordNames::QUERY_KW,
       m_query.c_str(),
       true);
   }
   
   if(theImageGeometry.valid())
   {
      ossimString newPrefix = prefix;
      newPrefix += "view_proj.";
      theImageGeometry->saveState(kwl,
                                   newPrefix.c_str());
   }
   
   return ossimAnnotationSource::saveState(kwl, prefix);
}

bool ossimGdalOgrVectorAnnotation::loadState(const ossimKeywordlist& kwl,
                                             const char* prefix)
{
   const char* filename    = kwl.find(prefix, ossimKeywordNames::FILENAME_KW);
   const char* penColor    = kwl.find(prefix, ossimKeywordNames::PEN_COLOR_KW);
   const char* brushColor  = kwl.find(prefix, ossimKeywordNames::BRUSH_COLOR_KW);
   const char* fillFlag    = kwl.find(prefix, ossimKeywordNames::FILL_FLAG_KW);
   const char* thickness   = kwl.find(prefix, ossimKeywordNames::THICKNESS_KW);
   const char* pointWh     = kwl.find(prefix, ossimKeywordNames::POINT_WIDTH_HEIGHT_KW);
   const char* border_size = kwl.find(prefix, ossimKeywordNames::BORDER_SIZE_KW);
   const char* query       = kwl.find(prefix, ossimKeywordNames::QUERY_KW);
   
   deleteTables();
   if(thickness)
   {
      setThickness(ossimString(thickness).toInt32());
   }
   
   if(penColor)
   {
      int r = 0;
      int g = 0;
      int b = 0;
      ossimString penColorStr = ossimString(penColor);
      if (penColorStr.split(",").size() == 3)
      {
         r = penColorStr.split(",")[0].toInt();
         g = penColorStr.split(",")[1].toInt();
         b = penColorStr.split(",")[2].toInt();
         if (r == 0 && g == 0 && b == 0)
         {
            r = 1;
            g = 1;
            b = 1;
         }
      }
      thePenColor = ossimRgbVector((ossim_uint8)r, (ossim_uint8)g, (ossim_uint8)b);
      m_needPenColor = true;
   }

   if(brushColor)
   {
      int r = 0;
      int g = 0;
      int b = 0;
      ossimString brushColorStr = ossimString(brushColor);
      if (brushColorStr.split(",").size() == 3)
      {
         r = brushColorStr.split(",")[0].toInt();
         g = brushColorStr.split(",")[1].toInt();
         b = brushColorStr.split(",")[2].toInt();
         if (r == 0 && g == 0 && b == 0)
         {
            r = 1;
            g = 1;
            b = 1;
         }
      }

      theBrushColor = ossimRgbVector((ossim_uint8)r, (ossim_uint8)g, (ossim_uint8)b);
   }
   if(pointWh)
   {
      double w, h;
      std::istringstream s(pointWh);
      s>>w>>h;
      thePointWidthHeight = ossimDpt(w, h);
   }
   
   if(fillFlag)
   {
      theFillFlag = ossimString(fillFlag).toBool();
   }
   theBorderSize = 0.0;
   if(border_size)
   {
      theBorderSize = ossimString(border_size).toDouble();
      ossimString unitPrefix = ossimString(prefix) +
                               ossimKeywordNames::BORDER_SIZE_KW +
                               ossimString(".");
      
      theBorderSizeUnits = (ossimUnitType)ossimUnitTypeLut::instance()->
         getEntryNumber(kwl,
                                                                                unitPrefix.c_str());
      if(theBorderSizeUnits != OSSIM_UNIT_UNKNOWN)
      {
         ossimUnitConversionTool unitConvert(theBorderSize,
                                             theBorderSizeUnits);
         
         theBorderSize      = unitConvert.getValue(OSSIM_DEGREES);
         theBorderSizeUnits = OSSIM_DEGREES;
      }
      else // assume degrees
      {
         theBorderSizeUnits = OSSIM_DEGREES;
      }
   }
   else
   {
      theBorderSize      = 0.0;
      theBorderSizeUnits = OSSIM_DEGREES;
   }
   
   if(filename)
   {
      if(!open(ossimFilename(filename)))
      {
         return false;
      }
   }

   if (query)
   {
     setQuery(query);
   }

   bool status = ossimAnnotationSource::loadState(kwl, prefix);
   
   initializeTables();

   return status;
}

std::ostream& ossimGdalOgrVectorAnnotation::print(std::ostream& out) const
{
   out << "ossimGdalOgrVectorAnnotation::print"
       << "\ntheLayersToRenderFlagList.size(): "
       << theLayersToRenderFlagList.size()
       << "\ntheLayerTable.size(): " << theLayerTable.size();

   ossim_uint32 i;
   for(i=0; i<theLayersToRenderFlagList.size(); ++i)
   {
      out << "layer[" << i << "]:"
          << (theLayersToRenderFlagList[i]?"enabled":"disabled")
          << std::endl;
   }
   return ossimAnnotationSource::print(out);
}

void ossimGdalOgrVectorAnnotation::transformObjectsFromView()
{
   if (theImageGeometry.valid())
   {
      if (theFeatureCacheTable.size() == 0)
      {
         initializeTables();
      }
      std::multimap<long, ossimAnnotationObject*>::iterator iter =
         theFeatureCacheTable.begin();
      
      while(iter != theFeatureCacheTable.end())
      {
         ossimGeoAnnotationObject* obj = PTR_CAST(ossimGeoAnnotationObject,
                                                  (*iter).second);
         
         if(obj&&theImageGeometry.valid())
         {
            obj->transform(theImageGeometry.get());
         }
         ++iter;
      }
      computeBoundingRect();
   }
}

void ossimGdalOgrVectorAnnotation::getFeatures(std::list<long>& result,
                                               const ossimIrect& rect)
{
   if (isOpen())
   {
      ossimGpt gp1;
      ossimGpt gp2;
      ossimGpt gp3;
      ossimGpt gp4;
      ossimDpt dp1 = rect.ul();
      ossimDpt dp2 = rect.ur();
      ossimDpt dp3 = rect.lr();
      ossimDpt dp4 = rect.ll();
      
      if (theImageGeometry.valid())
      {
         theImageGeometry->localToWorld(dp1, gp1);
         theImageGeometry->localToWorld(dp2, gp2);
         theImageGeometry->localToWorld(dp3, gp3);
         theImageGeometry->localToWorld(dp4, gp4);

         double maxX = std::max( gp1.lond(), std::max( gp2.lond(), std::max(gp3.lond(), gp4.lond())));
         double minX = std::min( gp1.lond(), std::min( gp2.lond(), std::min(gp3.lond(), gp4.lond())));
         double maxY = std::max( gp1.latd(), std::max( gp2.latd(), std::max(gp3.latd(), gp4.latd())));
         double minY = std::min( gp1.latd(), std::min( gp2.latd(), std::min(gp3.latd(), gp4.latd())));
         
         ossimDrect bounds(minX, minY, maxX, maxY);
         
         for(ossim_uint32 layerI = 0;
             layerI < theLayersToRenderFlagList.size();
             ++layerI)
         {
            if(theLayersToRenderFlagList[layerI])
            {
               if(theLayerTable[layerI])
               {
                  theLayerTable[layerI]->getIdList(result, bounds);
               }
            }
         }
      }
   }
}

void ossimGdalOgrVectorAnnotation::initializeTables()
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << "ossimGdalOgrVectorAnnotation::initializeTables(): entered.........." << std::endl;
   }
   if(theLayerTable.size())
   {
      deleteTables();
   }

   if(isOpen())
   {
      int upper = theLayersToRenderFlagList.size();
      theLayerTable.resize(upper);

      for(int i = 0; i < upper; ++i)
      {  
         if(theLayersToRenderFlagList[i])
         {
            OGREnvelope extent;
            OGRLayer* layer = NULL;
            if (!m_query.empty())
            {
              layer = theDataSource->ExecuteSQL(m_query, NULL, NULL);
            }
            else if (!m_layerName.empty())
            {
               layer = theDataSource->GetLayerByName(m_layerName.c_str());
            }
            else
            {
              layer = theDataSource->GetLayer(i);
            }

            ossimRefPtr<ossimProjection> proj;
            if (theIsExternalGeomFlag&&theImageGeometry.valid())
            {
               proj = theImageGeometry->getProjection();
            }
            else
            {
               proj = createProjFromReference(layer->GetSpatialRef());
            }

            ossimMapProjection* mapProj = PTR_CAST(ossimMapProjection, proj.get());
            
            layer->ResetReading();
            layer->GetExtent(&extent, true);
            layer->ResetReading();

            OGRFeature* feature = NULL;
            if(mapProj)
            {
               ossimDrect rect(extent.MinX,
                               extent.MaxY,
                               extent.MaxX,
                               extent.MinY,
                               OSSIM_RIGHT_HANDED);
               ossimGpt g1 = mapProj->inverse(rect.ul());
               ossimGpt g2 = mapProj->inverse(rect.ur());
               ossimGpt g3 = mapProj->inverse(rect.lr());
               ossimGpt g4 = mapProj->inverse(rect.ll());

               ossimDrect rect2 = ossimDrect(ossimDpt(g1),
                                            ossimDpt(g2),
                                            ossimDpt(g3),
                                            ossimDpt(g4));
               theLayerTable[i] = new ossimOgrGdalLayerNode(rect2);
            }
            else
            {
               theLayerTable[i] = new ossimOgrGdalLayerNode(ossimDrect(extent.MinX,
                                                                       extent.MinY,
                                                                       extent.MaxX,
                                                                       extent.MaxY));
            }
           
            while( (feature = layer->GetNextFeature()) != NULL)
            {
                  if(feature)
                  {
                     OGRGeometry* geom = feature->GetGeometryRef();
                     
                     if(geom)
                     {
                        switch(geom->getGeometryType())
                        {
                           case wkbMultiPoint:
                           case wkbMultiPoint25D:
                           {
                              if(traceDebug())
                              {
                                 ossimNotify(ossimNotifyLevel_DEBUG) << "Loading multi point" << std::endl;
                              }
                              loadMultiPoint(feature->GetFID(),
                                             (OGRMultiPoint*)geom,
                                             mapProj);
                              break;
                           }
                           case wkbPolygon25D:
                           case wkbPolygon:
                           {
                              if(traceDebug())
                              {
                                 ossimNotify(ossimNotifyLevel_DEBUG) << "Loading polygon" << std::endl;
                              }
                              if (m_geometryDistance > 0.0)
                              {
                                 OGRPolygon* poly = (OGRPolygon*)geom;
                                 OGRLinearRing* ring = poly->getExteriorRing();
                                 int numPoints = ring->getNumPoints();
                                 OGRGeometry* bufferGeom = geom->Buffer(m_geometryDistance, numPoints);
                                 loadPolygon(feature->GetFID(),
                                    (OGRPolygon*)bufferGeom,
                                    mapProj);
                              }
                              else
                              {
                                 loadPolygon(feature->GetFID(),
                                    (OGRPolygon*)geom,
                                    mapProj);
                              }

                              break;
                           }
                           case wkbLineString25D:
                           case wkbLineString:
                           {
                              if(traceDebug())
                              {
                                 ossimNotify(ossimNotifyLevel_DEBUG) << "Loading line string" << std::endl;
                              }
                              loadLineString(feature->GetFID(),
                                             (OGRLineString*)geom,
                                             mapProj);
                              break;
                           }
                           case wkbPoint:
                           case wkbPoint25D:
                           {
                              if(traceDebug())
                              {
                                 ossimNotify(ossimNotifyLevel_DEBUG) << "Loading point" << std::endl;
                              }
                              loadPoint(feature->GetFID(),
                                        (OGRPoint*)geom,
                                        mapProj);
                              break;
                           }
                           case wkbMultiPolygon25D:
                           case wkbMultiPolygon:
                           {
                              if(traceDebug())
                              {
                                 ossimNotify(ossimNotifyLevel_DEBUG) << "Loading multi polygon" << std::endl;
                              }
                              if (m_geometryDistance > 0.0)
                              {
                                 OGRGeometry* bufferGeom = geom->Buffer(m_geometryDistance);
                                 loadMultiPolygon(feature->GetFID(),
                                    (OGRMultiPolygon*)bufferGeom,
                                    mapProj);
                              }
                              else
                              {
                                 loadMultiPolygon(feature->GetFID(),
                                    (OGRMultiPolygon*)geom,
                                    mapProj);
                              }
                              break;
                                               
                           }
                           case wkbMultiLineString:
                           {
                              if(traceDebug())
                              {
                                 ossimNotify(ossimNotifyLevel_DEBUG) << "Loading line string" << std::endl;
                              }
                              loadMultiLineString(feature->GetFID(),
                                    (OGRMultiLineString*)geom,
                                    mapProj);
                                 break;
                              }
                           default:
                           {
                              if(traceDebug())
                              {
                                 ossimNotify(ossimNotifyLevel_WARN)
                                    << "ossimGdalOgrVectorAnnotation::initializeTables WARNING\n"
                                    
                                    << OGRGeometryTypeToName(geom->getGeometryType())
                                    <<" NOT SUPPORTED!"
                                    << endl;
                              }
                              break;
                           }
                        }
                        geom->getEnvelope(&extent);
                        if(mapProj)
                        {
                           ossimDrect rect(extent.MinX,
                                           extent.MaxY,
                                           extent.MaxX,
                                           extent.MinY,
                                           OSSIM_RIGHT_HANDED);
                           ossimGpt g1 = mapProj->inverse(rect.ul());
                           ossimGpt g2 = mapProj->inverse(rect.ur());
                           ossimGpt g3 = mapProj->inverse(rect.lr());
                           ossimGpt g4 = mapProj->inverse(rect.ll());
                           
                           theLayerTable[i]->theFeatureList.push_back(ossimOgrGdalFeatureNode(feature->GetFID(),
                                                                                              ossimDrect(ossimDpt(g1),
                                                                                                         ossimDpt(g2),
                                                                                                         ossimDpt(g3),
                                                                                                         ossimDpt(g4))));
                           
                        }
                        else
                        {
                           theLayerTable[i]->theFeatureList.push_back(ossimOgrGdalFeatureNode(feature->GetFID(),
                                                                                              ossimDrect(extent.MinX,
                                                                                                         extent.MinY,
                                                                                                         extent.MaxX,
                                                                                                         extent.MaxY)));
                        }
                     }
               }
               delete feature;
            }

            //if an OGRLayer pointer representing a results set from the query, this layer is 
            //in addition to the layers in the data store and must be destroyed with 
            //OGRDataSource::ReleaseResultSet() before the data source is closed (destroyed).
            if (!m_query.empty() && layer != NULL)
            {
              theDataSource->ReleaseResultSet(layer);
            }  
         }
         else
         {
            theLayerTable[i] = NULL;
         }
      }
   }
   computeBoundingRect();
   updateAnnotationSettings();
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalOgrVectorAnnotation::initializeTables(): leaving..."
         << std::endl;
   }
}

void ossimGdalOgrVectorAnnotation::deleteTables()
{
   for(ossim_uint32 i = 0; i < theLayerTable.size(); ++i)
   {
      if(theLayerTable[i])
      {
         delete theLayerTable[i];
      }
   }

   theLayerTable.clear();
   std::multimap<long, ossimAnnotationObject*>::iterator current = theFeatureCacheTable.begin();

   while(current != theFeatureCacheTable.end())
   {
      ((*current).second)->unref();
      ++current;
   }
   
   theFeatureCacheTable.clear();
}


void ossimGdalOgrVectorAnnotation::getFeature(vector<ossimAnnotationObject*>& featureList,
                                              long id)
{
   std::multimap<long, ossimAnnotationObject*>::iterator iter = theFeatureCacheTable.find(id);
   
   while( (iter != theFeatureCacheTable.end()) && ((*iter).first == id)  )
   {
      featureList.push_back((*iter).second);
      ++iter;
   }
}

ossimProjection* ossimGdalOgrVectorAnnotation::createProjFromReference(OGRSpatialReference* reference)const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << "ossimGdalOgrVectorAnnotation::createProjFromReference:   entered........" << std::endl;
   }
   if(!reference)
   {
      if(traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG) << "ossimGdalOgrVectorAnnotation::createProjFromReference:   leaving 1........" << std::endl;
      }
      return NULL;
   }
   if(reference->IsGeographic()||reference->IsLocal())
   {
      if(traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG) << "ossimGdalOgrVectorAnnotation::createProjFromReference:   leaving 2........" << std::endl;
      }
      return NULL;
   }
   char* wktString = NULL;
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
      ossimNotify(ossimNotifyLevel_DEBUG) << "ossimGdalOgrVectorAnnotation::createProjFromReference:   returning........" << std::endl;
   }
   return ossimProjectionFactoryRegistry::instance()->createProjection(kwl);
}

void ossimGdalOgrVectorAnnotation::loadPolygon(long id, OGRPolygon* poly, ossimMapProjection* mapProj)
{
   OGRLinearRing* ring = poly->getExteriorRing();
   
   ossimGpt origin;
   
   if(theImageGeometry.valid()&&theImageGeometry->getProjection())
   {
      origin = theImageGeometry->getProjection()->origin();
   }
   
   ossimRgbVector color;
   
   if(theFillFlag)
   {
      color = theBrushColor;
   }
   else
   {
      color = thePenColor;
   }
   
   if(ring)
   {
      int upper = ring->getNumPoints();
      vector<ossimGpt> points(upper);
      for(int i = 0; i < upper; ++i)
      {
         OGRPoint ogrPt;
         ring->getPoint(i, &ogrPt);
         if(mapProj)
         {
            points[i] = mapProj->inverse(ossimDpt(ogrPt.getX(),
                                                  ogrPt.getY()));

         }
         else
         {
            points[i] = ossimGpt(ogrPt.getY(),
                                 ogrPt.getX(),
                                 ogrPt.getZ(),
                                 origin.datum());
         }
      }
      ossimGeoAnnotationObject* annotation =
         new ossimGeoAnnotationPolyObject(points,
                                          theFillFlag,
                                          color.getR(),
                                          color.getG(),
                                          color.getB(),
                                          theThickness);
      if(theImageGeometry.valid())
      {
         annotation->transform(theImageGeometry.get());
      }
      
      theFeatureCacheTable.insert(make_pair(id,
                                            annotation));
   }
   int bound = poly->getNumInteriorRings();
   if(bound)
   {
      for(int i = 0; i < bound; ++i)
       {
          ring = poly->getInteriorRing(i);
          if(ring)
          {
             int j = 0;
             int upper = ring->getNumPoints();
             vector<ossimGpt> points(upper);
             for(j = 0; j < upper; ++j)
             {
                OGRPoint ogrPt;
                ring->getPoint(j, &ogrPt);
                if(mapProj)
                {
                   ossimDpt eastingNorthing(ogrPt.getX(),
                                            ogrPt.getY());
                   
                   points[j] = mapProj->inverse(eastingNorthing);
                   
                }
                else
                {
                   points[j] = ossimGpt(ogrPt.getY(),
                                        ogrPt.getX(),
                                        ogrPt.getZ(),
                                        origin.datum());
                }
             }
             ossimGeoAnnotationPolyObject* annotation =
                new ossimGeoAnnotationPolyObject(points,
                                                 theFillFlag,
                                                 color.getR(),
                                                 color.getG(),
                                                 color.getB(),
                                                 theThickness);
             annotation->setPolyType(ossimGeoAnnotationPolyObject::OSSIM_POLY_INTERIOR_RING);
       
             if(theImageGeometry.valid())
             {
                annotation->transform(theImageGeometry.get());
             }

             theFeatureCacheTable.insert(make_pair(id,
                                              annotation));

          }
       }
    }
}

void ossimGdalOgrVectorAnnotation::loadLineString(long id, OGRLineString* lineString,
                                            ossimMapProjection* mapProj)
{
    int upper = lineString->getNumPoints();
    ossimGpt origin;
    if(theImageGeometry.valid()&&theImageGeometry->getProjection())
    {
       origin = theImageGeometry->getProjection()->origin();
    }
    
   ossimRgbVector color;
   
   if(theFillFlag)
   {
      color = theBrushColor;
   }
   else
   {
      color = thePenColor;
   }
   vector<ossimGpt> polyLine(upper);
   for(int i = 0; i < upper; ++i)
   {
      OGRPoint ogrPt;
      
      lineString->getPoint(i, &ogrPt);
      
      if(mapProj)
      {
         ossimDpt eastingNorthing(ogrPt.getX(),
                                  ogrPt.getY());
         
         polyLine[i] = mapProj->inverse(eastingNorthing);
      }
      else
      {
         polyLine[i] = ossimGpt(ogrPt.getY(),
                                ogrPt.getX(),
                                ogrPt.getZ(),
                                origin.datum());
      }
   }
   
   ossimGeoAnnotationPolyLineObject* annotation =
      new ossimGeoAnnotationPolyLineObject(polyLine,
                                           color.getR(),
                                           color.getG(),
                                           color.getB(),
                                           theThickness);
   if(theImageGeometry.valid())
   {
      annotation->transform(theImageGeometry.get());
   }

   theFeatureCacheTable.insert(make_pair(id,
                                         annotation));
}

void ossimGdalOgrVectorAnnotation::loadMultiLineString(long id, OGRMultiLineString* multiLineString,
   ossimMapProjection* mapProj)
{
   ossimRgbVector color;

   if(theFillFlag)
   {
      color = theBrushColor;
   }
   else
   {
      color = thePenColor;
   }

   ossim_uint32 numGeometries = multiLineString->getNumGeometries();
   ossimGpt origin;
   if(theImageGeometry.valid()&&theImageGeometry->getProjection())
   {
      origin = theImageGeometry->getProjection()->origin();
   }

   vector<ossimGeoPolygon> geoPoly;
   for(ossim_uint32 geomIdx = 0; geomIdx < numGeometries; ++geomIdx)
   {
      OGRGeometry* geomRef = multiLineString->getGeometryRef(geomIdx);
      OGRLineString* lineString = (OGRLineString*)geomRef;
      if (lineString)
      {
         int upper = lineString->getNumPoints();
         vector<ossimGpt> polyLine(upper);
         for(int i = 0; i < upper; ++i)
         {
            OGRPoint ogrPt;

            lineString->getPoint(i, &ogrPt);

            if(mapProj)
            {
               ossimDpt eastingNorthing(ogrPt.getX(),
                  ogrPt.getY());

               polyLine[i] = mapProj->inverse(eastingNorthing);
            }
            else
            {
               polyLine[i] = ossimGpt(ogrPt.getY(),
                  ogrPt.getX(),
                  ogrPt.getZ(),
                  origin.datum());
            }
         }

         ossimGeoAnnotationPolyLineObject* annotation =
            new ossimGeoAnnotationPolyLineObject(polyLine,
            color.getR(),
            color.getG(),
            color.getB(),
            theThickness);
         if(theImageGeometry.valid())
         {
            annotation->transform(theImageGeometry.get());
         }

         theFeatureCacheTable.insert(make_pair(id,
            annotation));
      }
   }
}

void ossimGdalOgrVectorAnnotation::loadPoint(long id, OGRPoint* point, ossimMapProjection* mapProj)
{
   ossimGpt origin;
   ossimRgbVector color;
   
   if(theFillFlag)
   {
      color = theBrushColor;
   }
   else
   {
      color = thePenColor;
   }
   if(theImageGeometry.valid()&&theImageGeometry->getProjection())
   {
      origin = theImageGeometry->getProjection()->origin();
   }
   
   ossimGpt gpt;
   if(mapProj)
   {
      ossimDpt eastingNorthing(point->getX(),
                               point->getY());
      
      gpt = mapProj->inverse(eastingNorthing);
   }
   else
   {
      gpt = ossimGpt(point->getY(),
                     point->getX(),
                     point->getZ(),
                     origin.datum());
   }
   
   
   ossimGeoAnnotationEllipseObject* annotation =
      new ossimGeoAnnotationEllipseObject(gpt,
                                          thePointWidthHeight,
                                          theFillFlag,
                                          color.getR(),
                                          color.getG(),
                                          color.getB(),
                                          theThickness);
   if(theImageGeometry.valid())
   {
      annotation->transform(theImageGeometry.get());
   }
   theFeatureCacheTable.insert(make_pair(id, annotation));
}

void ossimGdalOgrVectorAnnotation::loadMultiPoint(long id,
                                                  OGRMultiPoint* multiPoint,
                                                  ossimMapProjection* mapProj)
{
   ossim_uint32 numGeometries = multiPoint->getNumGeometries();
   ossimRgbVector color;
   
   if(theFillFlag)
   {
      color = theBrushColor;
   }
   else
   {
      color = thePenColor;
   }
   
   ossimGeoAnnotationMultiEllipseObject* annotation =
      new ossimGeoAnnotationMultiEllipseObject(thePointWidthHeight,
                                               theFillFlag,
                                               color.getR(),
                                               color.getG(),
                                               color.getB(),
                                               theThickness);
   ossimGpt origin;
   if(theImageGeometry.valid()&&theImageGeometry->getProjection())
   {
      origin = theImageGeometry->getProjection()->origin();
   }
   for(ossim_uint32 i = 0; i < numGeometries; ++i)
   {
      OGRGeometry* geomRef = multiPoint->getGeometryRef(i);
      if( geomRef &&
          ( (geomRef->getGeometryType()==wkbPoint) ||
            (geomRef->getGeometryType()==wkbPoint25D) ) )
      {
         OGRPoint* point = (OGRPoint*)geomRef;
         
         if(mapProj)
         {
            ossimDpt eastingNorthing(point->getX(),
                                     point->getY());
            
            annotation->addPoint(mapProj->inverse(eastingNorthing));
         }
         annotation->addPoint(ossimGpt(point->getY(),
                                       point->getX(),
                                       point->getZ(),
                                       origin.datum()));
      }
   }
   if(theImageGeometry.valid())
   {
      annotation->transform(theImageGeometry.get());
   }

   theFeatureCacheTable.insert(make_pair(id, annotation));
}


void ossimGdalOgrVectorAnnotation::loadMultiPolygon(
   long id,
   OGRMultiPolygon* multiPolygon,
   ossimMapProjection* mapProj)
{
   ossimRgbVector color;
   
   if(theFillFlag)
   {
      color = theBrushColor;
   }
   else
   {
      color = thePenColor;
   }
   ossimGpt origin;
   ossim_uint32 numGeometries = multiPolygon->getNumGeometries();

   if(theImageGeometry.valid()&&theImageGeometry->getProjection())
   {
      origin = theImageGeometry->getProjection()->origin();
   }

   vector<ossimGeoPolygon> geoPoly;
   for(ossim_uint32 geomIdx = 0; geomIdx < numGeometries; ++geomIdx)
   {
      OGRGeometry* geomRef = multiPolygon->getGeometryRef(geomIdx);
      if( geomRef &&
          ( (geomRef->getGeometryType()==wkbPolygon) ||
            (geomRef->getGeometryType()==wkbPolygon25D) ) )
      {
         geoPoly.push_back(ossimGeoPolygon());
         OGRPolygon* poly = (OGRPolygon*)geomRef;
         OGRLinearRing* ring = poly->getExteriorRing();
         ossim_uint32 currentPoly = geoPoly.size()-1;

         if(ring)
         {
            ossim_uint32 upper = ring->getNumPoints();

            for(ossim_uint32 ringPointIdx = 0;
                ringPointIdx < upper;
                ++ringPointIdx)
            {
               OGRPoint ogrPt;
               ring->getPoint(ringPointIdx, &ogrPt);
               if(mapProj)
               {
                  geoPoly[currentPoly].addPoint(
                     mapProj->inverse(ossimDpt(ogrPt.getX(), ogrPt.getY())));
               }
               else
               {
                  geoPoly[currentPoly].addPoint(ogrPt.getY(),
                                                ogrPt.getX(),
                                                ogrPt.getZ(),
                                                origin.datum());
               }
            }
         }
         
         ossim_uint32 bound = poly->getNumInteriorRings();

         if(bound)
         {
            for(ossim_uint32 interiorRingIdx = 0;
                interiorRingIdx < bound;
                ++interiorRingIdx)
            {
               ring = poly->getInteriorRing(interiorRingIdx);

               if(ring)
               {
                  geoPoly.push_back(ossimGeoPolygon());
                  currentPoly = geoPoly.size()-1;

                  ossim_uint32 upper = ring->getNumPoints();

                  for(ossim_uint32 interiorRingPointIdx = 0;
                      interiorRingPointIdx < upper;
                      ++interiorRingPointIdx)
                  {
                     OGRPoint ogrPt;
                     ring->getPoint(interiorRingPointIdx, &ogrPt);
                     if(mapProj)
                     {
                        geoPoly[currentPoly].addPoint(
                           mapProj->inverse(ossimDpt(ogrPt.getX(),
                                                     ogrPt.getY())));
                     }
                     else
                     {
                        geoPoly[currentPoly].addPoint(ogrPt.getY(),
                                                      ogrPt.getX(),
                                                      ogrPt.getZ(),
                                                      origin.datum());
                     }
                  }
               }
            }
         }
      }
   }
   
   if(geoPoly.size())
   {
      ossimGeoAnnotationMultiPolyObject* annotation =
         new ossimGeoAnnotationMultiPolyObject(geoPoly,
                                               theFillFlag,
                                               color.getR(),
                                               color.getG(),
                                               color.getB(),
                                               theThickness);
      if(theImageGeometry.valid())
      {
         annotation->transform(theImageGeometry.get());
      }

      theFeatureCacheTable.insert(make_pair(id, annotation));
   }
}

bool ossimGdalOgrVectorAnnotation::open()
{
   return open(theFilename);
}

bool ossimGdalOgrVectorAnnotation::isOpen()const
{
   return (theDataSource!=NULL);
}

const ossimObject* ossimGdalOgrVectorAnnotation::getView()const
{
   return theImageGeometry.get();
}

void ossimGdalOgrVectorAnnotation::setBrushColor(const ossimRgbVector& brushColor)
{
   theBrushColor = brushColor;
}
   
void ossimGdalOgrVectorAnnotation::setPenColor(const ossimRgbVector& penColor)
{
   thePenColor = penColor;
}

ossimRgbVector ossimGdalOgrVectorAnnotation::getPenColor()const
{
   return thePenColor;
}

ossimRgbVector ossimGdalOgrVectorAnnotation::getBrushColor()const
{
   return theBrushColor;
}

double ossimGdalOgrVectorAnnotation::getPointRadius()const
{
   return thePointWidthHeight.x/2.0;
}

void ossimGdalOgrVectorAnnotation::setPointRadius(double r)
{
   thePointWidthHeight = ossimDpt(fabs(r)*2, fabs(r)*2);
}

bool ossimGdalOgrVectorAnnotation::getFillFlag()const
{
   return theFillFlag;
}

void ossimGdalOgrVectorAnnotation::setFillFlag(bool flag)
{
   theFillFlag=flag;
}

void ossimGdalOgrVectorAnnotation::setThickness(ossim_int32 thickness)
{
   if ( (thickness > 0) && (thickness < 256) )
   {
      theThickness = static_cast<ossim_uint8>(thickness);
   }
   else
   {
      ossimNotify(ossimNotifyLevel_WARN)
         << "ossimGdalOgrVectorAnnotation::setThickness range error: "
         << thickness
         << "\nValid range: 1 to 255"
         << std::endl;
   }
}

ossim_int32 ossimGdalOgrVectorAnnotation::getThickness()const
{
   return static_cast<ossim_int32>(theThickness);
}

void ossimGdalOgrVectorAnnotation::loadExternalGeometryFile()
{
   ossimFilename filename = theFilename;
   filename.setExtension(".geom");
   if(!filename.exists())
   {
      filename.setExtension(".GEOM");
      if(!filename.exists())
      {
         return;
      }
   }

   ossimKeywordlist kwl;
   if(kwl.addFile(filename))
   {
      ossimRefPtr<ossimImageGeometry> geom = new ossimImageGeometry;
      geom->loadState(kwl);
      if(geom->getProjection())
      {
         theImageGeometry = geom;
         ossimMapProjection* mapProj = PTR_CAST(ossimMapProjection, theImageGeometry->getProjection());
         if (mapProj)
         {
            // drb...
            //mapProj->setUlGpt(mapProj->origin());
            theIsExternalGeomFlag = true;

            if (traceDebug())
            {
               ossimNotify(ossimNotifyLevel_DEBUG)
                  << "ossimGdalOgrVectorAnnotation::loadExternalGeometryFile"
                  << " DEBUG:"
                  << "\nExternal projection loaded from geometry file."
                  << "\nProjection dump:" << std::endl;
               mapProj->print(ossimNotify(ossimNotifyLevel_DEBUG));
            }
         }
      }
   }
}

void ossimGdalOgrVectorAnnotation::loadExternalImageGeometryFromXml()
{
   ossimFilename filename = theFilename;
   ossimString fileBase = filename.noExtension();
   ossimFilename xmlFile = ossimString(fileBase + ".xml");
   if (!xmlFile.exists())//try the xml file which includes the entire source file name
   {
      xmlFile = theFilename + ".xml";
   }
   if (!xmlFile.exists())
   {
      return;
   }
   ossimFgdcXmlDoc* fgdcXmlDoc = new ossimFgdcXmlDoc;
   if ( fgdcXmlDoc->open(xmlFile) )
   {
      ossimRefPtr<ossimProjection> proj = fgdcXmlDoc->getProjection();
      if ( proj.valid() )
      {
         theImageGeometry = new ossimImageGeometry;
         theImageGeometry->setProjection( proj.get() );
         theIsExternalGeomFlag = true;
      }
   }
   delete fgdcXmlDoc;
   fgdcXmlDoc = 0;
}

void ossimGdalOgrVectorAnnotation::loadOmdFile()
{
   ossimFilename filename = theFilename;
   filename.setExtension(".omd");
   if(!filename.exists())
   {
      filename.setExtension(".OMD");
   }

   if( filename.exists() )
   {
      ossimKeywordlist kwl;
      if( kwl.addFile(filename) )
      {
         //---
         // Because loadState can call open() we will not call it here to
         // avoid a circuliar loop.  This duplicates some code but is safer.
         //---
         const char* lookup = 0;

         // Border:
         lookup = kwl.find(ossimKeywordNames::BORDER_SIZE_KW);
         if (lookup)
         {
            theBorderSize = ossimString(lookup).toDouble();
            ossimString unitPrefix = ossimKeywordNames::BORDER_SIZE_KW +
               ossimString(".");
      
            theBorderSizeUnits = (ossimUnitType)ossimUnitTypeLut::instance()->
               getEntryNumber(kwl, unitPrefix.c_str());
            if(theBorderSizeUnits != OSSIM_UNIT_UNKNOWN)
            {
               ossimUnitConversionTool unitConvert(theBorderSize,
                                                   theBorderSizeUnits);
         
               theBorderSize      = unitConvert.getValue(OSSIM_DEGREES);
               theBorderSizeUnits = OSSIM_DEGREES;
            }
            else // assume degrees
            {
               theBorderSizeUnits = OSSIM_DEGREES;
            }
         }

         // Brush color:
         lookup = kwl.find(ossimKeywordNames::BRUSH_COLOR_KW);
         if (lookup)
         {
            int r, g, b;
            std::istringstream s(lookup);
            s>>r>>g>>b;
            theBrushColor = ossimRgbVector((ossim_uint8)r,
                                           (ossim_uint8)g,
                                           (ossim_uint8)b);
         }
         else
         {
            lookup = kwl.find(NORMALIZED_RGB_BRUSH_COLOR_KW);
            if (lookup)
            {
               ossim_float64 r;
               ossim_float64 g;
               ossim_float64 b;
               
               std::istringstream in(lookup);
               in >> r >> g >> b;
               
               if ( (r >= 0.0) && (r <=1.0) )
               {
                  theBrushColor.setR(static_cast<ossim_uint8>(r*255.0+0.5));
               }
               if ( (g >= 0.0) && (g <=1.0) )
               {
                  theBrushColor.setG(static_cast<ossim_uint8>(g*255.0+0.5));
               }
               if ( (b >= 0.0) && (b <=1.0) )
               {
                  theBrushColor.setB(static_cast<ossim_uint8>(b*255.0+0.5));
               }
            }
         }

         // Fill:
         lookup = kwl.find(ossimKeywordNames::FILL_FLAG_KW);
         if (lookup)
         {
            theFillFlag = ossimString(lookup).toBool();
         }
         
         // Pen color:
         lookup = kwl.find(ossimKeywordNames::PEN_COLOR_KW);
         if (lookup)
         {
            int r, g, b;
            std::istringstream s(lookup);
            s>>r>>g>>b;
            thePenColor = ossimRgbVector((ossim_uint8)r,
                                         (ossim_uint8)g,
                                         (ossim_uint8)b);
         }
         else
         {
            lookup = kwl.find(NORMALIZED_RGB_PEN_COLOR_KW);
            if (lookup)
            {
               ossim_float64 r;
               ossim_float64 g;
               ossim_float64 b;
               
               std::istringstream in(lookup);
               in >> r >> g >> b;
               
               if ( (r >= 0.0) && (r <=1.0) )
               {
                  thePenColor.setR(static_cast<ossim_uint8>(r * 255.0 + 0.5));
               }
               if ( (g >= 0.0) && (g <=1.0) )
               {
                  thePenColor.setG(static_cast<ossim_uint8>(g * 255.0 + 0.5));
               }
               if ( (b >= 0.0) && (b <=1.0) )
               {
                  thePenColor.setB(static_cast<ossim_uint8>(b * 255.0 + 0.5));
               }
            }
         }

         // Point size:
         lookup = kwl.find(ossimKeywordNames::POINT_WIDTH_HEIGHT_KW);
         if (!lookup)
         {
            lookup = kwl.find(POINT_SIZE_KW);
         }
         if (lookup)
         {
            ossim_float64 x;
            ossim_float64 y;
            
            std::istringstream in(lookup);
            in >> x >> y;
            
            if ( (x > 0.0) && (y > 0.0) )
            {
               thePointWidthHeight.x = x;
               thePointWidthHeight.y = y;
            }
         }

         // Thickness:
         lookup = kwl.find(ossimKeywordNames::THICKNESS_KW);
         if (lookup)
         {
            setThickness(ossimString(lookup).toInt32());
         }

         // Update the feature table.
         updateAnnotationSettings();
         
      } // matches: if( kwl.addFile(filename) )
      
   } // matches: if(filename.exists())
}

void ossimGdalOgrVectorAnnotation::getDefaults()
{
   const char* lookup;

   // Look for auto color flag:
   bool autocolors = false;
   lookup = ossimPreferences::instance()->
      findPreference(SHAPEFILE_COLORS_AUTO_KW);
   if (lookup)
   {
      autocolors = ossimString::toBool(ossimString(lookup));
   }
   
   if (autocolors)
   {
      // Ensure the static index is within bounds of the array.
      if (currentAutoColorArrayIndex >= AUTO_COLOR_ARRAY_COUNT)
      {
         currentAutoColorArrayIndex = 0;
      }

      thePenColor.setR(autoColorArray[currentAutoColorArrayIndex].r);
      thePenColor.setG(autoColorArray[currentAutoColorArrayIndex].g);
      thePenColor.setB(autoColorArray[currentAutoColorArrayIndex].b);

      theBrushColor.setR(autoColorArray[currentAutoColorArrayIndex].r);
      theBrushColor.setG(autoColorArray[currentAutoColorArrayIndex].g);
      theBrushColor.setB(autoColorArray[currentAutoColorArrayIndex].b);
      
      ++currentAutoColorArrayIndex;
      if (currentAutoColorArrayIndex >= AUTO_COLOR_ARRAY_COUNT)
      {
         currentAutoColorArrayIndex = 0;
      }
      
   } // End of: if (autocolors)
   else
   {
      // Look for pen color.
      lookup = ossimPreferences::instance()->
         findPreference(NORMALIZED_RGB_PEN_COLOR_KW);
      if (lookup)
      {
         ossim_float64 r;
         ossim_float64 g;
         ossim_float64 b;
         
         std::istringstream in(lookup);
         in >> r >> g >> b;
         
         if ( (r >= 0.0) && (r <=1.0) )
         {
            thePenColor.setR(static_cast<ossim_uint8>(r * 255.0 + 0.5));
         }
         if ( (g >= 0.0) && (g <=1.0) )
         {
            thePenColor.setG(static_cast<ossim_uint8>(g * 255.0 + 0.5));
         }
         if ( (b >= 0.0) && (b <=1.0) )
         {
            thePenColor.setB(static_cast<ossim_uint8>(b * 255.0 + 0.5));
         }
      }
      
      // Look for brush color.
      lookup = ossimPreferences::instance()->
         findPreference(NORMALIZED_RGB_BRUSH_COLOR_KW);
      if (lookup)
      {
         ossim_float64 r;
         ossim_float64 g;
         ossim_float64 b;
         
         std::istringstream in(lookup);
         in >> r >> g >> b;
         
         if ( (r >= 0.0) && (r <=1.0) )
         {
            theBrushColor.setR(static_cast<ossim_uint8>(r * 255.0 + 0.5));
         }
         if ( (g >= 0.0) && (g <=1.0) )
         {
            theBrushColor.setG(static_cast<ossim_uint8>(g * 255.0 + 0.5));
         }
         if ( (b >= 0.0) && (b <=1.0) )
         {
            theBrushColor.setB(static_cast<ossim_uint8>(b * 255.0 + 0.5));
         }
      }
      
   } // End of: if (autocolors){...}else{

   // Look for point size.
   lookup = ossimPreferences::instance()->
      findPreference(POINT_SIZE_KW);
   if (lookup)
   {
      ossim_float64 x;
      ossim_float64 y;
      
      std::istringstream in(lookup);
      in >> x >> y;

      if ( (x > 0.0) && (y > 0.0) )
      {
         thePointWidthHeight.x = x;
         thePointWidthHeight.y = y;
      }
   }   
}

void ossimGdalOgrVectorAnnotation::verifyViewParams()
{
   if (!theImageGeometry.valid())
   {
      return;
   }
   
   ossimMapProjection* proj = PTR_CAST(ossimMapProjection,
                                       theImageGeometry->getProjection());
   if (!proj)
   {
      return;
   }

   ossimGpt ulGpt = proj->getUlGpt();
   if ( ulGpt.isLatNan() || ulGpt.isLonNan() )
   {
      proj->setUlGpt( ossimGpt(theBoundingExtent.MaxY,
                               theBoundingExtent.MinX) );
   }
   
   if (proj->isGeographic())
   {
      ossimDpt pt = proj->getDecimalDegreesPerPixel();
      if( pt.hasNans() )
      {
         // roughly... 10 meters per pixel in decimal degrees.
         ossim_float64 d = 1.0 / 11131.49079;
         proj->setDecimalDegreesPerPixel(ossimDpt(d, d));
      }

   }
   else
   {
      ossimDpt pt = proj->getMetersPerPixel();
      if (pt.hasNans())
      {
         proj->setMetersPerPixel(ossimDpt(10.0, 10.0));
      }
   }
}

std::multimap<long, ossimAnnotationObject*> ossimGdalOgrVectorAnnotation::getFeatureTable()
{
   if (theFeatureCacheTable.size() == 0)
   {
      initializeTables();
   }
   return theFeatureCacheTable;
}

bool ossimGdalOgrVectorAnnotation::setCurrentEntry(ossim_uint32 entryIdx)
{
   if (entryIdx < m_layerNames.size())
   {
      m_layerName = m_layerNames[entryIdx];
      return open(theFilename);
   }
   return false;
}
