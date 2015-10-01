//*******************************************************************
//
// License:  See top level LICENSE.txt file.
//
// Author:  Garrett Potts
//
// Description:
//
// Contains class implementaiton for the class "ossimOgrGdalTileSource".
//
//*******************************************************************
//  $Id: ossimOgrGdalTileSource.cpp 20133 2011-10-12 19:03:47Z oscarkramer $

#include <ossimOgrGdalTileSource.h>

RTTI_DEF3(ossimOgrGdalTileSource,
          "ossimOgrGdalTileSource",
          ossimImageHandler,
          ossimViewInterface,
          ossimEsriShapeFileInterface);

//*******************************************************************
// Public Constructor:
//*******************************************************************
ossimOgrGdalTileSource::ossimOgrGdalTileSource()
   :
      ossimImageHandler(),
      ossimViewInterface(),
      ossimEsriShapeFileInterface()
{
   theObject = this;
   theAnnotationSource = new ossimGdalOgrVectorAnnotation();
   thePixelType = OSSIM_PIXEL_IS_AREA;
}

//*******************************************************************
// Destructor:
//*******************************************************************
ossimOgrGdalTileSource::~ossimOgrGdalTileSource()
{
   ossimViewInterface::theObject = 0;
   close();
   
}

//*******************************************************************
// Public Method:
//*******************************************************************
bool ossimOgrGdalTileSource::open()
{
   return theAnnotationSource->open(theImageFile);
}

//*******************************************************************
// Public method:
//*******************************************************************
bool ossimOgrGdalTileSource::saveState(ossimKeywordlist& kwl,
                                       const char* prefix) const
{
   if(theAnnotationSource.valid())
   {
      theAnnotationSource->saveState(kwl, prefix);
   }
   return ossimImageHandler::saveState(kwl, prefix);
}

//*******************************************************************
// Public method:
//*******************************************************************
bool ossimOgrGdalTileSource::loadState(const ossimKeywordlist& kwl,
                                       const char* prefix)
{
   if (ossimImageHandler::loadState(kwl, prefix))
   {
      thePixelType = OSSIM_PIXEL_IS_AREA;
      if (open())
      {
         if(theAnnotationSource.valid())
         {
            theAnnotationSource->loadState(kwl, prefix);
         }
         return true;
      }
   }

   return false;
}

ossim_uint32 ossimOgrGdalTileSource::getImageTileWidth() const
{
   return 0;
}

ossim_uint32 ossimOgrGdalTileSource::getImageTileHeight() const
{
   return 0;
}

ossimRefPtr<ossimImageGeometry> ossimOgrGdalTileSource::getInternalImageGeometry() const
{
   theGeometry = theAnnotationSource->getImageGeometry();

   return theGeometry;  
}

void ossimOgrGdalTileSource::close()
{
   theAnnotationSource->close();
}

ossimRefPtr<ossimImageData> ossimOgrGdalTileSource::getTile(
   const ossimIrect& tileRect, ossim_uint32 resLevel)
{
   return theAnnotationSource->getTile(tileRect, resLevel);
}

ossim_uint32 ossimOgrGdalTileSource::getNumberOfInputBands() const
{
   return theAnnotationSource->getNumberOfOutputBands();
}

ossim_uint32 ossimOgrGdalTileSource::getNumberOfOutputBands() const
{
   return theAnnotationSource->getNumberOfOutputBands();
}

ossim_uint32 ossimOgrGdalTileSource::getNumberOfLines(ossim_uint32 /* reduced_res_level */ ) const
{
   ossimIrect theBounds = theAnnotationSource->getBoundingRect();
   
   if(theBounds.hasNans())
   {
      return theBounds.ul().x;
   }
   
   return theBounds.height();
}

ossim_uint32 ossimOgrGdalTileSource::getNumberOfSamples(ossim_uint32 /* reduced_res_level */ ) const
{
   ossimIrect theBounds = theAnnotationSource->getBoundingRect();
   
   if(theBounds.hasNans())
   {
      return theBounds.ul().x;
   }
   
   return theBounds.height();
}

ossim_uint32 ossimOgrGdalTileSource::getNumberOfDecimationLevels() const
{
   return 32;
}

ossimIrect ossimOgrGdalTileSource::getImageRectangle(ossim_uint32 /* reduced_res_level */ ) const
{
   return theAnnotationSource->getBoundingRect();
}

ossimScalarType ossimOgrGdalTileSource::getOutputScalarType() const
{
   return theAnnotationSource->getOutputScalarType();
}

ossim_uint32 ossimOgrGdalTileSource::getTileWidth() const
{
   return theAnnotationSource->getTileWidth();         
}

ossim_uint32 ossimOgrGdalTileSource::getTileHeight() const
{
   return theAnnotationSource->getTileHeight();
}

bool ossimOgrGdalTileSource::isOpen()const
{
   return theAnnotationSource->isOpen();
}
   
double ossimOgrGdalTileSource::getNullPixelValue(ossim_uint32 band)const
{
   return theAnnotationSource->getNullPixelValue(band);
}

double ossimOgrGdalTileSource::getMinPixelValue(ossim_uint32 band)const
{
   return theAnnotationSource->getMinPixelValue(band);
}

double ossimOgrGdalTileSource::getMaxPixelValue(ossim_uint32 band)const
{
   return theAnnotationSource->getMaxPixelValue(band);
}

ossimObject* ossimOgrGdalTileSource::getView()
{
   return theAnnotationSource->getView();
}

const ossimObject* ossimOgrGdalTileSource::getView()const
{
   return theAnnotationSource->getView();
}

bool ossimOgrGdalTileSource::setView(ossimObject*  baseObject)
{
   return theAnnotationSource->setView(baseObject);
}

void ossimOgrGdalTileSource::setProperty(ossimRefPtr<ossimProperty> property)
{
   if(theAnnotationSource.valid())
   {
      theAnnotationSource->setProperty(property);
   }
}

ossimRefPtr<ossimProperty> ossimOgrGdalTileSource::getProperty(const ossimString& name)const
{
   if(theAnnotationSource.valid())
   {
      return theAnnotationSource->getProperty(name);
   }
   
   return 0;
}

void ossimOgrGdalTileSource::getPropertyNames(std::vector<ossimString>& propertyNames)const
{
   if(theAnnotationSource.valid())
   {
      theAnnotationSource->getPropertyNames(propertyNames); 
   }
}

std::multimap<long, ossimAnnotationObject*> ossimOgrGdalTileSource::getFeatureTable()
{
  if(theAnnotationSource.valid())
  {
    return theAnnotationSource->getFeatureTable();
  }
  return std::multimap<long, ossimAnnotationObject*>();
}

void ossimOgrGdalTileSource::setQuery(const ossimString& query)
{
  if (theAnnotationSource.valid())
  {
    theAnnotationSource->setQuery(query);
  }
}

void ossimOgrGdalTileSource::setGeometryBuffer(ossim_float64 distance, ossimUnitType type)
{
   if (theAnnotationSource.valid())
   {
      theAnnotationSource->setGeometryBuffer(distance, type);
   }
}

bool ossimOgrGdalTileSource::setCurrentEntry(ossim_uint32 entryIdx)
{
   if (theAnnotationSource.valid())
   {
      return theAnnotationSource->setCurrentEntry(entryIdx);
   }
   return false;
}
