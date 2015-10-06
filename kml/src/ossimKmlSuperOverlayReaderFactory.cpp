//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  Mingjie Su, Harsh Govind
//
// Description: Factory for ossim KmlSuperOverlay reader using libkml library.
//----------------------------------------------------------------------------
// $Id: ossimKmlSuperOverlayReaderFactory.cpp 2472 2011-04-26 15:47:50Z ming.su $


#include "ossimKmlSuperOverlayReaderFactory.h"
#include "ossimKmlSuperOverlayReader.h"

#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/imaging/ossimImageHandler.h>

static const ossimTrace traceDebug("ossimKmlSuperOverlayReaderFactory:debug");

class ossimImageHandler;

RTTI_DEF1(ossimKmlSuperOverlayReaderFactory,
   "ossimKmlSuperOverlayReaderFactory",
   ossimImageHandlerFactoryBase);

ossimKmlSuperOverlayReaderFactory* ossimKmlSuperOverlayReaderFactory::theInstance = 0;

ossimKmlSuperOverlayReaderFactory::~ossimKmlSuperOverlayReaderFactory()
{
   theInstance = 0;
}

ossimKmlSuperOverlayReaderFactory* ossimKmlSuperOverlayReaderFactory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimKmlSuperOverlayReaderFactory;
   }
   return theInstance;
}

ossimImageHandler* ossimKmlSuperOverlayReaderFactory::open(const ossimFilename& fileName,
                                                           bool /* openOverview */ )const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimKmlSuperOverlayReaderFactory::open(filename) DEBUG: entered..."
         << "\ntrying ossimKakaduNitfReader"
         << std::endl;
   }

   ossimRefPtr<ossimImageHandler> reader = 0;

   if ( hasExcludedExtension(fileName) == false )
   {
      reader = new ossimKmlSuperOverlayReader;
      if(reader->open(fileName) == false)
      {
         reader = 0;
      }
   }

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimKmlSuperOverlayReaderFactory::open(filename) DEBUG: leaving..."
         << std::endl;
   }

   return reader.release();
}

ossimImageHandler* ossimKmlSuperOverlayReaderFactory::open(const ossimKeywordlist& kwl,
   const char* prefix)const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimKmlSuperOverlayReaderFactory::open(kwl, prefix) DEBUG: entered..."
         << "Trying ossimKakaduNitfReader"
         << std::endl;
   }

   ossimRefPtr<ossimImageHandler> reader = new ossimKmlSuperOverlayReader;
   if(reader->loadState(kwl, prefix) == false)
   {
      reader = 0;
   }

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimKmlSuperOverlayReaderFactory::open(kwl, prefix) DEBUG: leaving..."
         << std::endl;
   }

   return reader.release();
}

ossimObject* ossimKmlSuperOverlayReaderFactory::createObject(
   const ossimString& typeName)const
{
   ossimRefPtr<ossimObject> result = 0;
   if(typeName == "ossimKmlSuperOverlayReader")
   {
      result = new ossimKmlSuperOverlayReader;
   }

   return result.release();
}

ossimObject* ossimKmlSuperOverlayReaderFactory::createObject(
   const ossimKeywordlist& kwl,
   const char* prefix)const
{
   return this->open(kwl, prefix);
}

void ossimKmlSuperOverlayReaderFactory::getTypeNameList(
   std::vector<ossimString>& typeList)const
{
   typeList.push_back(ossimString("ossimKmlSuperOverlayReader"));
}

void ossimKmlSuperOverlayReaderFactory::getSupportedExtensions(
   ossimImageHandlerFactoryBase::UniqueStringList& extensionList)const
{
   extensionList.push_back(ossimString("kml"));
   extensionList.push_back(ossimString("kmz"));
}

bool ossimKmlSuperOverlayReaderFactory::hasExcludedExtension(
   const ossimFilename& file) const
{
   bool result = true;
   ossimString ext = file.ext().downcase();
   if (ext == "kml" || ext == "kmz") //only include the file with .kml or .kmz extension and exclude any other files
   {
      result = false;
   }
   return result;
}

ossimKmlSuperOverlayReaderFactory::ossimKmlSuperOverlayReaderFactory(){}

ossimKmlSuperOverlayReaderFactory::ossimKmlSuperOverlayReaderFactory(const ossimKmlSuperOverlayReaderFactory&){}

void ossimKmlSuperOverlayReaderFactory::operator=(const ossimKmlSuperOverlayReaderFactory&){}
