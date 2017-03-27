//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: OSSIM Kakadu plugin initialization
// code.
//
//----------------------------------------------------------------------------
// $Id: ossimKakaduPluginInit.cpp 20202 2011-11-04 13:57:13Z gpotts $

#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/plugin/ossimSharedPluginRegistry.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimWebRequestFactoryRegistry.h>
#include "ossimWebPluginRequestFactory.h"
#include <ossim/base/ossimStreamFactoryRegistry.h>
#include "CurlStreamDefaults.h"
#include "ossimCurlStreamFactory.h"

static void setDescription(ossimString& description)
{
   description = "Web plugin\n";
   description += "\tsupport http, https web protocols\n";
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
                                                       ossimSharedObjectInfo** info, 
                                                       const char* options)
   {    
      myInfo.getDescription = getDescription;
      myInfo.getNumberOfClassNames = getNumberOfClassNames;
      myInfo.getClassName = getClassName;
      
      *info = &myInfo;
      ossimKeywordlist kwl;
      kwl.parseString(ossimString(options));

      ossim::CurlStreamDefaults::loadDefaults();
      /* Register our stream factory... */
      ossim::StreamFactoryRegistry::instance()->
         registerFactory( ossim::CurlStreamFactory::instance() );

      if(ossimString(kwl.find("reader_factory.location")).downcase() == "front")
      {
         /* Register the readers... */
         ossimWebRequestFactoryRegistry::instance()->
         registerFactoryToFront(ossimWebPluginRequestFactory::instance());
      }
      else
      {
         /* Register the readers... */
         ossimWebRequestFactoryRegistry::instance()->
         registerFactory(ossimWebPluginRequestFactory::instance());
      }
      setDescription(theDescription);
   }
   
   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
   {
   }
}
