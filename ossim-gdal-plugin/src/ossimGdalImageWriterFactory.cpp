//*******************************************************************
// Copyright (C) 2000 ImageLinks Inc.
//
// License:  LGPL
//
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Garrett Potts
//
//*******************************************************************
//  $Id: ossimGdalImageWriterFactory.cpp 19908 2011-08-05 19:57:34Z dburken $

#include "ossimGdalImageWriterFactory.h"
#include "ossimGdalWriter.h"
#include <ossim/base/ossimKeywordNames.h>

ossimGdalImageWriterFactory* ossimGdalImageWriterFactory::theInstance = (ossimGdalImageWriterFactory*)NULL;


ossimGdalImageWriterFactory* ossimGdalImageWriterFactory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimGdalImageWriterFactory;
   }

   return theInstance;
}

ossimGdalImageWriterFactory::~ossimGdalImageWriterFactory()
{
   theInstance = (ossimGdalImageWriterFactory*)NULL;
}

ossimImageFileWriter*
ossimGdalImageWriterFactory::createWriter(const ossimKeywordlist& kwl,
                                          const char *prefix)const
{
   ossimString type = kwl.find(prefix, ossimKeywordNames::TYPE_KW);
   ossimImageFileWriter* result = (ossimImageFileWriter*)NULL;
   if(type != "")
   {
      result = createWriter(type);
      if (result)
      {
         if (result->hasImageType(type))
         {
            ossimKeywordlist kwl2(kwl);
            kwl2.add(prefix,
                     ossimKeywordNames::IMAGE_TYPE_KW,
                     type,
                     true);
         
            result->loadState(kwl2, prefix);
         }
         else
         {
            result->loadState(kwl, prefix);
         }
      }
   }

   return result;
}

ossimImageFileWriter* ossimGdalImageWriterFactory::createWriter(const ossimString& typeName)const
{
   ossimRefPtr<ossimGdalWriter> writer = new ossimGdalWriter;
   if (writer->getClassName() == typeName)
   {
      return writer.release();
   }

   if (writer->hasImageType(typeName))
   {
      writer->setOutputImageType(typeName);
      return writer.release();
   }

   writer = 0;
   
   return writer.release();
}

ossimObject* ossimGdalImageWriterFactory::createObject(const ossimKeywordlist& kwl,
                                                   const char *prefix)const
{
   return createWriter(kwl, prefix);
}

ossimObject* ossimGdalImageWriterFactory::createObject(const ossimString& typeName)const
{
   return createWriter(typeName);
}


void ossimGdalImageWriterFactory::getExtensions(std::vector<ossimString>& result)const
{
   result.push_back("img");
   result.push_back("jp2");
   result.push_back("png");
}

void ossimGdalImageWriterFactory::getTypeNameList(std::vector<ossimString>& typeList)const
{
   getImageTypeList(typeList);
}

void ossimGdalImageWriterFactory::getImageTypeList(std::vector<ossimString>& imageTypeList)const
{
   ossimGdalWriter* writer = new ossimGdalWriter;
   writer->getImageTypeList(imageTypeList);
   delete writer;
}

void ossimGdalImageWriterFactory::getImageFileWritersBySuffix(ossimImageWriterFactoryBase::ImageFileWriterList& result,
                                                              const ossimString& ext)const
{
   int c = GDALGetDriverCount();
   int idx = 0;
   for(idx = 0; idx < c; ++idx)
   {
      
      GDALDriverH h = GDALGetDriver(idx);
      if(canWrite(h))
      {
         ossimString  driverName = GDALGetDriverShortName(h);
         driverName = "gdal_" + driverName.upcase();
         ossimString metaData(GDALGetMetadataItem(h, GDAL_DMD_EXTENSION, 0));
         if(!metaData.empty())
         {
            std::vector<ossimString> splitArray;
            metaData.split(splitArray, " /");
            
            ossim_uint32 idxExtension = 0;
            
            for(idxExtension = 0; idxExtension < splitArray.size(); ++idxExtension)
            {
               if(ext == splitArray[idxExtension])
               {
                  ossimGdalWriter* writer = new ossimGdalWriter;
                  writer->setOutputImageType(driverName);
                  result.push_back(writer);
                  if ( driverName == "gdal_JP2KAK" )
                  {
                     // Make it lossless for starters.  User can still override.
                     ossimKeywordlist kwl;
                     kwl.add("property0.name", "QUALITY");
                     kwl.add("property0.value", "100");
                     writer->loadState(kwl, NULL);
                  }
                  return;
               }
            }
         }
      }
   }
}

void ossimGdalImageWriterFactory::getImageFileWritersByMimeType(ossimImageWriterFactoryBase::ImageFileWriterList& result,
                                                                const ossimString& mimeType)const
{
   int c = GDALGetDriverCount();
   int idx = 0;
   for(idx = 0; idx < c; ++idx)
   {
      
      GDALDriverH h = GDALGetDriver(idx);
      if(canWrite(h))
      {
         ossimString  driverName = GDALGetDriverShortName(h);
         driverName = "gdal_" + driverName.upcase();
         ossimString metaData(GDALGetMetadataItem(h, GDAL_DMD_MIMETYPE, 0));
         if(!metaData.empty())
         {
            if(metaData == mimeType)
            {
               ossimGdalWriter* writer = new ossimGdalWriter;
               writer->setOutputImageType(driverName);
               result.push_back(writer);
               if ( driverName == "gdal_JP2KAK" )
               {
                  // Make it lossless for starters.  User can still override.
                  ossimKeywordlist kwl;
                  kwl.add("property0.name", "QUALITY");
                  kwl.add("property0.value", "100");
                  writer->loadState(kwl, NULL);
               }
               return;
            }
         }
      }
   }
}

bool ossimGdalImageWriterFactory::canWrite(GDALDatasetH handle)const
{
   return ( GDALGetMetadataItem(handle, GDAL_DCAP_CREATE, 0)||  GDALGetMetadataItem(handle, GDAL_DCAP_CREATECOPY, 0));
}

