//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  David Burken
//
// Description: Factory for OSSIM Kakadu writers.
//----------------------------------------------------------------------------
// $Id: ossimKakaduWriterFactory.cpp 23089 2015-01-21 21:01:22Z dburken $

#include "ossimKakaduWriterFactory.h"
#include "ossimKakaduJp2Writer.h"
#include "ossimKakaduNitfWriter.h"
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/imaging/ossimImageFileWriter.h>

RTTI_DEF1(ossimKakaduWriterFactory,
          "ossimKakaduWriterFactory",
          ossimImageWriterFactoryBase);

ossimKakaduWriterFactory::~ossimKakaduWriterFactory()
{
}

ossimKakaduWriterFactory* ossimKakaduWriterFactory::instance()
{
   static ossimKakaduWriterFactory inst;
   return &inst;
}

ossimImageFileWriter* ossimKakaduWriterFactory::createWriterFromExtension(
   const ossimString& fileExtension)const
{
   ossimImageFileWriter* writer = 0;

#if 0 /* Can't use as we don't know if the user wants a compressed nitf. */
   if( (fileExtension == "ntf") || (fileExtension == "nitf") )
   {
      writer = new ossimKakaduNitfWriter;
   }
#endif

   if(fileExtension == "jp2")
   {
      writer = new ossimKakaduJp2Writer;
   }

   return writer;
}

ossimImageFileWriter*
ossimKakaduWriterFactory::createWriter(const ossimKeywordlist& kwl,
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

ossimImageFileWriter* ossimKakaduWriterFactory::createWriter(
   const ossimString& typeName)const
{
   ossimRefPtr<ossimImageFileWriter> writer = 0;
   if ( (typeName == "ossim_kakadu_nitf_j2k") ||
        ( typeName == "ossimKakaduNitfWriter") )
   {
      writer = new ossimKakaduNitfWriter;
   }
   else if ( (typeName == "ossim_kakadu_jp2") ||
             (typeName == "ossimKakaduJp2Writer") )
   {
      writer = new ossimKakaduJp2Writer;
   }
   else
   {
      // See if the type name is supported by the writer.

      //---
      // Note: Will not do ossimKakaduNitfWriter here as user may want a
      // non-licensed solution.
      //---

      writer = new ossimKakaduJp2Writer;
      if ( writer->hasImageType(typeName) == false )
      {
         writer = 0;
      }
   }
   return writer.release();
}

ossimObject* ossimKakaduWriterFactory::createObject(
   const ossimKeywordlist& kwl, const char *prefix)const
{
   return createWriter(kwl, prefix);
}

ossimObject* ossimKakaduWriterFactory::createObject(
   const ossimString& typeName) const
{
   return createWriter(typeName);
}

void ossimKakaduWriterFactory::getExtensions(
   std::vector<ossimString>& result)const
{
   result.push_back("jp2");
   result.push_back("ntf");
}

void ossimKakaduWriterFactory::getTypeNameList(std::vector<ossimString>& typeList)const
{
   typeList.push_back(ossimString("ossimKakaduNitfWriter"));
   typeList.push_back(ossimString("ossimKakaduJp2Writer"));
}

//---
// Adds our writers to the list of writers...
//---
void ossimKakaduWriterFactory::getImageTypeList(std::vector<ossimString>& imageTypeList)const
{
   imageTypeList.push_back( ossimString("ossim_kakadu_nitf_j2k") );
   imageTypeList.push_back( ossimString("ossim_kakadu_jp2") );
}

void ossimKakaduWriterFactory::getImageFileWritersBySuffix(
   ossimImageWriterFactoryBase::ImageFileWriterList& result,
   const ossimString& ext) const
{
   ossimString testExt = ext.downcase();
   if(testExt == "jp2")
   {
      result.push_back(new ossimKakaduJp2Writer);
   }
   else if(testExt == "ntf")
   {
      result.push_back(new ossimKakaduNitfWriter);
   }
}

void ossimKakaduWriterFactory::getImageFileWritersByMimeType(ossimImageWriterFactoryBase::ImageFileWriterList& result,
                                           const ossimString& mimeType)const
{
   if(mimeType == "image/jp2")
   {
      result.push_back(new ossimKakaduJp2Writer);
   }
}

ossimKakaduWriterFactory::ossimKakaduWriterFactory(){}

ossimKakaduWriterFactory::ossimKakaduWriterFactory(const ossimKakaduWriterFactory&){}

void ossimKakaduWriterFactory::operator=(const ossimKakaduWriterFactory&){}




