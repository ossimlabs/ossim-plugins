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
#include <ossim/plugin/ossimPluginConstants.h>
#include "ossimPngReaderFactory.h"
#include "ossimPngWriterFactory.h"
#include "ossimPngCodecFactory.h"
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageWriterFactoryRegistry.h>
#include <ossim/imaging/ossimCodecFactoryRegistry.h>

extern "C"
{
   static ossimSharedObjectInfo  myPngInfo;
   static ossimString thePngDescription;
   static std::vector<ossimString> thePngObjList;

   static const char* getPngDescription()
   {
      if (thePngDescription.empty())
      {
         thePngDescription = "PNG reader / writer plugin.\n";
      }
      return thePngDescription.c_str();
   }

   static int getPngNumberOfClassNames()
   {
      return (int)thePngObjList.size();
   }

   static const char* getPngClassName(int idx)
   {
      if(idx < (int)thePngObjList.size())
      {
         return thePngObjList[idx].c_str();
      }
      return (const char*)0;
   }

   /* Note symbols need to be exported on windoze... */
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(ossimSharedObjectInfo** info,
                                                       const char* /*options*/)
   {
      thePngObjList.push_back("ossimPngReader");
      thePngObjList.push_back("ossimPngWriter");
      thePngObjList.push_back("ossimPngCodec");

      myPngInfo.getDescription = getPngDescription;
      myPngInfo.getNumberOfClassNames = getPngNumberOfClassNames;
      myPngInfo.getClassName = getPngClassName;

      *info = &myPngInfo;

      /* Register the readers... */
      ossimImageHandlerRegistry::instance()->
        registerFactory(ossimPngReaderFactory::instance());

      /* Register the writers... */
      ossimImageWriterFactoryRegistry::instance()->
         registerFactory(ossimPngWriterFactory::instance());

      ossimCodecFactoryRegistry::instance()->registerFactory(ossimPngCodecFactory::instance());

      ossimPngReaderFactory::instance()->getTypeNameList(thePngObjList);
      ossimPngWriterFactory::instance()->getTypeNameList(thePngObjList);
      ossimPngCodecFactory::instance()->getTypeNameList(thePngObjList);

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
