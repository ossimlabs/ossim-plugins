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

static void setJpeg12Description(ossimString& description)
{
   description = "NITF 12 bit jpeg reader plugin\n";
}

extern "C"
{
   ossimSharedObjectInfo  myJpeg12Info;
   ossimString theJpeg12Description;
   std::vector<ossimString> theJpeg12ObjList;

   const char* getJpeg12Description()
   {
      return theJpeg12Description.c_str();
   }

   int getJpeg12NumberOfClassNames()
   {
      return (int)theJpeg12ObjList.size();
   }

   const char* getJpeg12ClassName(int idx)
   {
      if(idx < (int)theJpeg12ObjList.size())
      {
         return theJpeg12ObjList[idx].c_str();
      }
      return (const char*)0;
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize( ossimSharedObjectInfo** info, 
                                                        const char* options)
   {    
      myJpeg12Info.getDescription = getJpeg12Description;
      myJpeg12Info.getNumberOfClassNames = getJpeg12NumberOfClassNames;
      myJpeg12Info.getClassName = getJpeg12ClassName;
      
      *info = &myJpeg12Info;
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

      setJpeg12Description(theJpeg12Description);
      ossimJpeg12ReaderFactory::instance()->getTypeNameList(theJpeg12ObjList);
   }
   
   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
   {
      ossimImageHandlerRegistry::instance()->
         unregisterFactory(ossimJpeg12ReaderFactory::instance());
   }
}
