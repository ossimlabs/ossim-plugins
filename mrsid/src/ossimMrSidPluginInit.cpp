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

static void setMrSidDescription(ossimString& description)
{
#ifdef OSSIM_ENABLE_MRSID_WRITE 
   description = "MrSid reader / writer plugin\n\n";
#else
   description = "MrSid reader plugin\n\n";
#endif
}

extern "C"
{
   ossimSharedObjectInfo  myMrSidInfo;
   ossimString theMrSidDescription;
   std::vector<ossimString> theMrSidObjList;

   const char* getMrSidDescription()
   {
      return theMrSidDescription.c_str();
   }

   int getMrSidNumberOfClassNames()
   {
      return (int)theMrSidObjList.size();
   }

   const char* getMrSidClassName(int idx)
   {
      if(idx < (int)theMrSidObjList.size())
      {
         return theMrSidObjList[idx].c_str();
      }
      return (const char*)0;
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(
      ossimSharedObjectInfo** info)
   {    
      myMrSidInfo.getDescription = getMrSidDescription;
      myMrSidInfo.getNumberOfClassNames = getMrSidNumberOfClassNames;
      myMrSidInfo.getClassName = getMrSidClassName;
      
      *info = &myMrSidInfo;
      
      /* Register the readers... */
      ossimImageHandlerRegistry::instance()->
         registerFactory(ossimMrSidReaderFactory::instance(), false);
      ossimMrSidReaderFactory::instance()->getTypeNameList(theMrSidObjList);

#ifdef OSSIM_ENABLE_MRSID_WRITE
      /* Register the writers... */
      ossimImageWriterFactoryRegistry::instance()->
         registerFactory(ossimMrSidWriterFactory::instance());
      ossimMrSidWriterFactory::instance()->getTypeNameList(theMrSidObjList);
#endif
      
      setMrSidDescription(theMrSidDescription);

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
