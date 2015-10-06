//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: Factory for OSSIM SQLite writer factory.
//----------------------------------------------------------------------------
// $Id$

#include "ossimSqliteWriterFactory.h"
#include "ossimGpkgWriter.h"
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/imaging/ossimImageFileWriter.h>

ossimSqliteWriterFactory* ossimSqliteWriterFactory::theInstance = 0;

RTTI_DEF1(ossimSqliteWriterFactory,
          "ossimSqliteWriterFactory",
          ossimImageWriterFactoryBase);

ossimSqliteWriterFactory::~ossimSqliteWriterFactory()
{
   theInstance = 0;
}

ossimSqliteWriterFactory* ossimSqliteWriterFactory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimSqliteWriterFactory;
   }
   return theInstance;
}

ossimImageFileWriter* ossimSqliteWriterFactory::createWriterFromExtension(
   const ossimString& fileExtension)const
{
   ossimRefPtr<ossimGpkgWriter> writer = 0;
   if ( (fileExtension == "gpkg") || (fileExtension == ".gpkg") )
   {
      writer = new ossimGpkgWriter();
   }
   return writer.release();
}

ossimImageFileWriter*
ossimSqliteWriterFactory::createWriter(const ossimKeywordlist& kwl,
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

ossimImageFileWriter* ossimSqliteWriterFactory::createWriter(
   const ossimString& typeName)const
{
   ossimRefPtr<ossimImageFileWriter> writer = 0;
   if (typeName == "ossimGpkgWriter")
   {
      writer = new ossimGpkgWriter();
   }
   else
   {
      // See if the type name is supported by the writer.
      writer = new ossimGpkgWriter();
      if ( writer->hasImageType(typeName) == false )
      {
         writer = 0;
      }
   }
   return writer.release();
}

ossimObject* ossimSqliteWriterFactory::createObject(const ossimKeywordlist& kwl,
                                                 const char *prefix)const
{
   return createWriter(kwl, prefix);
}

ossimObject* ossimSqliteWriterFactory::createObject(
   const ossimString& typeName) const
{
   return createWriter(typeName);
}

void ossimSqliteWriterFactory::getExtensions(
   std::vector<ossimString>& result)const
{
   result.push_back("gpkg");
}

void ossimSqliteWriterFactory::getTypeNameList(std::vector<ossimString>& typeList)const
{
   typeList.push_back(ossimString("ossimGpkgWriter"));
}

void ossimSqliteWriterFactory::getImageTypeList(std::vector<ossimString>& imageTypeList)const
{
   ossimRefPtr<ossimGpkgWriter> writer = new ossimGpkgWriter;
   writer->getImageTypeList(imageTypeList);
   writer = 0;
}

void ossimSqliteWriterFactory::getImageFileWritersBySuffix(
   ossimImageWriterFactoryBase::ImageFileWriterList& result, const ossimString& ext)const
{
   ossimString testExt = ext.downcase();
   if(testExt == "gpkg")
   {
      result.push_back(new ossimGpkgWriter);
   }
}

void ossimSqliteWriterFactory::getImageFileWritersByMimeType(
   ossimImageWriterFactoryBase::ImageFileWriterList& result, const ossimString& mimeType)const
{
   ossimString testMime = mimeType.downcase();
   if(testMime == "image/gpkg")
   {
      result.push_back(new ossimGpkgWriter);
   }
}

ossimSqliteWriterFactory::ossimSqliteWriterFactory(){}

ossimSqliteWriterFactory::ossimSqliteWriterFactory(const ossimSqliteWriterFactory&){}

void ossimSqliteWriterFactory::operator=(const ossimSqliteWriterFactory&){}




