//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: OSSIM SQLite plugin initialization
// code.
//
//----------------------------------------------------------------------------
// $Id: ossimSqlitePluginInit.cpp 19669 2011-05-27 13:27:29Z gpotts $

#include <ossim/plugin/ossimSharedObjectBridge.h>
#include "ossimSqliteInfoFactory.h"
#include <ossim/plugin/ossimPluginConstants.h>
#include "ossimSqliteReaderFactory.h"
#include "ossimSqliteWriterFactory.h"
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageWriterFactoryRegistry.h>
#include <ossim/support_data/ossimInfoFactoryRegistry.h>

static void setSqliteDescription(ossimString& description)
{
   description = "GeoPackage reader / writer plugin\n\n";
}


extern "C"
{
   ossimSharedObjectInfo  mySqliteInfo;
   ossimString theSqliteDescription;
   std::vector<ossimString> theSqliteObjList;

   const char* getSqliteDescription()
   {
      return theSqliteDescription.c_str();
   }

   int getSqliteNumberOfClassNames()
   {
      return (int)theSqliteObjList.size();
   }

   const char* getSqliteClassName(int idx)
   {
      if(idx < (int)theSqliteObjList.size())
      {
         return theSqliteObjList[idx].c_str();
      }
      return (const char*)0;
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(
                                                       ossimSharedObjectInfo** info, 
                                                       const char* /*options*/)
   {    
      mySqliteInfo.getDescription = getSqliteDescription;
      mySqliteInfo.getNumberOfClassNames = getSqliteNumberOfClassNames;
      mySqliteInfo.getClassName = getSqliteClassName;
      
      *info = &mySqliteInfo;
      
      /* Register the readers... */
      ossimImageHandlerRegistry::instance()->
        registerFactory(ossimSqliteReaderFactory::instance());

      /* Register the info factoy... */
      ossimInfoFactoryRegistry::instance()->
         registerFactory(ossimSqliteInfoFactory::instance());
      
      /* Register the writers... */
      ossimImageWriterFactoryRegistry::instance()->
         registerFactory(ossimSqliteWriterFactory::instance());
      
      setSqliteDescription(theSqliteDescription);
      ossimSqliteReaderFactory::instance()->getTypeNameList(theSqliteObjList);
      ossimSqliteWriterFactory::instance()->getTypeNameList(theSqliteObjList);
      theSqliteObjList.push_back("ossimGpkgInfo");
  }

   /* Note symbols need to be exported on windoze... */ 
  OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
  {
     ossimImageHandlerRegistry::instance()->
        unregisterFactory(ossimSqliteReaderFactory::instance());

     ossimInfoFactoryRegistry::instance()->
        unregisterFactory(ossimSqliteInfoFactory::instance());
        
     ossimImageWriterFactoryRegistry::instance()->
        unregisterFactory(ossimSqliteWriterFactory::instance());
  }
}
