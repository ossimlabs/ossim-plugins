//*******************************************************************
// Copyright (C) 2005 David Burken, all rights reserved.
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  David Burken
//*******************************************************************
//  $Id: ossimGdalPluginInit.cpp 18971 2011-02-25 19:53:30Z gpotts $
#include <gdal.h>
#include <ossim/plugin/ossimSharedObjectBridge.h>
#include <ossim/plugin/ossimPluginConstants.h>
#include <ossimGdalFactory.h>
#include <ossimGdalObjectFactory.h>
#include <ossimGdalImageWriterFactory.h>
#include <ossimGdalInfoFactory.h>
#include <ossimGdalProjectionFactory.h>
#include <ossimGdalOverviewBuilderFactory.h>
#include <ossim/base/ossimObjectFactoryRegistry.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageWriterFactoryRegistry.h>
#include <ossim/imaging/ossimOverviewBuilderFactoryRegistry.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <ossim/support_data/ossimInfoFactoryRegistry.h>

static void setDescription(ossimString& description)
{
   description = "GDAL Plugin\n\n";
   
   int driverCount = GDALGetDriverCount();
   int idx = 0;
   description += "GDAL Supported formats\n";
   for(idx = 0; idx < driverCount; ++idx)
   {
      GDALDriverH driver = GDALGetDriver(idx);
      if(driver)
      {
         description += "  name: ";
         description += ossimString(GDALGetDriverShortName(driver)) + " " + ossimString(GDALGetDriverLongName(driver)) + "\n";
      }
   }
}


extern "C"
{
   ossimSharedObjectInfo  myInfo;
   ossimString theDescription;
   std::vector<ossimString> theObjList;
   const char* getDescription()
   {
      return theDescription.c_str();
   }
   int getNumberOfClassNames()
   {
      return (int)theObjList.size();
   }
   const char* getClassName(int idx)
   {
      if(idx < (int)theObjList.size())
      {
         return theObjList[0].c_str();
      }
      return (const char*)0;
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(
               ossimSharedObjectInfo** info, 
               const char* options)
   {    
      myInfo.getDescription = getDescription;
      myInfo.getNumberOfClassNames = getNumberOfClassNames;
      myInfo.getClassName = getClassName;
      
      *info = &myInfo;
      ossimKeywordlist kwl;
      kwl.parseString(ossimString(options));

      /* Register the readers... */
     ossimImageHandlerRegistry::instance()->
        registerFactory(ossimGdalFactory::instance(), ossimString(kwl.find("read_factory.location")).downcase() == "front" );

     /* Register the writers... */
     ossimImageWriterFactoryRegistry::instance()->
        registerFactory(ossimGdalImageWriterFactory::instance(), ossimString(kwl.find("writer_factory.location")).downcase() == "front" );

     /* Register the overview builder factory. */
     ossimOverviewBuilderFactoryRegistry::instance()->
        registerFactory(ossimGdalOverviewBuilderFactory::instance());

     ossimProjectionFactoryRegistry::instance()->
        registerFactory(ossimGdalProjectionFactory::instance());

     /* Register generic objects... */
     ossimObjectFactoryRegistry::instance()->
        registerFactory(ossimGdalObjectFactory::instance());

     /* Register gdal info factoy... */
     ossimInfoFactoryRegistry::instance()->
       registerFactory(ossimGdalInfoFactory::instance());

     setDescription(theDescription);
  }

   /* Note symbols need to be exported on windoze... */ 
  OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
  {
     ossimImageHandlerRegistry::instance()->
        unregisterFactory(ossimGdalFactory::instance());

     ossimImageWriterFactoryRegistry::instance()->
        unregisterFactory(ossimGdalImageWriterFactory::instance());

     ossimOverviewBuilderFactoryRegistry::instance()->
        unregisterFactory(ossimGdalOverviewBuilderFactory::instance());

     ossimProjectionFactoryRegistry::instance()->unregisterFactory(ossimGdalProjectionFactory::instance());

     ossimObjectFactoryRegistry::instance()->
        unregisterFactory(ossimGdalObjectFactory::instance());

     ossimInfoFactoryRegistry::instance()->
       unregisterFactory(ossimGdalInfoFactory::instance());
  }
}
