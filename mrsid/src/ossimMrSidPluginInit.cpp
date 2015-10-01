//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  Mingjie Su
//
// Description: OSSIM MrSID plugin initialization
// code.
//
//----------------------------------------------------------------------------
// $Id: ossimMrSidPluginInit.cpp 899 2010-05-17 21:00:26Z david.burken $

#include <ossim/plugin/ossimSharedObjectBridge.h>
#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageWriterFactoryRegistry.h>
#include <ossim/imaging/ossimOverviewBuilderFactoryRegistry.h>

#include "ossimMrSidReaderFactory.h"
#include "ossimMrSidWriterFactory.h"

static void setDescription(ossimString& description)
{
#ifdef OSSIM_ENABLE_MRSID_WRITE 
   description = "MrSid reader / writer plugin\n\n";
#else
   description = "MrSid reader plugin\n\n";
#endif
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
         registerFactory(ossimMrSidReaderFactory::instance(), false);

#ifdef OSSIM_ENABLE_MRSID_WRITE
      /* Register the writers... */
      ossimImageWriterFactoryRegistry::instance()->
         registerFactory(ossimMrSidWriterFactory::instance());
#endif
      
      setDescription(theDescription);
   }
   
   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
   {
      ossimImageHandlerRegistry::instance()->
         unregisterFactory(ossimMrSidReaderFactory::instance());

#ifdef OSSIM_ENABLE_MRSID_WRITE      
      ossimImageWriterFactoryRegistry::instance()->
         unregisterFactory(ossimMrSidWriterFactory::instance());
#endif
   }
}
