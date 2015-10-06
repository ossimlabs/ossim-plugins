//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: Factory for OSSIM SQLite reader factory.
// 
//----------------------------------------------------------------------------
// $Id$

#include "ossimSqliteReaderFactory.h"
#include "ossimGpkgReader.h"
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimString.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimKeywordNames.h>

#include <sqlite3.h>

static const ossimTrace traceDebug("ossimSqliteReaderFactory:debug");

RTTI_DEF1(ossimSqliteReaderFactory,
          "ossimSqliteReaderFactory",
          ossimImageHandlerFactoryBase);

ossimSqliteReaderFactory* ossimSqliteReaderFactory::theInstance = 0;

ossimSqliteReaderFactory::~ossimSqliteReaderFactory()
{
   theInstance = 0;
}

ossimSqliteReaderFactory* ossimSqliteReaderFactory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimSqliteReaderFactory;
   }
   return theInstance;
}
   
ossimImageHandler* ossimSqliteReaderFactory::open(const ossimFilename& file,
                                                  bool openOverview)const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimSqliteReaderFactory::open(filename) DEBUG: entered..."
         << "\ntrying ossimGpkgReader"
         << std::endl;
   }
   ossimRefPtr<ossimImageHandler> reader = 0;
   
   std::string ext = file.ext().downcase().string();
   if ( ext == "gpkg" )
   {
      reader = new ossimGpkgReader;
      reader->setOpenOverviewFlag(openOverview);
      if(reader->open(file) == false)
      {
         reader = 0;
      }
   }
   
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimSqliteReaderFactory::open(filename) DEBUG: leaving..."
         << std::endl;
   }
   
   return reader.release();
}

ossimImageHandler* ossimSqliteReaderFactory::open(const ossimKeywordlist& kwl,
                                               const char* prefix)const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimSqliteReaderFactory::open(kwl, prefix) DEBUG: entered..."
         << "Trying ossimGpkgReader"
         << std::endl;
   }

   ossimRefPtr<ossimImageHandler> reader = new ossimGpkgReader;
   if(reader->loadState(kwl, prefix) == false)
   {
      reader = 0;
   }
   
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimSqliteReaderFactory::open(kwl, prefix) DEBUG: leaving..."
         << std::endl;
   }
   
   return reader.release();
}

ossimObject* ossimSqliteReaderFactory::createObject(
   const ossimString& typeName)const
{
   ossimRefPtr<ossimObject> result = 0;
   if(typeName == "ossimGpkgReader")
   {
      result = new ossimGpkgReader;
   }
   return result.release();
}

ossimObject* ossimSqliteReaderFactory::createObject(const ossimKeywordlist& kwl,
                                                 const char* prefix)const
{
   return this->open(kwl, prefix);
}
 
void ossimSqliteReaderFactory::getTypeNameList(std::vector<ossimString>& typeList)const
{
   typeList.push_back(ossimString("ossimGpkgReader"));
}

void ossimSqliteReaderFactory::getSupportedExtensions(
   ossimImageHandlerFactoryBase::UniqueStringList& extensionList)const
{
   extensionList.push_back(ossimString("gpkg"));
}

void ossimSqliteReaderFactory::getImageHandlersBySuffix(ossimImageHandlerFactoryBase::ImageHandlerList& result,
                                      const ossimString& ext)const
{
   ossimString testExt = ext.downcase();
   if(ext == "gpkg")
   {
      result.push_back(new ossimGpkgReader);
   }
}

void ossimSqliteReaderFactory::getImageHandlersByMimeType(ossimImageHandlerFactoryBase::ImageHandlerList& result,
                                        const ossimString& mimeType)const
{
   ossimString testExt = mimeType.downcase();
   if(testExt == "image/gpkg")
   {
      result.push_back(new ossimGpkgReader);
   }
}

ossimSqliteReaderFactory::ossimSqliteReaderFactory(){}

ossimSqliteReaderFactory::ossimSqliteReaderFactory(const ossimSqliteReaderFactory&){}

void ossimSqliteReaderFactory::operator=(const ossimSqliteReaderFactory&){}
