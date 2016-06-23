//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: OSSIM jpeg12 plugin initialization code.
//
//----------------------------------------------------------------------------
// $Id$

#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/plugin/ossimSharedObjectBridge.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include "ossimJpeg12ReaderFactory.h"

static void setDescription(ossimString& description)
{
   description = "NITF 12 bit jpeg reader plugin\n";
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
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize( ossimSharedObjectInfo** info, 
                                                        const char* options)
   {    
      myInfo.getDescription = getDescription;
      myInfo.getNumberOfClassNames = getNumberOfClassNames;
      myInfo.getClassName = getClassName;
      
      *info = &myInfo;
      ossimKeywordlist kwl;
      kwl.parseString(ossimString(options));
      if(ossimString(kwl.find("reader_factory.location")).downcase() == "front")
      {
         /* Register the readers to front... */
         ossimImageHandlerRegistry::instance()->
            registerFactoryToFront(ossimJpeg12ReaderFactory::instance());
      }
      else
      {
         /* Register the readers... */
         ossimImageHandlerRegistry::instance()->
            registerFactory(ossimJpeg12ReaderFactory::instance());
      }

      setDescription(theDescription);
   }
   
   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
   {
      ossimImageHandlerRegistry::instance()->
         unregisterFactory(ossimJpeg12ReaderFactory::instance());
   }
}
