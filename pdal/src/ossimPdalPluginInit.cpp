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

static void setPdalDescription(ossimString& description)
{
   description = "pdal reader / writer plugin\n\n";
}

extern "C"
{
   ossimSharedObjectInfo  myPdalInfo;
   ossimString thePdalDescription;
   std::vector<ossimString> thePdalObjList;

   const char* getPdalDescription()
   {
      return thePdalDescription.c_str();
   }

   int getPdalNumberOfClassNames()
   {
      return (int)thePdalObjList.size();
   }

   const char* getPdalClassName(int idx)
   {
      if(idx < (int)thePdalObjList.size())
      {
         return thePdalObjList[idx].c_str();
      }
      return (const char*)0;
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(
      ossimSharedObjectInfo** info)
   {    
      myPdalInfo.getDescription = getPdalDescription;
      myPdalInfo.getNumberOfClassNames = getPdalNumberOfClassNames;
      myPdalInfo.getClassName = getPdalClassName;
      
      *info = &myPdalInfo;
      
      /* Register the readers... */
      ossimPointCloudHandlerRegistry::instance()->
         registerFactory(ossimPdalReaderFactory::instance());
      ossimPdalReaderFactory::instance()->getTypeNameList(thePdalObjList);
      
      setPdalDescription(thePdalDescription);
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
