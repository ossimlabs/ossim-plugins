//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author: David Burken
//
// Copied from Mingjie Su's ossimHdfPluginInit.
//
// Description: OSSIM HDF plugin initialization
// code.
//
//----------------------------------------------------------------------------
// $Id$

#include <ossim/plugin/ossimSharedObjectBridge.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageWriterFactoryRegistry.h>
#include <ossim/imaging/ossimOverviewBuilderFactoryRegistry.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <ossim/support_data/ossimInfoFactoryRegistry.h>

#include "../ossimPluginConstants.h"
#include "ossimH5ReaderFactory.h"
#include "ossimH5InfoFactory.h"
#include "ossimH5ProjectionFactory.h"

static void setDescription(ossimString& description)
{
   description = "HDF5 reader plugin\n\n";
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
         registerFactory(ossimH5ReaderFactory::instance());

      /* Register hdf info factoy... */
      ossimInfoFactoryRegistry::instance()->
         registerFactory(ossimH5InfoFactory::instance());

      /* Register hdf projection factoy... */
      ossimProjectionFactoryRegistry::instance()->
         registerFactoryToFront(ossimH5ProjectionFactory::instance());

      setDescription(theDescription);
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
   {
      ossimImageHandlerRegistry::instance()->
         unregisterFactory(ossimH5ReaderFactory::instance());

      ossimInfoFactoryRegistry::instance()->
         unregisterFactory(ossimH5InfoFactory::instance());

      ossimProjectionFactoryRegistry::instance()->
         unregisterFactory(ossimH5ProjectionFactory::instance());
   }
}
