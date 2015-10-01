//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author: Garrett Potts
//
// Description: Factory for OSSIM JPIP reader using kakadu library.
//----------------------------------------------------------------------------
// $Id$

#include "ossimKakaduJpipHandlerFactory.h"
#include "ossimKakaduJpipHandler.h"
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimUrl.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/imaging/ossimImageHandler.h>

static const ossimTrace traceDebug( ossimString("ossimKakaduJpipHandlerFactory:debug") );

class ossimImageHandler;

RTTI_DEF1(ossimKakaduJpipHandlerFactory,
          "ossimKakaduJpipHandlerFactory",
          ossimImageHandlerFactoryBase);

ossimKakaduJpipHandlerFactory::~ossimKakaduJpipHandlerFactory()
{
}

ossimKakaduJpipHandlerFactory* ossimKakaduJpipHandlerFactory::instance()
{
   static ossimKakaduJpipHandlerFactory inst;
   return &inst;
}

ossimImageHandler* ossimKakaduJpipHandlerFactory::open(const ossimFilename& fileName,
                                                       bool openOverview)const
{
   static const char* M = "ossimKakaduJpipHandlerFactory::open(filename) -- ";
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)<<M<<"Entered with filename <"<<fileName<<">\n" ;
   }
   
   ossimRefPtr<ossimImageHandler> reader = 0; 
   
   if (traceDebug()) ossimNotify(ossimNotifyLevel_DEBUG)<<M<< "Trying ossimKakaduJpipHandler...";
   reader = new ossimKakaduJpipHandler();
   reader->setOpenOverviewFlag(openOverview);
   if(!reader->open(fileName))
   {
      reader = 0;
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

ossimImageHandler* ossimKakaduJpipHandlerFactory::open(const ossimKeywordlist& kwl,
                                                  const char* prefix)const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimKakaduJpipHandlerFactory::open(kwl, prefix) DEBUG: entered..."
         << std::endl;
   }
   ossimRefPtr<ossimImageHandler> reader = 0; 
   const char* lookup = kwl.find(prefix, ossimKeywordNames::TYPE_KW);
   const char* filenameLookup = kwl.find(prefix, ossimKeywordNames::FILENAME_KW);
   const char* urlLookup = kwl.find(prefix, "url");
   
   ossimRefPtr<ossimObject> obj;
   // first check if url is present in keywordlist
   if(urlLookup)
   {
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "URL lookup"
         << std::endl;
   }

      if(lookup)
      {
         obj = createObject(ossimString(lookup));
         reader = dynamic_cast<ossimImageHandler*> (obj.get());
         obj = 0;
         if(reader.valid())
         {
            if(reader->loadState(kwl, prefix) == false)
            {
               reader = 0;
            }
         }
      }
      if(!reader.valid())
      {
         if(ossimUrl(urlLookup).getProtocol().downcase().contains("jpip"))
         {
            reader = new ossimKakaduJpipHandler();
            if(reader->loadState(kwl, prefix) == false)
            {
               reader = 0;
            }
         }
      }
   }
   else if(filenameLookup)
   {
      ossimFilename local(filenameLookup);

     
      // check if the filename is a jpip url
      if(!ossimFilename(filenameLookup).exists()&&
         ossimUrl(filenameLookup).getProtocol().downcase().contains("jpip"))
      {
         reader = open(ossimFilename(filenameLookup));
      }
      else if(local.exists()&&(local.ext().downcase()=="jpip"))
      {
         // do local file keywordlist
         //
         ossimKeywordlist tempKwl;
         tempKwl.add(kwl, prefix, true);
         if(tempKwl.addFile(ossimFilename(filenameLookup)))
         {
            if(tempKwl.find("url"))
            {
               reader = open(tempKwl);
            }
         }
      }
   }
   else if(lookup)
   {
      obj = createObject(ossimString(lookup));
      reader = dynamic_cast<ossimImageHandler*> (obj.get());
      obj = 0;
      if(reader.valid())
      {
         if(reader->loadState(kwl, prefix) == false)
         {
            reader = 0;
         }
      }
   }
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimKakaduJpipHandlerFactory::open(kwl, prefix) DEBUG: leaving..."
         << std::endl;
   }
   
   return reader.release();
}

ossimObject* ossimKakaduJpipHandlerFactory::createObject(const ossimString& typeName)const
{
   ossimRefPtr<ossimObject> result = 0;
   
   if(typeName == STATIC_TYPE_NAME(ossimKakaduJpipHandler))
   {
      result =  new ossimKakaduJpipHandler();
   }
   
   return result.release();
}

ossimObject* ossimKakaduJpipHandlerFactory::createObject(
   const ossimKeywordlist& kwl,
   const char* prefix)const
{
   return this->open(kwl, prefix);
}
 
void ossimKakaduJpipHandlerFactory::getTypeNameList(
   std::vector<ossimString>& typeList)const
{
   typeList.push_back(STATIC_TYPE_NAME(ossimKakaduJpipHandler));
}

void ossimKakaduJpipHandlerFactory::getSupportedExtensions(
   ossimImageHandlerFactoryBase::UniqueStringList& extensionList)const
{
   extensionList.push_back("jp2");
   extensionList.push_back("jpip");
}

void ossimKakaduJpipHandlerFactory::getImageHandlersBySuffix(ossimImageHandlerFactoryBase::ImageHandlerList& result,
                                                       const ossimString& ext)const
{
    ossimString testExt = ext.downcase();
   if((testExt == "jp2")||(testExt=="jp2"))
   {
      result.push_back(new ossimKakaduJpipHandler());
   }
}

/**
 *
 * Will add to the result list and handler that supports the passed in mime type
 *
 */
void ossimKakaduJpipHandlerFactory::getImageHandlersByMimeType(ossimImageHandlerFactoryBase::ImageHandlerList& result,
                                                          const ossimString& mimeType)const
{
}


ossimKakaduJpipHandlerFactory::ossimKakaduJpipHandlerFactory(){}

ossimKakaduJpipHandlerFactory::ossimKakaduJpipHandlerFactory(const ossimKakaduJpipHandlerFactory&){}

void ossimKakaduJpipHandlerFactory::operator=(const ossimKakaduJpipHandlerFactory&){}
