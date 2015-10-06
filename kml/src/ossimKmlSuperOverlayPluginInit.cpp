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
#include "../ossimPluginConstants.h"
// #include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageWriterFactoryRegistry.h>
#include <ossim/imaging/ossimOverviewBuilderFactoryRegistry.h>

// #include "ossimKmlSuperOverlayReaderFactory.h"
#include "ossimKmlSuperOverlayWriterFactory.h"

static void setDescription(ossimString& description)
{
   description = "KmlSuperOverlay writer plugin\n\n";
   // description = "KmlSuperOverlay reader plugin\n\n";
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
      ossimSharedObjectInfo** info)
   {    
      myInfo.getDescription = getDescription;
      myInfo.getNumberOfClassNames = getNumberOfClassNames;
      myInfo.getClassName = getClassName;
      
      *info = &myInfo;

      /* Register the readers... */
      // ossimImageHandlerRegistry::instance()->
      //    registerFactory(ossimKmlSuperOverlayReaderFactory::instance());
      
      /* Register the writers... */
      ossimImageWriterFactoryRegistry::instance()->
         registerFactory(ossimKmlSuperOverlayWriterFactory::instance());
      
      setDescription(theDescription);
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
