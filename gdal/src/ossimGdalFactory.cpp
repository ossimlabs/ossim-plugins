//*******************************************************************
// Copyright (C) 2000 ImageLinks Inc.
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Garrett Potts
//*******************************************************************
//  $Id: ossimGdalFactory.cpp 22229 2013-04-12 14:13:46Z dburken $

#include <ossimGdalFactory.h>
#include <ossimGdalTileSource.h>
#include <ossimOgrGdalTileSource.h>
#include <ossimOgrVectorTileSource.h>
#include <ossimHdfReader.h>

#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimString.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimKeywordNames.h>

#include <gdal.h>
#include <ogrsf_frmts.h>

static const ossimTrace traceDebug("ossimGdalFactory:debug");

RTTI_DEF1(ossimGdalFactory, "ossimGdalFactory", ossimImageHandlerFactoryBase);

ossimGdalFactory* ossimGdalFactory::theInstance = 0;
ossimGdalFactory::~ossimGdalFactory()
{
   theInstance = (ossimGdalFactory*)NULL;
}


ossimGdalFactory* ossimGdalFactory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimGdalFactory;
      CPLSetErrorHandler((CPLErrorHandler)CPLQuietErrorHandler);
      GDALAllRegister();
      OGRRegisterAll();
   }

   // lets turn off gdal error reporting
   //
//   GDALSetCacheMax(1024*1024*20);
//   GDALSetCacheMax(0);

   return theInstance;
}
   
ossimImageHandler* ossimGdalFactory::open(const ossimFilename& fileName,
                                          bool openOverview)const
{
   // For GDAL we can't check for file exists since they support encoded opens for
   // subdatasets
   //
   
   if(fileName.ext().downcase() == "nui") return 0;
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalFactory::open(filename) DEBUG: entered..." << std::endl;
   }
   
   ossimRefPtr<ossimImageHandler> result;

   //try hdf reader first
   if (traceDebug())
   {
     ossimNotify(ossimNotifyLevel_DEBUG)
       << "ossimGdalFactory::open(filename) DEBUG:"
       << "\ntrying ossimHdfReader"
       << std::endl;
   }
   result = new ossimHdfReader;
   result->setOpenOverviewFlag(openOverview);
   if(result->open(fileName))
   {
     return result.release();
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalFactory::open(filename) DEBUG:"
         << "\ntrying ossimGdalTileSource"
         << std::endl;
   }

   result = new ossimGdalTileSource;
   result->setOpenOverviewFlag(openOverview);   
   if(result->open(fileName))
   {
      return result.release();
   }

   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalFactory::open(filename) DEBUG:"
         << "\ntrying ossimOgrVectorTileSource\n";
   }

   // No need to set overview flag.
   result = new ossimOgrVectorTileSource;
   if(result->open(fileName))
   {
     return result.release();
   }
   
   // we will have vector datasets only open if file exists.
   if(!fileName.exists() || 
       fileName.before(":", 3).upcase() == "SDE" || 
       fileName.before(":", 4).upcase() == "GLTP" || 
       fileName.ext().downcase() == "mdb")
   {
      result = 0;
      return result.release();
   }

   // No need to set overview flag.
   if (traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalFactory::open(filename) DEBUG:"
         << "\ntrying ossimOgrGdalTileSource\n";
   }
   
   result = new ossimOgrGdalTileSource;
   if(result->open(fileName))
   {
      return result.release();
   }
   result = 0;
   return result.release();
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << "ossimGdalFactory::open(filename) DEBUG: leaving..."
         << std::endl;
   }
   
   return (ossimImageHandler*)NULL;
}

ossimImageHandler* ossimGdalFactory::open(const ossimKeywordlist& kwl,
                                          const char* prefix)const
{
   ossimRefPtr<ossimImageHandler> result;
   
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << "ossimGdalFactory::open(kwl, prefix) DEBUG: entered..." << std::endl;
   }
   if(traceDebug())
   {
     ossimNotify(ossimNotifyLevel_DEBUG) << "ossimGdalFactory::open(kwl, prefix) DEBUG: trying ossimHdfReader" << std::endl;
   }
   result = new ossimHdfReader;
   if(result->loadState(kwl, prefix))
   {
     return result.release();
   }
   result = 0;
   
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << "ossimGdalFactory::open(kwl, prefix) DEBUG: trying ossimGdalTileSource" << std::endl;
   }
   result = new ossimGdalTileSource;
   if(result->loadState(kwl, prefix))
   {
      return result.release();
   }
   result = 0;
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << "ossimGdalFactory::open(kwl, prefix) DEBUG: trying ossimOgrGdalTileSource" << std::endl;
   }
   
   result = new ossimOgrGdalTileSource;
   if(result->loadState(kwl, prefix))
   {
      return result.release();
   }
   result = 0;
   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG) << "ossimGdalFactory::open(kwl, prefix) DEBUG: leaving..." << std::endl;
   }
   
   return result.release();
}

ossimRefPtr<ossimImageHandler> ossimGdalFactory::openOverview(
   const ossimFilename& file ) const
{
   ossimRefPtr<ossimImageHandler> result = 0;
   if ( file.size() )
   {
      result = new ossimGdalTileSource;
      
      result->setOpenOverviewFlag( false ); // Always false.

      if ( result->open( file ) == false )
      {
         result = 0;
      }
   }
   return result;
}

ossimObject* ossimGdalFactory::createObject(const ossimString& typeName)const
{
   if(STATIC_TYPE_NAME(ossimHdfReader) == typeName)
   {
     return new ossimHdfReader();
   }
   if(STATIC_TYPE_NAME(ossimGdalTileSource) == typeName)
   {
      return new ossimGdalTileSource();
   }
   if(STATIC_TYPE_NAME(ossimOgrGdalTileSource) == typeName)
   {
      return new ossimOgrGdalTileSource();
   }
   
   return (ossimObject*)0;
}

ossimObject* ossimGdalFactory::createObject(const ossimKeywordlist& kwl,
                                            const char* prefix)const
{
   ossimRefPtr<ossimObject> result;
   const char* type = kwl.find(prefix, ossimKeywordNames::TYPE_KW);

   if(type)
   {
      if (ossimString(type).trim() == STATIC_TYPE_NAME(ossimImageHandler))
      {
         const char* lookup = kwl.find(prefix, ossimKeywordNames::FILENAME_KW);
         if (lookup)
         {
            // Call the open that takes a filename...
            result = this->open(kwl, prefix);//ossimFilename(lookup));
         }
      }
      else
      {
         result = createObject(ossimString(type));
         if(result.valid())
         {
            result->loadState(kwl, prefix);
         }
      }
   }

   return result.release();
}
 
void ossimGdalFactory::getTypeNameList(std::vector<ossimString>& typeList)const
{
   typeList.push_back(STATIC_TYPE_NAME(ossimHdfReader));
   typeList.push_back(STATIC_TYPE_NAME(ossimGdalTileSource));
   typeList.push_back(STATIC_TYPE_NAME(ossimOgrGdalTileSource));
}

void ossimGdalFactory::getSupportedExtensions(ossimImageHandlerFactoryBase::UniqueStringList& extensionList)const
{
   // ossimOgrGdalTileSource - shape file reader:
   extensionList.push_back( ossimString("shp") );
   
   int driverCount = GDALGetDriverCount();
   int idx = 0;
   
   for(idx = 0; idx < driverCount; ++idx)
   {
      GDALDriverH driver =  GDALGetDriver(idx);  
      
      if(driver)
      {
         const char* metaData = GDALGetMetadataItem(driver, GDAL_DMD_EXTENSION, 0);
         int nMetaData = metaData ? strlen(metaData) : 0;
         if(metaData && nMetaData>0 )
         {
            std::vector<ossimString> splitArray;
            ossimString(metaData).split(splitArray, " /");

            ossim_uint32 idxExtension = 0;

            for(idxExtension = 0; idxExtension < splitArray.size(); ++idxExtension)
            {
               extensionList.push_back(splitArray[idxExtension].downcase());
            }
         }
      }
   }

   if(GDALGetDriverByName("AAIGrid"))
   {
      extensionList.push_back("adf");
   }
}

void ossimGdalFactory::getImageHandlersBySuffix(
   ossimImageHandlerFactoryBase::ImageHandlerList& result, const ossimString& ext) const
{
   if ( ext == "shp" )
   {
      result.push_back(new ossimOgrGdalTileSource);
   }
   else
   {
      ossimImageHandlerFactoryBase::UniqueStringList extList;
      getSupportedExtensions(extList);
      
      ossimString testExt = ext.downcase();
      
      if(std::find(extList.getList().begin(),
                   extList.getList().end(), testExt) != extList.getList().end())
      {
         result.push_back(new ossimGdalTileSource);
      }
   }
}

void ossimGdalFactory::getImageHandlersByMimeType(ossimImageHandlerFactoryBase::ImageHandlerList& result, const ossimString& mimeType)const
{
   int driverCount = GDALGetDriverCount();
   int idx = 0;
   
   for(idx = 0; idx < driverCount; ++idx)
   {
      GDALDriverH driver =  GDALGetDriver(idx);  
      
      if(driver)
      {
         const char* metaData = GDALGetMetadataItem(driver, GDAL_DMD_MIMETYPE, 0);
         int nMetaData = metaData ? strlen(metaData) : 0;
         if(metaData && nMetaData>0 )
         {
            if(ossimString(metaData) == mimeType)
            {
               result.push_back(new ossimGdalTileSource());
               return;
            }
         }
      }
   }
}

