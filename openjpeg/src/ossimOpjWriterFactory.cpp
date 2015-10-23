//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: Factory for OSSIM Open JPEG writer.
//----------------------------------------------------------------------------
// $Id: ossimOpjWriterFactory.cpp 11046 2007-05-25 18:03:03Z gpotts $

#include "ossimOpjWriterFactory.h"
#include "ossimOpjJp2Writer.h"
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/imaging/ossimImageFileWriter.h>

ossimOpjWriterFactory* ossimOpjWriterFactory::theInstance = NULL;

RTTI_DEF1(ossimOpjWriterFactory,
          "ossimOpjWriterFactory",
          ossimImageWriterFactoryBase);

ossimOpjWriterFactory::~ossimOpjWriterFactory()
{
   theInstance = 0;
}

ossimOpjWriterFactory* ossimOpjWriterFactory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimOpjWriterFactory;
   }
   return theInstance;
}

ossimImageFileWriter *ossimOpjWriterFactory::createWriterFromExtension(
   const ossimString& fileExtension)const
{
   ossimOpjJp2Writer* writer = 0;
   if ( (fileExtension == "jp2") || (fileExtension == "JP2") || (fileExtension == "j2k"))
   {
      writer = new ossimOpjJp2Writer();
   }
   return writer;
}

ossimImageFileWriter* ossimOpjWriterFactory::createWriter(
   const ossimKeywordlist& kwl, const char *prefix)const
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

ossimImageFileWriter* ossimOpjWriterFactory::createWriter(
   const ossimString& typeName)const
{
   ossimRefPtr<ossimImageFileWriter> writer = 0;
   if (typeName == "ossimOpjJp2Writer")
   {
      writer = new ossimOpjJp2Writer;
   }
   else
   {
      // See if the type name is supported by the writer.
      writer = new ossimOpjJp2Writer(typeName);
      if ( writer->hasImageType(typeName) == false )
      {
         writer = 0;
      }
   }
   return writer.release();
}

ossimObject* ossimOpjWriterFactory::createObject(
   const ossimKeywordlist& kwl, const char *prefix)const
{
   return createWriter(kwl, prefix);
}

ossimObject* ossimOpjWriterFactory::createObject(
   const ossimString& typeName) const
{
   return createWriter(typeName);
}

void ossimOpjWriterFactory::getExtensions(
   std::vector<ossimString>& result)const
{
   result.push_back("jp2");
   result.push_back("j2k");   
}

void ossimOpjWriterFactory::getTypeNameList(std::vector<ossimString>& typeList) const
{
   typeList.push_back(ossimString("ossimOpjJp2Writer"));
}

void ossimOpjWriterFactory::getImageTypeList(std::vector<ossimString>& imageTypeList) const
{
   // include both geotiff and gmljp2 headers
   imageTypeList.push_back( ossimString("ossim_opj_jp2") );

   // include only a geotiff header
   imageTypeList.push_back( ossimString("ossim_opj_geojp2") ); 

   // include only a gmljp2 header
   imageTypeList.push_back( ossimString("ossim_opj_gmljp2") ); 
}

void ossimOpjWriterFactory::getImageFileWritersBySuffix(
   ossimImageWriterFactoryBase::ImageFileWriterList& result,
   const ossimString& ext) const
{
   ossimString testExt = ext.downcase();
   if ( (testExt == "jp2") || (testExt == "j2k") )
   {
      result.push_back(new ossimOpjJp2Writer);
   }
}

void ossimOpjWriterFactory::getImageFileWritersByMimeType(
   ossimImageWriterFactoryBase::ImageFileWriterList& result,
   const ossimString& mimeType)const
{
   if ( (mimeType == "image/jp2") || (mimeType == "image/j2k") )
   {
      result.push_back( new ossimOpjJp2Writer() );
   }
}

ossimOpjWriterFactory::ossimOpjWriterFactory(){}

ossimOpjWriterFactory::ossimOpjWriterFactory(const ossimOpjWriterFactory&){}

void ossimOpjWriterFactory::operator=(const ossimOpjWriterFactory&){}




