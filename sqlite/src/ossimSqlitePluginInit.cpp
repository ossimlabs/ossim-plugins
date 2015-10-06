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
#include "ossimPluginConstants.h"
#include "ossimSqliteReaderFactory.h"
#include "ossimSqliteWriterFactory.h"
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageWriterFactoryRegistry.h>
#include <ossim/support_data/ossimInfoFactoryRegistry.h>

static void setDescription(ossimString& description)
{
   description = "GeoPackage reader / writer plugin\n\n";
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
                                                       const char* /*options*/)
   {    
      myInfo.getDescription = getDescription;
      myInfo.getNumberOfClassNames = getNumberOfClassNames;
      myInfo.getClassName = getClassName;
      
      *info = &myInfo;
      
      /* Register the readers... */
      ossimImageHandlerRegistry::instance()->
        registerFactory(ossimSqliteReaderFactory::instance());

      /* Register the info factoy... */
      ossimInfoFactoryRegistry::instance()->
         registerFactory(ossimSqliteInfoFactory::instance());
      
      /* Register the writers... */
      ossimImageWriterFactoryRegistry::instance()->
         registerFactory(ossimSqliteWriterFactory::instance());
      
      setDescription(theDescription);
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
