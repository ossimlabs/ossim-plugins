//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Mingjie Su, Harsh Govind
//
// Description: Factory for OSSIM KmlSuperOverlay writers.
//----------------------------------------------------------------------------
// $Id: ossimKmlSuperOverlayWriterFactory.cpp 2178 2011-02-17 18:38:30Z ming.su $

#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/imaging/ossimImageFileWriter.h>

#include "ossimKmlSuperOverlayWriterFactory.h"
#include "ossimKmlSuperOverlayWriter.h"

ossimKmlSuperOverlayWriterFactory* ossimKmlSuperOverlayWriterFactory::theInstance = 0;

RTTI_DEF1(ossimKmlSuperOverlayWriterFactory,
          "ossimKmlSuperOverlayWriterFactory",
          ossimImageWriterFactoryBase);

ossimKmlSuperOverlayWriterFactory::~ossimKmlSuperOverlayWriterFactory()
{
   theInstance = 0;
}

ossimKmlSuperOverlayWriterFactory* ossimKmlSuperOverlayWriterFactory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimKmlSuperOverlayWriterFactory;
   }
   return theInstance;
}

ossimImageFileWriter* ossimKmlSuperOverlayWriterFactory::createWriterFromExtension(
   const ossimString& fileExtension)const
{
   ossimImageFileWriter* writer = 0;

   if(fileExtension == "kml" || fileExtension == "kmz")
   {
      writer = new ossimKmlSuperOverlayWriter;
   }
   return writer;
}

void ossimKmlSuperOverlayWriterFactory::getImageFileWritersBySuffix(ossimImageWriterFactoryBase::ImageFileWriterList& result,
                                                        const ossimString& ext)const
{
   ossimString testExt = ext.downcase();
   if(testExt == "kml" || testExt == "kmz")
   {
      result.push_back(new ossimKmlSuperOverlayWriter);
   }
}

ossimImageFileWriter*
ossimKmlSuperOverlayWriterFactory::createWriter(const ossimKeywordlist& kwl,
                                       const char *prefix)const
{
   ossimRefPtr<ossimImageFileWriter> writer = 0;
   const char* type = kwl.find(prefix, ossimKeywordNames::TYPE_KW);
   if (type)
   {
      writer = createWriter(ossimString(type));
      if (writer.valid())
      {
         if (writer->loadState(kwl, prefix) == false)
         {
            writer = 0;
         }
      }
   }
   return writer.release();
}

ossimImageFileWriter* ossimKmlSuperOverlayWriterFactory::createWriter(
   const ossimString& typeName)const
{
   ossimRefPtr<ossimImageFileWriter> writer = 0;
   if (typeName == "ossimKmlSuperOverlayWriter" || typeName == "ossim_kmlsuperoverlay")
   {
      writer = new ossimKmlSuperOverlayWriter;
   }
   return writer.release();
}

ossimObject* ossimKmlSuperOverlayWriterFactory::createObject(
   const ossimKeywordlist& kwl, const char *prefix)const
{
   return createWriter(kwl, prefix);
}

ossimObject* ossimKmlSuperOverlayWriterFactory::createObject(
   const ossimString& typeName) const
{
   return createWriter(typeName);
}

void ossimKmlSuperOverlayWriterFactory::getExtensions(
   std::vector<ossimString>& result)const
{
   result.push_back("kml");
   result.push_back("kmz");
}

void ossimKmlSuperOverlayWriterFactory::getTypeNameList(std::vector<ossimString>& typeList)const
{
   getImageTypeList(typeList);
}

//---
// Adds our writers to the list of writers...
//---
void ossimKmlSuperOverlayWriterFactory::getImageTypeList(std::vector<ossimString>& imageTypeList)const
{
   ossimRefPtr<ossimImageFileWriter> writer = new ossimKmlSuperOverlayWriter;
   writer->getImageTypeList(imageTypeList);
}

ossimKmlSuperOverlayWriterFactory::ossimKmlSuperOverlayWriterFactory(){}

ossimKmlSuperOverlayWriterFactory::ossimKmlSuperOverlayWriterFactory(const ossimKmlSuperOverlayWriterFactory&){}

void ossimKmlSuperOverlayWriterFactory::operator=(const ossimKmlSuperOverlayWriterFactory&){}

