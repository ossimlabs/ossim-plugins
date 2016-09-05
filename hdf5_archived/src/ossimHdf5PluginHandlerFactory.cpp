/*****************************************************************************
*                                                                            *
*                                 O S S I M                                  *
*            Open Source, Geospatial Image Processing Project                *
*          License: MIT, see LICENSE at the top-level directory              *
*                                                                            *
*****************************************************************************/

#include "ossimHdf5PluginHandlerFactory.h"

#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/imaging/ossimImageHandler.h>
#include "ossimViirsHandler.h"

class ossimImageHandler;

RTTI_DEF1(ossimHdf5PluginHandlerFactory, "ossimHdf5HandlerFactory", ossimImageHandlerFactoryBase);

ossimHdf5PluginHandlerFactory* ossimHdf5PluginHandlerFactory::theInstance = 0;

// To add other sensor types that use HDF5, search for the string "Add other sensors here".

ossimHdf5PluginHandlerFactory::~ossimHdf5PluginHandlerFactory()
{
   theInstance = 0;
}

ossimHdf5PluginHandlerFactory* ossimHdf5PluginHandlerFactory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimHdf5PluginHandlerFactory;
   }
   return theInstance;
}

ossimImageHandler* ossimHdf5PluginHandlerFactory::open(const ossimFilename& fileName,
                                                 bool openOverview)const
{
   ossimImageHandlerFactoryBase::UniqueStringList extensionList;
   getSupportedExtensions(extensionList);

   ossimString ext = fileName.ext().downcase();
   bool ext_handled = false;
   for (int i=0; (i<extensionList.size()) && !ext_handled ; ++i)
   {
      if (ext == extensionList[i])
         ext_handled = true;
   }
   if (!ext_handled)
       return NULL;

   // Now try each of the represented concrete types in this plugin:
   ossimRefPtr<ossimImageHandler> reader = 0;
   while (1)
   {
      // VIIRS:
      reader = new ossimViirsHandler;
      reader->setOpenOverviewFlag(openOverview);
      if(reader->open(fileName))
         break;

      // Add other sensors here:

      reader = 0;
      break;
   }

   return reader.release();
}

ossimImageHandler* ossimHdf5PluginHandlerFactory::open(const ossimKeywordlist& kwl,
                                                       const char* prefix) const
{
   ossimRefPtr<ossimImageHandler> reader = 0;
   while (1)
   {
      // VIIRS:
      reader = new ossimViirsHandler;
      if(reader->loadState(kwl, prefix))
         break;

      // Add other sensors here:
      reader = 0;
      break;
   }
   return reader.release();
}

ossimObject* ossimHdf5PluginHandlerFactory::createObject(const ossimString& typeName) const
{
   ossimRefPtr<ossimObject> result = 0;
   if(typeName == "ossimViirsHandler")
      result = new ossimViirsHandler;

   // Add other sensors here:

   return result.release();
}

ossimObject* ossimHdf5PluginHandlerFactory::createObject(
   const ossimKeywordlist& kwl,
   const char* prefix)const
{
   return this->open(kwl, prefix);
}

void ossimHdf5PluginHandlerFactory::getTypeNameList(
   std::vector<ossimString>& typeList)const
{
   typeList.push_back(ossimString("ossimViirsHandler"));
   // Add other sensors here:
}

void ossimHdf5PluginHandlerFactory::getSupportedExtensions(
   ossimImageHandlerFactoryBase::UniqueStringList& extensionList)const
{
   extensionList.push_back(ossimString("h5"));
   extensionList.push_back(ossimString("he5"));
   extensionList.push_back(ossimString("hdf5"));   
}

void ossimHdf5PluginHandlerFactory::getSupportedRenderableNames(std::vector<ossimString>& names) const
{
   names.clear();

   // Fetch all sensors represented by this plugin. Start with VIIRS:
   ossimViirsHandler viirs;
   const vector<ossimString>& viirsRenderables = viirs.getRenderableSetNames();
   names.insert(names.end(), viirsRenderables.begin(), viirsRenderables.end());

   // Add other sensors here:
}

ossimHdf5PluginHandlerFactory::ossimHdf5PluginHandlerFactory(){}

ossimHdf5PluginHandlerFactory::ossimHdf5PluginHandlerFactory(const ossimHdf5PluginHandlerFactory&){}

void ossimHdf5PluginHandlerFactory::operator=(const ossimHdf5PluginHandlerFactory&){}

