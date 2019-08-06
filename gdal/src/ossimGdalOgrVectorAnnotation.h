//*******************************************************************
// License:  LGPL
//
// See top level LICENSE.txt file.
//
// Author:  Garrett Potts
//
// Description:
//
// Contains class implementaiton for the class "ossimOgrGdalTileSource".
//
//*******************************************************************
//  $Id: ossimGdalOgrVectorAnnotation.h 19733 2011-06-06 23:35:25Z dburken $
#ifndef ossimGdalOgrVectorAnnotation_HEADER
#define ossimGdalOgrVectorAnnotation_HEADER

#include <map>
#include <vector>
#include <list>
#include <gdal.h>
#include <ogrsf_frmts.h>
#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/base/ossimViewInterface.h>
#include <ossim/base/ossimRgbVector.h>
#include <ossim/imaging/ossimAnnotationSource.h>
#include <ossim/imaging/ossimImageGeometry.h>
#include <ossim/projection/ossimProjection.h>


class ossimProjection;
class ossimMapProjection;
class ossimOgrGdalLayerNode;
class ossimOgrGdalFeatureNode;
class ossimAnnotationObject;

class OSSIM_PLUGINS_DLL ossimGdalOgrVectorAnnotation :
   public ossimAnnotationSource,
   public ossimViewInterface
{
public:
   ossimGdalOgrVectorAnnotation(ossimImageSource* inputSource=0);
   virtual ~ossimGdalOgrVectorAnnotation();
   virtual bool open();
   virtual bool open(const ossimFilename& file);
   virtual bool isOpen()const;
   virtual void close();
   virtual ossimFilename getFilename()const;
   
   virtual bool setView(ossimObject* baseObject);
   
   virtual ossimObject*       getView();
   virtual const ossimObject* getView()const;

   //! Returns the image geometry object associated with this tile source or NULL if non defined.
   //! The geometry contains full-to-local image transform as well as projection (image-to-world)
   virtual ossimRefPtr<ossimImageGeometry> getImageGeometry() const;

   virtual ossimIrect getBoundingRect(ossim_uint32 resLevel=0)const;
   virtual void computeBoundingRect();

   virtual void drawAnnotations(ossimRefPtr<ossimImageData> tile);


   virtual void setBrushColor(const ossimRgbVector& brushColor);
   virtual void setPenColor(const ossimRgbVector& penColor);
   virtual ossimRgbVector getPenColor()const;
   virtual ossimRgbVector getBrushColor()const;
   virtual double getPointRadius()const;
   virtual void setPointRadius(double r);
   virtual bool getFillFlag()const;
   virtual void setFillFlag(bool flag);
   virtual void setThickness(ossim_int32 thickness);
   virtual ossim_int32 getThickness()const;

   virtual void setProperty(ossimRefPtr<ossimProperty> property);
   virtual ossimRefPtr<ossimProperty> getProperty(const ossimString& name)const;
   virtual void getPropertyNames(std::vector<ossimString>& propertyNames)const;
   
   virtual bool saveState(ossimKeywordlist& kwl,
                          const char* prefix=0)const;

   virtual bool loadState(const ossimKeywordlist& kwl,
                          const char* prefix=0);

   virtual std::ostream& print(std::ostream& out) const;

   std::multimap<long, ossimAnnotationObject*> getFeatureTable();

   void setQuery(const ossimString& query);

   void setGeometryBuffer(ossim_float64 distance, ossimUnitType type);

   //OGRLayer::GetExtent() returns the MBR (minimal bounding rect) of the data in the layer. 
   //So when only do ossim-info, it is not necessary to go through each feature to calculate
   //the bounding which cause the memory allocate problem when the shape file is large.
   void initializeBoundingRec(std::vector<ossimGpt> points);

   bool setCurrentEntry(ossim_uint32 entryIdx);

protected:
   OGRDataSource                      *theDataSource;
   OGRSFDriver                        *theDriver;
   ossimFilename                       theFilename;
   OGREnvelope                         theBoundingExtent;
   ossimRefPtr<ossimImageGeometry>     theImageGeometry;
   std::vector<bool>                        theLayersToRenderFlagList;
   std::vector<ossimOgrGdalLayerNode*> theLayerTable;
   ossimRgbVector                      thePenColor;
   ossimRgbVector                      theBrushColor;
   bool                                theFillFlag;
   ossim_uint8                         theThickness;
   ossimDpt                            thePointWidthHeight;
   double                              theBorderSize;
   ossimUnitType                       theBorderSizeUnits;
   ossimDrect                          theImageBound;
   bool                                theIsExternalGeomFlag;
   
   std::multimap<long, ossimAnnotationObject*> theFeatureCacheTable;

   ossimString                                 m_query;
   bool                                        m_needPenColor;
   ossim_float64                               m_geometryDistance;
   ossimUnitType                               m_geometryDistanceType;
   ossimString                                 m_layerName;
   std::vector<ossimString>                    m_layerNames;
   
   void computeDefaultView();

   /** Uses theViewProjection */
   void transformObjectsFromView();
   
   void loadPoint(long id, OGRPoint* point, ossimMapProjection* mapProj);
   void loadMultiPoint(long id, OGRMultiPoint* multiPoint, ossimMapProjection* mapProj);
   void loadMultiPolygon(long id, OGRMultiPolygon* multiPolygon, ossimMapProjection* mapProj);
   void loadPolygon(long id, OGRPolygon* polygon, ossimMapProjection* mapProj);
   void loadLineString(long id, OGRLineString* lineString, ossimMapProjection* mapProj);
   void loadMultiLineString(long id, OGRMultiLineString* multiLineString, ossimMapProjection* mapProj);
   
   void getFeatures(std::list<long>& result,
                    const ossimIrect& rect);
   void getFeature(std::vector<ossimAnnotationObject*>& featureList,
                   long id);
   ossimProjection* createProjFromReference(OGRSpatialReference* reference)const;
   void initializeTables();
   void deleteTables();
   void updateAnnotationSettings();

   /**
    * Will set theViewProjection if geometry file is present with projection.
    * Also sets theIsExternalGeomFlag.
    */
   void loadExternalGeometryFile();

   /**
    * Will set theViewProjection if xml file is present with projection.
    * Also sets theIsExternalGeomFlag.
    */
   void loadExternalImageGeometryFromXml();

   /**
    * Looks for "file.omd" and loads pen, brush and point settings if present.
    */
   void loadOmdFile();

   /**
    * Will set thePenColor and theBrushColor if keyword found in preferences.
    *
    * Keyword example:
    * shapefile_normalized_rgb_pen_color:   0.004 1.0 0.004
    * shapefile_normalized_rgb_brush_color: 0.004 1.0 0.004
    */
   void getDefaults();

   /**
    * @brief Checks for nan scale and tie point.  Sets to some default if
    * nan.
    */
   void verifyViewParams();

TYPE_DATA
   
}; // End of: class ossimGdalOgrVectorAnnotation

#endif
