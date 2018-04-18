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
#include <ossim/base/ossimEnvironmentUtility.h>
#include <ossim/base/ossimRegExp.h>
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

static void setGdalDescription(ossimString& description)
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
static void setValidDrivers(const ossimKeywordlist& options){
  int driverCount = GDALGetDriverCount();
  int idx = 0;
  ossimString driverRegExString = ossimEnvironmentUtility::instance()->getEnvironmentVariable("GDAL_ENABLE_DRIVERS");
  ossimRegExp driverRegEx;
  std::function<bool(GDALDriverH, ossimRegExp &)>
    isDriverEnabled = [](GDALDriverH driver, ossimRegExp &regExpression) -> bool { return true; };
  
  // check env variables first
  //
  if (!driverRegExString.empty())
  {
    isDriverEnabled = [](GDALDriverH driver, ossimRegExp &regExpression) -> bool { return regExpression.find(GDALGetDriverShortName(driver)); };
  }
  else
  {
    driverRegExString = ossimEnvironmentUtility::instance()->getEnvironmentVariable("GDAL_DISABLE_DRIVERS");
    if (!driverRegExString.empty())
    {
      isDriverEnabled = [](GDALDriverH driver, ossimRegExp &regExpression) -> bool { return !regExpression.find(GDALGetDriverShortName(driver)); };
    }
  }
  // now check properties
  if (driverRegExString.empty())
  {
    driverRegExString = options.find("enable_drivers");
    if (!driverRegExString.empty())
    {
      isDriverEnabled = [](GDALDriverH driver, ossimRegExp &regExpression) -> bool { return regExpression.find(GDALGetDriverShortName(driver)); };
    }
    else
    {
      driverRegExString = options.find("disable_drivers");
      if (!driverRegExString.empty())
      {
        isDriverEnabled = [](GDALDriverH driver, ossimRegExp &regExpression) -> bool { return !regExpression.find(GDALGetDriverShortName(driver)); };
      }
    }
  }
  if (!driverRegExString.empty())
  {
    driverRegEx.compile(driverRegExString.c_str());
  }
  for (idx = 0; idx < driverCount; ++idx)
  {
    GDALDriverH driver = GDALGetDriver(idx);
    if (driver)
    {
      if (!isDriverEnabled(driver, driverRegEx))
      {
        GDALDeregisterDriver(driver);
        GDALDestroyDriver(driver);
      }
    }
  }
}

extern "C"
{
   ossimSharedObjectInfo  gdalInfo;
   ossimString gdalDescription;
   std::vector<ossimString> gdalObjList;
   const char* getGdalDescription()
   {
      return gdalDescription.c_str();
   }
   int getGdalNumberOfClassNames()
   {
      return (int)gdalObjList.size();
   }
   const char* getGdalClassName(int idx)
   {
      if(idx < (int)gdalObjList.size())
      {
         return gdalObjList[idx].c_str();
      }
      return (const char*)0;
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(
               ossimSharedObjectInfo** info, 
               const char* options)
   {
     gdalInfo.getDescription = getGdalDescription;
     gdalInfo.getNumberOfClassNames = getGdalNumberOfClassNames;
     gdalInfo.getClassName = getGdalClassName;

     *info = &gdalInfo;
     ossimKeywordlist kwl;
     kwl.parseString(ossimString(options));
     /* Register the readers... */
     ossimImageHandlerRegistry::instance()
         ->registerFactory(ossimGdalFactory::instance(), ossimString(kwl.find("read_factory.location")).downcase() == "front");

     /* Register the writers... */
     ossimImageWriterFactoryRegistry::instance()->registerFactory(ossimGdalImageWriterFactory::instance(), ossimString(kwl.find("writer_factory.location")).downcase() == "front");

     /* Register the overview builder factory. */
     ossimOverviewBuilderFactoryRegistry::instance()->registerFactory(ossimGdalOverviewBuilderFactory::instance());

     ossimProjectionFactoryRegistry::instance()->registerFactory(ossimGdalProjectionFactory::instance());

     /* Register generic objects... */
     ossimObjectFactoryRegistry::instance()->registerFactory(ossimGdalObjectFactory::instance());

     /* Register gdal info factoy... */
     ossimInfoFactoryRegistry::instance()->registerFactory(ossimGdalInfoFactory::instance());

     setValidDrivers(kwl);
     setGdalDescription(gdalDescription);
     ossimGdalFactory::instance()->getTypeNameList(gdalObjList);
     ossimGdalImageWriterFactory::instance()->getTypeNameList(gdalObjList);
     ossimGdalOverviewBuilderFactory::instance()->getTypeNameList(gdalObjList);
     ossimGdalProjectionFactory::instance()->getTypeNameList(gdalObjList);
     ossimGdalObjectFactory::instance()->getTypeNameList(gdalObjList);
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
