//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: Factory for OSSIM Portable Network Graphics (PNG) writer.
//----------------------------------------------------------------------------
// $Id: ossimPngWriterFactory.cpp 18003 2010-08-30 18:02:52Z gpotts $

#include "ossimPngWriterFactory.h"
#include "ossimPngWriter.h"
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/imaging/ossimImageFileWriter.h>

ossimPngWriterFactory* ossimPngWriterFactory::theInstance = 0;

RTTI_DEF1(ossimPngWriterFactory,
          "ossimPngWriterFactory",
          ossimImageWriterFactoryBase);

ossimPngWriterFactory::~ossimPngWriterFactory()
{
   theInstance = 0;
}

ossimPngWriterFactory* ossimPngWriterFactory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimPngWriterFactory;
   }
   return theInstance;
}

ossimImageFileWriter *ossimPngWriterFactory::createWriterFromExtension(
   const ossimString& fileExtension)const
{
   ossimRefPtr<ossimPngWriter> writer = 0;
   if ( (fileExtension == "png") || (fileExtension == ".png") )
   {
      writer = new ossimPngWriter;
   }
   return writer.release();
}

ossimImageFileWriter*
ossimPngWriterFactory::createWriter(const ossimKeywordlist& kwl,
                                    const char *prefix)const
{
   ossimRefPtr<ossimImageFileWriter> writer = 0;
   const char* type = kwl.find(prefix, ossimKeywordNames::TYPE_KW);
   if (type)
   {
      writer = createWriter(ossimString(type));
      if ( writer.valid() )
      {
         if (writer->loadState(kwl, prefix) == false)
         {
            writer = 0;
         }
      }
   }
   return writer.release();
}

ossimImageFileWriter* ossimPngWriterFactory::createWriter(
   const ossimString& typeName)const
{
   ossimRefPtr<ossimImageFileWriter> writer = 0;
   if (typeName == "ossimPngWriter")
   {
      writer = new ossimPngWriter;
   }
   else
   {
      // See if the type name is supported by the writer.
      writer = new ossimPngWriter;
      if ( writer->hasImageType(typeName) == false )
      {
         writer = 0;
      }
   }
   return writer.release();
}

ossimObject* ossimPngWriterFactory::createObject(const ossimKeywordlist& kwl,
                                                 const char *prefix)const
{
   return createWriter(kwl, prefix);
}

ossimObject* ossimPngWriterFactory::createObject(
   const ossimString& typeName) const
{
   return createWriter(typeName);
}

void ossimPngWriterFactory::getExtensions(
   std::vector<ossimString>& result)const
{
   result.push_back("png");
}

void ossimPngWriterFactory::getTypeNameList(std::vector<ossimString>& typeList)const
{
   typeList.push_back(ossimString("ossimPngWriter"));
}

void ossimPngWriterFactory::getImageTypeList(std::vector<ossimString>& imageTypeList)const
{
   ossimRefPtr<ossimPngWriter> writer = new ossimPngWriter;
   writer->getImageTypeList(imageTypeList);
   writer = 0;
}

void ossimPngWriterFactory::getImageFileWritersBySuffix(ossimImageWriterFactoryBase::ImageFileWriterList& result,
                                                        const ossimString& ext)const
{
   ossimString testExt = ext.downcase();
   if(testExt == "png")
   {
      result.push_back(new ossimPngWriter);
   }
}

void ossimPngWriterFactory::getImageFileWritersByMimeType(ossimImageWriterFactoryBase::ImageFileWriterList& result,
                                                          const ossimString& mimeType)const
{
   ossimString testMime = mimeType.downcase();
   if(testMime == "image/png")
   {
      result.push_back(new ossimPngWriter);
   }
}

ossimPngWriterFactory::ossimPngWriterFactory(){}

ossimPngWriterFactory::ossimPngWriterFactory(const ossimPngWriterFactory&){}

void ossimPngWriterFactory::operator=(const ossimPngWriterFactory&){}




