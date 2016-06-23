//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: Factory for OSSIM NITF 12 bit jpeg reader using jpeg library.
//----------------------------------------------------------------------------
// $Id$

#include "ossimJpeg12ReaderFactory.h"
#include "ossimJpeg12NitfReader.h"
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/imaging/ossimImageHandler.h>

static const ossimTrace traceDebug( ossimString("ossimJpeg12ReaderFactory:debug") );

class ossimImageHandler;

RTTI_DEF1(ossimJpeg12ReaderFactory,
          "ossimJpeg12ReaderFactory",
          ossimImageHandlerFactoryBase);

ossimJpeg12ReaderFactory::~ossimJpeg12ReaderFactory()
{
}

ossimJpeg12ReaderFactory* ossimJpeg12ReaderFactory::instance()
{
   static ossimJpeg12ReaderFactory inst;
   return &inst;
}

ossimImageHandler* ossimJpeg12ReaderFactory::open(const ossimFilename& fileName,
                                                  bool openOverview)const
{
   static const char* M = "ossimJpeg12ReaderFactory::open(filename) -- ";
   if(traceDebug())
      ossimNotify(ossimNotifyLevel_DEBUG)<<M<<"Entered with filename <"<<fileName<<">"  ;

   ossimRefPtr<ossimImageHandler> reader = 0; 
   while (true)
   {
      if (hasExcludedExtension(fileName))  break;

      if (traceDebug()) ossimNotify(ossimNotifyLevel_DEBUG)<<M<< "Trying ossimJpeg12NitfReader...";
      reader = new ossimJpeg12NitfReader;
      reader->setOpenOverviewFlag(openOverview);
      if(reader->open(fileName))  break;

      reader = 0;
      break;
   } 
   
   if (traceDebug())
   {
      if (reader.valid())
         ossimNotify(ossimNotifyLevel_DEBUG)<<M<< "   SUCCESS" << std::endl;
      else
         ossimNotify(ossimNotifyLevel_DEBUG)<<M<< "   Open FAILED" << std::endl;
   }
 
   return reader.release();
}

ossimImageHandler* ossimJpeg12ReaderFactory::open(const ossimKeywordlist& kwl,
                                                  const char* prefix)const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimJpeg12ReaderFactory::open(kwl, prefix) DEBUG: entered..."
         << std::endl;
   }

   ossimRefPtr<ossimImageHandler> reader = 0; 

   // To save time check the file name first.
   const char* lookup = kwl.find(prefix, ossimKeywordNames::FILENAME_KW);
   if (!lookup)
   {
      // Deprecated...
      lookup = kwl.find(prefix, ossimKeywordNames::IMAGE_FILE_KW);
   }

   if (lookup)
   {
      ossimFilename f = lookup;
      if ( hasExcludedExtension(f) == false )
      {
         reader = new ossimJpeg12NitfReader;
         if(reader->loadState(kwl, prefix) == false)
         {
            reader = 0;
         }
      }
   }

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimJpeg12ReaderFactory::open(kwl, prefix) DEBUG: leaving..."
         << std::endl;
   }
   
   return reader.release();
}

ossimObject* ossimJpeg12ReaderFactory::createObject(
   const ossimString& typeName)const
{
   ossimRefPtr<ossimObject> result = 0;
   if(typeName == "ossimJpeg12NitfReader")
   {
      result = new ossimJpeg12NitfReader;
   }
   return result.release();
}

ossimObject* ossimJpeg12ReaderFactory::createObject(
   const ossimKeywordlist& kwl,
   const char* prefix)const
{
   return this->open(kwl, prefix);
}
 
void ossimJpeg12ReaderFactory::getTypeNameList(
   std::vector<ossimString>& typeList)const
{
   typeList.push_back(ossimString("ossimJpeg12NitfReader"));
}

void ossimJpeg12ReaderFactory::getSupportedExtensions(
   ossimImageHandlerFactoryBase::UniqueStringList& /* extensionList */ )const
{
   /* Nothing to do here. */
}

bool ossimJpeg12ReaderFactory::hasExcludedExtension(
   const ossimFilename& file) const
{
   bool result = false;
   ossimString ext = file.ext().downcase();
   if ( (ext == "tif")  ||
        (ext == "tiff") ||
        (ext == "jpg")  ||
        (ext == "jpeg") ||
        (ext == "png") )
   {
      result = true;
   }
   return result;
}

/**
 * Will add to the result list any handler that supports the passed in extensions
 */
void ossimJpeg12ReaderFactory::getImageHandlersBySuffix(
   ossimImageHandlerFactoryBase::ImageHandlerList& result,
   const ossimString& ext)const
{
   ossimString testExt = ext.downcase();
   if( (testExt == "ntf") || (testExt == "nitf") )
   {
      result.push_back(new ossimJpeg12NitfReader);
   }
}

/**
 * Will add to the result list and handler that supports the passed in mime type
 */
void ossimJpeg12ReaderFactory::getImageHandlersByMimeType(
   ossimImageHandlerFactoryBase::ImageHandlerList& result,
   const ossimString& mimeType)const
{
   ossimString test(mimeType.begin(), mimeType.begin()+6);
   if(test == "image/")
   {
      ossimString mimeTypeTest(mimeType.begin() + 6, mimeType.end());
      if(mimeTypeTest == "nitf")
      {
         result.push_back(new ossimJpeg12NitfReader);
      }
   }
}


ossimJpeg12ReaderFactory::ossimJpeg12ReaderFactory(){}

ossimJpeg12ReaderFactory::ossimJpeg12ReaderFactory(const ossimJpeg12ReaderFactory&){}

void ossimJpeg12ReaderFactory::operator=(const ossimJpeg12ReaderFactory&){}
