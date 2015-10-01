//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: Factory for OSSIM JPEG 2000 (J2k) reader using kakadu library.
//----------------------------------------------------------------------------
// $Id: ossimKakaduReaderFactory.cpp 22884 2014-09-12 13:14:35Z dburken $

#include "ossimKakaduReaderFactory.h"
#include "ossimKakaduJp2Reader.h"
#include "ossimKakaduJ2kReader.h"
#include "ossimKakaduNitfReader.h"
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/imaging/ossimImageHandler.h>

static const ossimTrace traceDebug( ossimString("ossimKakaduReaderFactory:debug") );

class ossimImageHandler;

RTTI_DEF1(ossimKakaduReaderFactory,
          "ossimKakaduReaderFactory",
          ossimImageHandlerFactoryBase);

ossimKakaduReaderFactory::~ossimKakaduReaderFactory()
{
}

ossimKakaduReaderFactory* ossimKakaduReaderFactory::instance()
{
   static ossimKakaduReaderFactory inst;
   return &inst;
}

ossimImageHandler* ossimKakaduReaderFactory::open(const ossimFilename& fileName,
                                                  bool openOverview)const
{
   static const char* M = "ossimKakaduReaderFactory::open(filename) -- ";
   if(traceDebug())
      ossimNotify(ossimNotifyLevel_DEBUG)<<M<<"Entered with filename <"<<fileName<<">"  ;

   ossimRefPtr<ossimImageHandler> reader = 0; 
   while (true)
   {
      if (hasExcludedExtension(fileName))  break;

      if (traceDebug()) ossimNotify(ossimNotifyLevel_DEBUG)<<M<< "Trying ossimKakaduNitfReader...";
      reader = new ossimKakaduNitfReader;
      reader->setOpenOverviewFlag(openOverview);
      if(reader->open(fileName))  break;

      if (traceDebug()) ossimNotify(ossimNotifyLevel_DEBUG)<<M<< "Trying ossimKakaduJp2Reader...";
      reader = new ossimKakaduJp2Reader;
      reader->setOpenOverviewFlag(openOverview);
      if(reader->open(fileName))  break;

      if (traceDebug()) ossimNotify(ossimNotifyLevel_DEBUG)<<M<< "Trying ossimKakaduJ2kReader...";
      reader = new ossimKakaduJ2kReader;
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

ossimImageHandler* ossimKakaduReaderFactory::open(const ossimKeywordlist& kwl,
                                                  const char* prefix)const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimKakaduReaderFactory::open(kwl, prefix) DEBUG: entered..."
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
         reader = new ossimKakaduNitfReader;
         if(reader->loadState(kwl, prefix) == false)
         {
            reader = 0;
         }
         
         if (!reader)
         {
            reader = new ossimKakaduJp2Reader;
            if(reader->loadState(kwl, prefix) == false)
            {
               reader = 0;
            }
         }
         
         if (!reader)
         {
            reader = new ossimKakaduJ2kReader;
            if(reader->loadState(kwl, prefix) == false)
            {
               reader = 0;
            }
         }
      }
   }

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimKakaduReaderFactory::open(kwl, prefix) DEBUG: leaving..."
         << std::endl;
   }
   
   return reader.release();
}

ossimRefPtr<ossimImageHandler> ossimKakaduReaderFactory::openOverview(
   const ossimFilename& file ) const
{
   ossimRefPtr<ossimImageHandler> result = 0;
   if ( file.size() )
   {
      result = new ossimKakaduNitfReader;

      result->setOpenOverviewFlag( false ); // Always false...

      if( result->open( file ) == false )
      {
         result = 0;
      }
   }
   return result;
}

ossimObject* ossimKakaduReaderFactory::createObject(
   const ossimString& typeName)const
{
   ossimRefPtr<ossimObject> result = 0;
   if(typeName == "ossimKakaduNitfReader")
   {
      result = new ossimKakaduNitfReader;
   }
   else if(typeName == "ossimKakaduJp2Reader")
   {
      result = new ossimKakaduJp2Reader;
   }
   else if(typeName == "ossimKakaduJ2kReader")
   {
      result =  new ossimKakaduJ2kReader;
   }
   return result.release();
}

ossimObject* ossimKakaduReaderFactory::createObject(
   const ossimKeywordlist& kwl,
   const char* prefix)const
{
   return this->open(kwl, prefix);
}
 
void ossimKakaduReaderFactory::getTypeNameList(
   std::vector<ossimString>& typeList)const
{
   typeList.push_back(ossimString("ossimKakaduNitfReader"));
   typeList.push_back(ossimString("ossimKakaduJp2Reader"));
   typeList.push_back(ossimString("ossimKakaduJ2kReader"));
}

void ossimKakaduReaderFactory::getSupportedExtensions(
   ossimImageHandlerFactoryBase::UniqueStringList& extensionList)const
{
   extensionList.push_back(ossimString("j2k"));
   extensionList.push_back(ossimString("jp2"));   
}

bool ossimKakaduReaderFactory::hasExcludedExtension(
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
 *
 * Will add to the result list any handler that supports the passed in extensions
 *
 */
void ossimKakaduReaderFactory::getImageHandlersBySuffix(ossimImageHandlerFactoryBase::ImageHandlerList& result,
                                                        const ossimString& ext)const
{
   ossimString testExt = ext.downcase();
   if(testExt == "jp2")
   {
      result.push_back(new ossimKakaduJp2Reader);
   }
   else if(testExt == "ntf")
   {
      result.push_back(new ossimKakaduNitfReader);
   }
   else if(testExt == "nitf")
   {
      result.push_back(new ossimKakaduNitfReader);
   }
}
/**
 *
 * Will add to the result list and handler that supports the passed in mime type
 *
 */
void ossimKakaduReaderFactory::getImageHandlersByMimeType(ossimImageHandlerFactoryBase::ImageHandlerList& result,
                                                          const ossimString& mimeType)const
{
   ossimString test(mimeType.begin(), mimeType.begin()+6);
   if(test == "image/")
   {
      ossimString mimeTypeTest(mimeType.begin() + 6, mimeType.end());
      if(mimeTypeTest == "jp2")
      {
         result.push_back(new ossimKakaduJp2Reader);
      }
      else if(mimeTypeTest == "nitf")
      {
         result.push_back(new ossimKakaduNitfReader);
      }
   }
}


ossimKakaduReaderFactory::ossimKakaduReaderFactory(){}

ossimKakaduReaderFactory::ossimKakaduReaderFactory(const ossimKakaduReaderFactory&){}

void ossimKakaduReaderFactory::operator=(const ossimKakaduReaderFactory&){}
