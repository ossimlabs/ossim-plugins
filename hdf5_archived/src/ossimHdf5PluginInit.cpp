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

#include <ossim/plugin/ossimPluginConstants.h>

#include "ossimHdf5PluginHandlerFactory.h"
#include "ossimViirsHandler.h"

static void setDescription(ossimString& description)
{
   description = "HDF5 Unclass plugin\n\n";

   vector<ossimString> renderables;
   ossimHdf5PluginHandlerFactory::instance()->getSupportedRenderableNames(renderables);

   description = description + "Datasets enabled for rendering:\n\n";
   ossim_uint32 idx = 0;
   for(idx = 0;idx < renderables.size();++idx)
   {
      description = description + renderables[idx] + "\n";
   }

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
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(ossimSharedObjectInfo** info)
   {    
      myInfo.getDescription = getDescription;
      myInfo.getNumberOfClassNames = getNumberOfClassNames;
      myInfo.getClassName = getClassName;

      *info = &myInfo;


      /* Register the readers... */
      ossimImageHandlerRegistry::instance()->
         registerFactory(ossimHdf5PluginHandlerFactory::instance());

      /* Register hdf projection factory... */
//      ossimProjectionFactoryRegistry::instance()->
//         registerFactoryToFront(ossimHdf5ProjectionFactory::instance());

      setDescription(theDescription);
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
   {
      ossimImageHandlerRegistry::instance()->
         unregisterFactory(ossimHdf5PluginHandlerFactory::instance());

//      ossimProjectionFactoryRegistry::instance()->
//         unregisterFactory(ossimHdf5ProjectionFactory::instance());
   }
}
