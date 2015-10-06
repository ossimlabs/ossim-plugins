//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: Factory for OSSIM Portable Network Graphics (PNG) reader
// (tile source).
//----------------------------------------------------------------------------
// $Id: ossimPngReaderFactory.cpp 22633 2014-02-20 00:57:42Z dburken $

#include "ossimPngReaderFactory.h"
#include "ossimPngReader.h"
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimString.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimKeywordNames.h>

static const ossimTrace traceDebug("ossimPngReaderFactory:debug");

RTTI_DEF1(ossimPngReaderFactory,
          "ossimPngReaderFactory",
          ossimImageHandlerFactoryBase);

ossimPngReaderFactory* ossimPngReaderFactory::theInstance = 0;

ossimPngReaderFactory::~ossimPngReaderFactory()
{
   theInstance = 0;
}

ossimPngReaderFactory* ossimPngReaderFactory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimPngReaderFactory;
   }
   return theInstance;
}
   
ossimImageHandler* ossimPngReaderFactory::open(const ossimFilename& fileName,
                                               bool openOverview)const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimPngReaderFactory::open(filename) DEBUG: entered..."
         << "\ntrying ossimPngReader"
         << std::endl;
   }
   
   ossimRefPtr<ossimImageHandler> reader = new ossimPngReader;
   reader->setOpenOverviewFlag(openOverview);
   if(reader->open(fileName) == false)
   {
      reader = 0;
   }
   
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimPngReaderFactory::open(filename) DEBUG: leaving..."
         << std::endl;
   }
   
   return reader.release();
}

ossimImageHandler* ossimPngReaderFactory::open(const ossimKeywordlist& kwl,
                                               const char* prefix)const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimPngReaderFactory::open(kwl, prefix) DEBUG: entered..."
         << "Trying ossimPngReader"
         << std::endl;
   }

   ossimRefPtr<ossimImageHandler> reader = new ossimPngReader();
   if(reader->loadState(kwl, prefix) == false)
   {
      reader = 0;
   }
   
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimPngReaderFactory::open(kwl, prefix) DEBUG: leaving..."
         << std::endl;
   }
   
   return reader.release();
}

ossimRefPtr<ossimImageHandler> ossimPngReaderFactory::open(
   std::istream* str, std::streamoff restartPosition, bool youOwnIt ) const
{
   ossimRefPtr<ossimImageHandler> result = 0;
   
   ossimRefPtr<ossimPngReader> reader = new ossimPngReader();
   if ( reader->open( str, restartPosition, youOwnIt ) )
   {
      result = reader.get();
   }
   
   return result;
}

ossimObject* ossimPngReaderFactory::createObject(
   const ossimString& typeName)const
{
   ossimRefPtr<ossimObject> result = 0;
   if(typeName == "ossimPngReader")
   {
      result = new ossimPngReader;
   }
   return result.release();
}

ossimObject* ossimPngReaderFactory::createObject(const ossimKeywordlist& kwl,
                                                 const char* prefix)const
{
   return this->open(kwl, prefix);
}
 
void ossimPngReaderFactory::getTypeNameList(std::vector<ossimString>& typeList)const
{
   typeList.push_back(ossimString("ossimPngReader"));
}

void ossimPngReaderFactory::getSupportedExtensions(
   ossimImageHandlerFactoryBase::UniqueStringList& extensionList)const
{
   extensionList.push_back(ossimString("png"));
}

void ossimPngReaderFactory::getImageHandlersBySuffix(ossimImageHandlerFactoryBase::ImageHandlerList& result,
                                      const ossimString& ext)const
{
   ossimString testExt = ext.downcase();
   if(ext == "png")
   {
      result.push_back(new ossimPngReader);
   }
}

void ossimPngReaderFactory::getImageHandlersByMimeType(ossimImageHandlerFactoryBase::ImageHandlerList& result,
                                        const ossimString& mimeType)const
{
   ossimString testExt = mimeType.downcase();
   if(testExt == "image/png")
   {
      result.push_back(new ossimPngReader);
   }
}

ossimPngReaderFactory::ossimPngReaderFactory(){}

ossimPngReaderFactory::ossimPngReaderFactory(const ossimPngReaderFactory&){}

void ossimPngReaderFactory::operator=(const ossimPngReaderFactory&){}
