//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: OSSIM Open JPEG plugin initialization
// code.
//
//----------------------------------------------------------------------------
// $Id$

#include <ossim/plugin/ossimSharedObjectBridge.h>
#include <ossim/plugin/ossimPluginConstants.h>
#include "ossimOpjReaderFactory.h"
#include "ossimOpjWriterFactory.h"
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageWriterFactoryRegistry.h>

static void setOpenJpegDescription(ossimString& description)
{
   description = "Open JPEG (j2k) reader / writer plugin\n\n";
}


extern "C"
{
   ossimSharedObjectInfo  myOpenJpegInfo;
   ossimString theOpenJpegDescription;
   std::vector<ossimString> theOpenJpegObjList;

   const char* getOpenJpegDescription()
   {
      return theOpenJpegDescription.c_str();
   }

   int getOpenJpegNumberOfClassNames()
   {
      return (int)theOpenJpegObjList.size();
   }

   const char* getOpenJpegClassName(int idx)
   {
      if(idx < (int)theOpenJpegObjList.size())
      {
         return theOpenJpegObjList[idx].c_str();
      }
      return (const char*)0;
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(
      ossimSharedObjectInfo** info)
   {    
      myOpenJpegInfo.getDescription = getOpenJpegDescription;
      myOpenJpegInfo.getNumberOfClassNames = getOpenJpegNumberOfClassNames;
      myOpenJpegInfo.getClassName = getOpenJpegClassName;
      
      *info = &myOpenJpegInfo;
      
      /* Register the readers... */
      ossimImageHandlerRegistry::instance()->
        registerFactory(ossimOpjReaderFactory::instance());
      
      /* Register the writers... */
      ossimImageWriterFactoryRegistry::instance()->
         registerFactory(ossimOpjWriterFactory::instance());
      
      setOpenJpegDescription(theOpenJpegDescription);
      ossimOpjReaderFactory::instance()->getTypeNameList(theOpenJpegObjList);
      ossimOpjWriterFactory::instance()->getTypeNameList(theOpenJpegObjList);
  }

   /* Note symbols need to be exported on windoze... */ 
  OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
  {
     ossimImageHandlerRegistry::instance()->
        unregisterFactory(ossimOpjReaderFactory::instance());

     ossimImageWriterFactoryRegistry::instance()->
        unregisterFactory(ossimOpjWriterFactory::instance());
  }
}
