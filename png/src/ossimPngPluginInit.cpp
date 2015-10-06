//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: OSSIM Portable Network Graphics (PNG) plugin initialization
// code.
//
//----------------------------------------------------------------------------
// $Id: ossimPngPluginInit.cpp 22998 2014-11-23 21:38:13Z gpotts $

#include <ossim/plugin/ossimSharedObjectBridge.h>
#include "ossimPluginConstants.h"
#include "ossimPngReaderFactory.h"
#include "ossimPngWriterFactory.h"
#include "ossimPngCodecFactory.h"
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageWriterFactoryRegistry.h>
#include <ossim/imaging/ossimCodecFactoryRegistry.h>

static void setDescription(ossimString& description)
{
   description = "PNG reader / writer plugin\n\n";
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
        registerFactory(ossimPngReaderFactory::instance());
      
      /* Register the writers... */
      ossimImageWriterFactoryRegistry::instance()->
         registerFactory(ossimPngWriterFactory::instance());
      
      ossimCodecFactoryRegistry::instance()->registerFactory(ossimPngCodecFactory::instance());
      setDescription(theDescription);
  }

   /* Note symbols need to be exported on windoze... */ 
  OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
  {
     ossimImageHandlerRegistry::instance()->
        unregisterFactory(ossimPngReaderFactory::instance());

     ossimImageWriterFactoryRegistry::instance()->
        unregisterFactory(ossimPngWriterFactory::instance());

     ossimCodecFactoryRegistry::instance()->unregisterFactory(ossimPngCodecFactory::instance());
  }
}
