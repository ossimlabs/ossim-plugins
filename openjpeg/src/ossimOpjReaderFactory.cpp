//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: Factory for J2k image readers OpenJPEG library.
//----------------------------------------------------------------------------
// $Id: ossimOpjReaderFactory.cpp 11046 2007-05-25 18:03:03Z gpotts $

#include <ossimOpjReaderFactory.h>
#include <ossimOpjJp2Reader.h>
#include <ossimOpjNitfReader.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimString.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimKeywordNames.h>

static const ossimTrace traceDebug("ossimOpjReaderFactory:debug");

RTTI_DEF1(ossimOpjReaderFactory,
          "ossimOpjReaderFactory",
          ossimImageHandlerFactoryBase);

ossimOpjReaderFactory* ossimOpjReaderFactory::theInstance = 0;

ossimOpjReaderFactory::~ossimOpjReaderFactory()
{
   theInstance = 0;
}

ossimOpjReaderFactory* ossimOpjReaderFactory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimOpjReaderFactory;
   }
   return theInstance;
}
   
ossimImageHandler* ossimOpjReaderFactory::open(
   const ossimFilename& fileName, bool openOverview) const
{
   static const char* M = "ossimOpjReaderFactory::open(filename)";
   
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << M << " Entered with filename:" << fileName
         << "\n";
   }

   ossimRefPtr<ossimImageHandler> reader = 0; 
   while (true)
   {
      if (hasExcludedExtension(fileName))  break;

      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG) << M << " Trying ossimOpjJp2Reader...";
      }
      reader = new ossimOpjJp2Reader();
      reader->setOpenOverviewFlag(openOverview);
      if( reader->open(fileName) ) break;

      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG) << M << " Trying ossimOpjNitfReader...";
      }      
      reader = new ossimOpjNitfReader();
      if( reader->open(fileName) ) break;

      reader = 0;
      break;
   }
   
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << M << " DEBUG: leaving..." << std::endl;
   }
   
   return reader.release();
}

ossimImageHandler* ossimOpjReaderFactory::open(
   const ossimKeywordlist& kwl, const char* prefix)const
{
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimOpjReaderFactory::open(kwl, prefix) DEBUG: entered..."
         << "Trying ossimOpjReader"
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
         reader = new ossimOpjNitfReader;
         if(reader->loadState(kwl, prefix) == false)
         {
            reader = 0;
         }
         
         if (!reader)
         {
            reader = new ossimOpjJp2Reader;
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
         << "ossimOpjReaderFactory::open(kwl, prefix) DEBUG: leaving..."
         << std::endl;
   }
   
   return reader.release();
}

ossimRefPtr<ossimImageHandler> ossimOpjReaderFactory::openOverview(
   const ossimFilename& file ) const
{
   ossimRefPtr<ossimImageHandler> result = 0;
   if ( file.size() )
   {
      result = new ossimOpjNitfReader();

      result->setOpenOverviewFlag( false ); // Always false...

      if( result->open( file ) == false )
      {
         result = 0;
      }
   }
   return result;
}

ossimObject* ossimOpjReaderFactory::createObject(
   const ossimString& typeName)const
{
   if(typeName == "ossimOpjJp2Reader")
   {
      return new ossimOpjJp2Reader;
   }
   else if(typeName == "ossimOpjNitfReader")
   {
      return new ossimOpjNitfReader;
   }
   return 0;
}

ossimObject* ossimOpjReaderFactory::createObject(
   const ossimKeywordlist& kwl,
   const char* prefix)const
{
   return this->open(kwl, prefix);
}
 
void ossimOpjReaderFactory::getTypeNameList(
   std::vector<ossimString>& typeList)const
{
   typeList.push_back(ossimString("ossimOpjJp2Reader"));
   typeList.push_back(ossimString("ossimOpjNitfReader"));
}

void ossimOpjReaderFactory::getSupportedExtensions(
   ossimImageHandlerFactoryBase::UniqueStringList& extensionList) const
{
   extensionList.push_back(ossimString("jp2"));
}

void ossimOpjReaderFactory::getImageHandlersBySuffix(
   ossimImageHandlerFactoryBase::ImageHandlerList& result,
   const ossimString& ext)const
{
   ossimString testExt = ext.downcase();
   if ( (testExt == "jp2") || (testExt == "j2k") )
   {
      result.push_back( new ossimOpjJp2Reader() );
   }
   else if( (testExt == "ntf") || (testExt == "nitf") )
   {
      result.push_back(new ossimOpjNitfReader() );
   }
}

void ossimOpjReaderFactory::getImageHandlersByMimeType(
   ossimImageHandlerFactoryBase::ImageHandlerList& result,
   const ossimString& mimeType) const
{
   ossimString test(mimeType.begin(), mimeType.begin()+6);
   if(test == "image/")
   {
      ossimString mimeTypeTest(mimeType.begin() + 6, mimeType.end());
      if( (mimeTypeTest == "jp2") || (mimeTypeTest == "j2k") )
      {
         result.push_back(new ossimOpjJp2Reader);
      }
      else if(mimeTypeTest == "nitf")
      {
         result.push_back(new ossimOpjNitfReader);
      }
   }
}

bool ossimOpjReaderFactory::hasExcludedExtension( const ossimFilename& file) const
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

ossimOpjReaderFactory::ossimOpjReaderFactory(){}

ossimOpjReaderFactory::ossimOpjReaderFactory(const ossimOpjReaderFactory&){}

void ossimOpjReaderFactory::operator=(const ossimOpjReaderFactory&){}
