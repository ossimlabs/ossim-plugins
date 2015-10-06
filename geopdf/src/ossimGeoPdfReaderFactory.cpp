//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  Mingjie Su
//
// Description: Factory for OSSIM GeoPdf reader using kakadu library.
//----------------------------------------------------------------------------
// $Id: ossimGeoPdfReaderFactory.cpp 20935 2012-05-18 14:19:30Z dburken $

#include "ossimGeoPdfReaderFactory.h"
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/imaging/ossimImageHandler.h>
#include "ossimGeoPdfReader.h"

static const ossimTrace traceDebug("ossimGeoPdfReaderFactory:debug");

class ossimImageHandler;

RTTI_DEF1(ossimGeoPdfReaderFactory,
   "ossimGeoPdfReaderFactory",
   ossimImageHandlerFactoryBase);

ossimGeoPdfReaderFactory* ossimGeoPdfReaderFactory::theInstance = 0;

ossimGeoPdfReaderFactory::~ossimGeoPdfReaderFactory()
{
   theInstance = 0;
}

ossimGeoPdfReaderFactory* ossimGeoPdfReaderFactory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimGeoPdfReaderFactory;
   }
   return theInstance;
}

ossimImageHandler* ossimGeoPdfReaderFactory::open(const ossimFilename& fileName,
   bool openOverview)const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGeoPdfReaderFactory::open(filename) DEBUG: entered..."
         << "\ntrying ossimKakaduNitfReader"
         << std::endl;
   }

   ossimRefPtr<ossimImageHandler> reader = 0;

   if ( hasExcludedExtension(fileName) == false )
   {
      reader = new ossimGeoPdfReader;
      reader->setOpenOverviewFlag(openOverview);
      if(reader->open(fileName) == false)
      {
         reader = 0;
      }
   }

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGeoPdfReaderFactory::open(filename) DEBUG: leaving..."
         << std::endl;
   }

   return reader.release();
}

ossimImageHandler* ossimGeoPdfReaderFactory::open(const ossimKeywordlist& kwl,
   const char* prefix)const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGeoPdfReaderFactory::open(kwl, prefix) DEBUG: entered..."
         << "Trying ossimKakaduNitfReader"
         << std::endl;
   }

   ossimRefPtr<ossimImageHandler> reader = new ossimGeoPdfReader;
   if(reader->loadState(kwl, prefix) == false)
   {
      reader = 0;
   }

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGeoPdfReaderFactory::open(kwl, prefix) DEBUG: leaving..."
         << std::endl;
   }

   return reader.release();
}

ossimObject* ossimGeoPdfReaderFactory::createObject(
   const ossimString& typeName)const
{
   ossimRefPtr<ossimObject> result = 0;
   if(typeName == "ossimGeoPdfReader")
   {
      result = new ossimGeoPdfReader;
   }

   return result.release();
}

ossimObject* ossimGeoPdfReaderFactory::createObject(
   const ossimKeywordlist& kwl,
   const char* prefix)const
{
   return this->open(kwl, prefix);
}

void ossimGeoPdfReaderFactory::getTypeNameList(
   std::vector<ossimString>& typeList)const
{
   typeList.push_back(ossimString("ossimGeoPdfReader"));
}

void ossimGeoPdfReaderFactory::getSupportedExtensions(
   ossimImageHandlerFactoryBase::UniqueStringList& extensionList)const
{
   extensionList.push_back(ossimString("pdf"));
}

bool ossimGeoPdfReaderFactory::hasExcludedExtension(
   const ossimFilename& file) const
{
   bool result = true;
   ossimString ext = file.ext().downcase();
   if (ext == "pdf") //only include the file with .pdf extension and exclude any other files
   {
      result = false;
   }
   return result;
}

void ossimGeoPdfReaderFactory::getImageHandlersBySuffix(ossimImageHandlerFactoryBase::ImageHandlerList& result,
   const ossimString& ext)const
{
   ossimString testExt = ext.downcase();
   if(ext == "pdf")
   {
      result.push_back(new ossimGeoPdfReader);
   }
}

void ossimGeoPdfReaderFactory::getImageHandlersByMimeType(ossimImageHandlerFactoryBase::ImageHandlerList& result,
   const ossimString& mimeType)const
{
   ossimString testExt = mimeType.downcase();
   if(testExt == "image/pdf")
   {
      result.push_back(new ossimGeoPdfReader);
   }
}

ossimGeoPdfReaderFactory::ossimGeoPdfReaderFactory(){}

ossimGeoPdfReaderFactory::ossimGeoPdfReaderFactory(const ossimGeoPdfReaderFactory&){}

void ossimGeoPdfReaderFactory::operator=(const ossimGeoPdfReaderFactory&){}


