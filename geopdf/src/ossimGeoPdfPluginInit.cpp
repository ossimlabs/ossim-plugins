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

static void setGeoPdfDescription(ossimString& description)
{
   description = "GeoPdf reader plugin\n\n";
}


extern "C"
{
   ossimSharedObjectInfo  myGeoPdfInfo;
   ossimString theGeoPdfDescription;
   std::vector<ossimString> theGeoPdfObjList;

   const char* getGeoPdfDescription()
   {
      return theGeoPdfDescription.c_str();
   }

   int getGeoPdfNumberOfClassNames()
   {
      return (int)theGeoPdfObjList.size();
   }

   const char* getGeoPdfClassName(int idx)
   {
      if(idx < (int)theGeoPdfObjList.size())
      {
         return theGeoPdfObjList[idx].c_str();
      }
      return (const char*)0;
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(
      ossimSharedObjectInfo** info)
   {    
      myGeoPdfInfo.getDescription = getGeoPdfDescription;
      myGeoPdfInfo.getNumberOfClassNames = getGeoPdfNumberOfClassNames;
      myGeoPdfInfo.getClassName = getGeoPdfClassName;
      
      *info = &myGeoPdfInfo;
      
      /* Register the readers... */
      ossimImageHandlerRegistry::instance()->
         registerFactory(ossimGeoPdfReaderFactory::instance(), false);

      /* Register GeoPdf info objects... */
      ossimInfoFactoryRegistry::instance()->
        registerFactory(ossimGeoPdfInfoFactory::instance());

      setGeoPdfDescription(theGeoPdfDescription);
      ossimGeoPdfReaderFactory::instance()->getTypeNameList(theGeoPdfObjList);
      theGeoPdfObjList.push_back("ossimGeoPdfInfo");
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
