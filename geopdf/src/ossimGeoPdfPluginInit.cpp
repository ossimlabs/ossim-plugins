//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  Mingjie Su
//
// Description: OSSIM GeoPdf plugin initialization
// code.
//
//----------------------------------------------------------------------------
// $Id: ossimGeoPdfPluginInit.cpp 20935 2012-05-18 14:19:30Z dburken $

#include <ossim/plugin/ossimSharedObjectBridge.h>
#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimOverviewBuilderFactoryRegistry.h>
#include <ossim/support_data/ossimInfoFactoryRegistry.h>

#include "ossimGeoPdfReaderFactory.h"
#include "ossimGeoPdfInfoFactory.h"

static void setDescription(ossimString& description)
{
   description = "GeoPdf reader plugin\n\n";
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
      ossimImageHandlerRegistry::instance()->
         registerFactory(ossimGeoPdfReaderFactory::instance(), false);

      /* Register GeoPdf info objects... */
      ossimInfoFactoryRegistry::instance()->
        registerFactory(ossimGeoPdfInfoFactory::instance());

      setDescription(theDescription);
   }
   
   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
   {
      ossimImageHandlerRegistry::instance()->
         unregisterFactory(ossimGeoPdfReaderFactory::instance());
         
      ossimInfoFactoryRegistry::instance()->
         unregisterFactory(ossimGeoPdfInfoFactory::instance());
   }
}
