//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  Mingjie Su
//
// Description: OSSIM KmlSuperOverlay plugin initialization
// code.
//
//----------------------------------------------------------------------------
// $Id: ossimKmlSuperOverlayPluginInit.cpp 2470 2011-04-26 15:32:19Z david.burken $

#include <ossim/plugin/ossimSharedObjectBridge.h>
#include <ossim/plugin/ossimPluginConstants.h>
// #include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageWriterFactoryRegistry.h>
#include <ossim/imaging/ossimOverviewBuilderFactoryRegistry.h>

// #include "ossimKmlSuperOverlayReaderFactory.h"
#include "ossimKmlSuperOverlayWriterFactory.h"

static void setKmlDescription(ossimString& description)
{
   description = "KmlSuperOverlay writer plugin\n\n";
   // description = "KmlSuperOverlay reader plugin\n\n";
}

extern "C"
{
   ossimSharedObjectInfo  myKmlInfo;
   ossimString theKmlDescription;
   std::vector<ossimString> theKmlObjList;

   const char* getKmlDescription()
   {
      return theKmlDescription.c_str();
   }

   int getKmlNumberOfClassNames()
   {
      return (int)theKmlObjList.size();
   }

   const char* getKmlClassName(int idx)
   {
      if(idx < (int)theKmlObjList.size())
      {
         return theKmlObjList[idx].c_str();
      }
      return (const char*)0;
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(
      ossimSharedObjectInfo** info)
   {    
      myKmlInfo.getDescription = getKmlDescription;
      myKmlInfo.getNumberOfClassNames = getKmlNumberOfClassNames;
      myKmlInfo.getClassName = getKmlClassName;
      
      *info = &myKmlInfo;

      /* Register the readers... */
      // ossimImageHandlerRegistry::instance()->
      //    registerFactory(ossimKmlSuperOverlayReaderFactory::instance());
      
      /* Register the writers... */
      ossimImageWriterFactoryRegistry::instance()->
         registerFactory(ossimKmlSuperOverlayWriterFactory::instance());
      
      setKmlDescription(theKmlDescription);
      //ossimKmlSuperOverlayReaderFactory::instance()->getTypeNameList(theKmlObjList);
      ossimKmlSuperOverlayWriterFactory::instance()->getTypeNameList(theKmlObjList);

   }
   
   /* Note symbols need to be exported on windoze... */
   OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
   { 
      // ossimImageHandlerRegistry::instance()->
      //    unregisterFactory(ossimKmlSuperOverlayReaderFactory::instance());
   
      ossimImageWriterFactoryRegistry::instance()->
         unregisterFactory(ossimKmlSuperOverlayWriterFactory::instance());
   }
}
