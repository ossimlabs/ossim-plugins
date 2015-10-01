//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Mingjie Su
//
// Description: Factory for OSSIM MrSID writers.
//----------------------------------------------------------------------------
// $Id: ossimMrSidWriterFactory.cpp 899 2010-05-17 21:00:26Z david.burken $

#ifdef OSSIM_ENABLE_MRSID_WRITE

#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/imaging/ossimImageFileWriter.h>

#include "ossimMrSidWriterFactory.h"
#include "ossimMrSidWriter.h"

ossimMrSidWriterFactory* ossimMrSidWriterFactory::theInstance = 0;

RTTI_DEF1(ossimMrSidWriterFactory,
          "ossimMrSidWriterFactory",
          ossimImageWriterFactoryBase);

ossimMrSidWriterFactory::~ossimMrSidWriterFactory()
{
   theInstance = 0;
}

ossimMrSidWriterFactory* ossimMrSidWriterFactory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimMrSidWriterFactory;
   }
   return theInstance;
}

ossimImageFileWriter* ossimMrSidWriterFactory::createWriterFromExtension(
   const ossimString& fileExtension)const
{
   ossimImageFileWriter* writer = 0;

   if(fileExtension == "sid")
   {
      writer = new ossimMrSidWriter;
   }
   return writer;
}

ossimImageFileWriter*
ossimMrSidWriterFactory::createWriter(const ossimKeywordlist& kwl,
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

ossimImageFileWriter* ossimMrSidWriterFactory::createWriter(
   const ossimString& typeName)const
{
   ossimRefPtr<ossimImageFileWriter> writer = 0;
   if (typeName == "ossimMrSidWriter" || typeName == "ossim_mrsid")
   {
      writer = new ossimMrSidWriter;
   }
   return writer.release();
}

ossimObject* ossimMrSidWriterFactory::createObject(
   const ossimKeywordlist& kwl, const char *prefix)const
{
   return createWriter(kwl, prefix);
}

ossimObject* ossimMrSidWriterFactory::createObject(
   const ossimString& typeName) const
{
   return createWriter(typeName);
}

void ossimMrSidWriterFactory::getExtensions(
   std::vector<ossimString>& result)const
{
   result.push_back("sid");
}

void ossimMrSidWriterFactory::getTypeNameList(std::vector<ossimString>& typeList)const
{
   getImageTypeList(typeList);
}

//---
// Adds our writers to the list of writers...
//---
void ossimMrSidWriterFactory::getImageTypeList(std::vector<ossimString>& imageTypeList)const
{
   ossimRefPtr<ossimImageFileWriter> writer = new ossimMrSidWriter;
   writer->getImageTypeList(imageTypeList);
}

void ossimMrSidWriterFactory::getImageFileWritersBySuffix(
   ossimImageWriterFactoryBase::ImageFileWriterList& result, const ossimString& ext)const
{
   ossimString testExt = ext.downcase();
   if(testExt == "sid")
   {
      result.push_back(new ossimMrSidWriter);
   }
}

void ossimMrSidWriterFactory::getImageFileWritersByMimeType(
   ossimImageWriterFactoryBase::ImageFileWriterList& result, const ossimString& mimeType)const
{
   ossimString testMime = mimeType.downcase();
   if(testMime == "image/sid")
   {
      result.push_back(new ossimMrSidWriter);
   }
}

ossimMrSidWriterFactory::ossimMrSidWriterFactory(){}

ossimMrSidWriterFactory::ossimMrSidWriterFactory(const ossimMrSidWriterFactory&){}

void ossimMrSidWriterFactory::operator=(const ossimMrSidWriterFactory&){}

#endif /* #if OSSIM_ENABLE_MRSID_WRITE */
