//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author: David Burken
//
// Copied from Mingjie Su's ossimHdfReaderFactory.
//
// Description: Factory for OSSIM HDF5 reader using HDF5 libraries.
//----------------------------------------------------------------------------
// $Id: ossimH5ReaderFactory.cpp 2645 2011-05-26 15:21:34Z oscar.kramer $

#include "ossimH5ReaderFactory.h"
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/imaging/ossimImageHandler.h>
#include "ossimH5Reader.h"

static const ossimTrace traceDebug("ossimH5ReaderFactory:debug");

class ossimImageHandler;

RTTI_DEF1(ossimH5ReaderFactory,
          "ossimH5ReaderFactory",
          ossimImageHandlerFactoryBase);

ossimH5ReaderFactory* ossimH5ReaderFactory::theInstance = 0;

ossimH5ReaderFactory::~ossimH5ReaderFactory()
{
   theInstance = 0;
}

ossimH5ReaderFactory* ossimH5ReaderFactory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimH5ReaderFactory;
   }
   return theInstance;
}

ossimImageHandler* ossimH5ReaderFactory::open(const ossimFilename& fileName,
                                              bool openOverview)const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimH5ReaderFactory::open(filename) DEBUG: entered..."
         << "\ntrying ossimH5Reader"
         << std::endl;
   }

   ossimRefPtr<ossimImageHandler> reader = 0;

   if ( hasExcludedExtension(fileName) == false )
   {
      if (reader == 0) //try hdf5 reader
      {
         // cout << "Calling ossimH5Reader ***********************" << endl;
         reader = new ossimH5Reader;
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
         << "ossimH5ReaderFactory::open(filename) DEBUG: leaving..."
         << std::endl;
   }

   return reader.release();
}

ossimImageHandler* ossimH5ReaderFactory::open(const ossimKeywordlist& kwl,
                                              const char* prefix)const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimH5ReaderFactory::open(kwl, prefix) DEBUG: entered..."
         << "Trying ossimKakaduNitfReader"
         << std::endl;
   }

   ossimRefPtr<ossimImageHandler> reader = new ossimH5Reader;
   if(reader->loadState(kwl, prefix) == false)
   {
      reader = 0;
   }

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimH5ReaderFactory::open(kwl, prefix) DEBUG: leaving..."
         << std::endl;
   }

   return reader.release();
}

ossimObject* ossimH5ReaderFactory::createObject(
   const ossimString& typeName)const
{
   ossimRefPtr<ossimObject> result = 0;
   if(typeName == "ossimH5Reader")
   {
      result = new ossimH5Reader;
   }

   return result.release();
}

ossimObject* ossimH5ReaderFactory::createObject(
   const ossimKeywordlist& kwl,
   const char* prefix)const
{
   return this->open(kwl, prefix);
}

void ossimH5ReaderFactory::getTypeNameList(
   std::vector<ossimString>& typeList)const
{
   typeList.push_back(ossimString("ossimH5Reader"));
}

void ossimH5ReaderFactory::getSupportedExtensions(
   ossimImageHandlerFactoryBase::UniqueStringList& extensionList)const
{
   extensionList.push_back(ossimString("h5"));
   extensionList.push_back(ossimString("he5"));
   extensionList.push_back(ossimString("hdf5"));   
}

bool ossimH5ReaderFactory::hasExcludedExtension(
   const ossimFilename& file) const
{
   bool result = true;
   ossimString ext = file.ext().downcase();
   //only include the file with those extension and exclude any other files
   if ( (ext == "h5") || (ext == "hdf5") || (ext == "he5") )
   {
      result = false;
   }
   return result;
}

ossimH5ReaderFactory::ossimH5ReaderFactory(){}

ossimH5ReaderFactory::ossimH5ReaderFactory(const ossimH5ReaderFactory&){}

void ossimH5ReaderFactory::operator=(const ossimH5ReaderFactory&){}

