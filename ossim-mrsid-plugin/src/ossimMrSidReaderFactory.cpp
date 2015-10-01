//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  Mingjie Su
//
// Description: Factory for OSSIM MrSID reader using kakadu library.
//----------------------------------------------------------------------------
// $Id: ossimMrSidReaderFactory.cpp 469 2009-12-23 18:52:47Z ming.su $

#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/imaging/ossimImageHandler.h>

#include "ossimMrSidReaderFactory.h"
#include "ossimMrSidReader.h"
#include "ossimMG4LidarReader.h"

static const ossimTrace traceDebug("ossimMrSidReaderFactory:debug");

class ossimImageHandler;

RTTI_DEF1(ossimMrSidReaderFactory,
          "ossimMrSidReaderFactory",
          ossimImageHandlerFactoryBase);

ossimMrSidReaderFactory* ossimMrSidReaderFactory::theInstance = 0;

ossimMrSidReaderFactory::~ossimMrSidReaderFactory()
{
   theInstance = 0;
}

ossimMrSidReaderFactory* ossimMrSidReaderFactory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimMrSidReaderFactory;
   }
   return theInstance;
}

ossimImageHandler* ossimMrSidReaderFactory::open(
   const ossimFilename& fileName, bool openOverview)const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimMrSidReaderFactory::open(filename) DEBUG: entered...\n";
   }

   ossimRefPtr<ossimImageHandler> reader = 0;

   if ( hasExcludedExtension(fileName) == false )
   {
      if(traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG) << "\nTrying ossimMrSidReader\n";
      }
      reader = new ossimMrSidReader;
      reader->setOpenOverviewFlag(openOverview);
      if(reader->open(fileName) == false)
      {
         reader = 0;
      }

      if (!reader.valid())
      {
         reader = new ossimMG4LidarReader;
         reader->setOpenOverviewFlag(openOverview);
         if(reader->open(fileName) == false)
         {
            reader = 0;
         }
      }
   }

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimMrSidReaderFactory::open(filename) DEBUG: leaving...\n";
   }

   return reader.release();
}

ossimImageHandler* ossimMrSidReaderFactory::open(const ossimKeywordlist& kwl,
                                                 const char* prefix)const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimMrSidReaderFactory::open(kwl, prefix) DEBUG: entered..."
         << "Trying ossimMrSidReader..."
         << std::endl;
   }

   ossimRefPtr<ossimImageHandler> reader = new ossimMrSidReader;
   if(reader->loadState(kwl, prefix) == false)
   {
      reader = 0;
   }

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimMrSidReaderFactory::open(kwl, prefix) DEBUG: leaving..."
         << std::endl;
   }

   return reader.release();
}

ossimObject* ossimMrSidReaderFactory::createObject(
   const ossimString& typeName)const
{
   ossimRefPtr<ossimObject> result = 0;
   if(typeName == "ossimMrSidReader")
   {
      result = new ossimMrSidReader;
   }
   else if(typeName == "ossimMG4LidarReader")
   {
      result = new ossimMrSidReader;
   }

   return result.release();
}

ossimObject* ossimMrSidReaderFactory::createObject(
   const ossimKeywordlist& kwl,
   const char* prefix)const
{
   return this->open(kwl, prefix);
}

void ossimMrSidReaderFactory::getTypeNameList(
   std::vector<ossimString>& typeList)const
{
   typeList.push_back(ossimString("ossimMrSidReader"));
}

void ossimMrSidReaderFactory::getSupportedExtensions(
   ossimImageHandlerFactoryBase::UniqueStringList& extensionList)const
{
   extensionList.push_back(ossimString("sid"));
}

void ossimMrSidReaderFactory::getImageHandlersBySuffix(
   ossimImageHandlerFactoryBase::ImageHandlerList& result, const ossimString& ext)const
{
   ossimString testExt = ext.downcase();
   if(ext == "sid")
   {
      result.push_back(new ossimMrSidReader);
   }
}

void ossimMrSidReaderFactory::getImageHandlersByMimeType(
   ossimImageHandlerFactoryBase::ImageHandlerList& result, const ossimString& mimeType)const
{
   ossimString testExt = mimeType.downcase();
   if(testExt == "image/sid")
   {
      result.push_back(new ossimMrSidReader);
   }
}

bool ossimMrSidReaderFactory::hasExcludedExtension(
   const ossimFilename& file) const
{
   bool result = true;
   ossimString ext = file.ext().downcase();
   if (ext == "sid") //only include the file with .sid extension and exclude any other files
   {
      result = false;
   }
   return result;
}

ossimMrSidReaderFactory::ossimMrSidReaderFactory(){}

ossimMrSidReaderFactory::ossimMrSidReaderFactory(const ossimMrSidReaderFactory&){}

void ossimMrSidReaderFactory::operator=(const ossimMrSidReaderFactory&){}
