//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description:
// 
// OSSIM Point Data Abstractin Library(PDAL) plugin initialization code.
//
//----------------------------------------------------------------------------
// $Id

#include <ossim/plugin/ossimSharedObjectBridge.h>
#include <ossim/plugin/ossimPluginConstants.h>
#include "ossimPdalReaderFactory.h"
//#include "ossimPdalImageHandlerFactory.h"
#include <ossim/base/ossimString.h>
#include <ossim/point_cloud/ossimPointCloudHandlerRegistry.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>

static void setDescription(ossimString& description)
{
   description = "pdal reader / writer plugin\n\n";
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
      ossimPointCloudHandlerRegistry::instance()->
         registerFactory(ossimPdalReaderFactory::instance());
      
      //ossimImageHandlerRegistry::instance()->
      //   registerFactory(ossimPdalImageHandlerFactory::instance());

      setDescription(theDescription);
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
   {
      ossimPointCloudHandlerRegistry::instance()->
         unregisterFactory(ossimPdalReaderFactory::instance());
      //ossimImageHandlerRegistry::instance()->
      //   unregisterFactory(ossimPdalImageHandlerFactory::instance());
   }
}
