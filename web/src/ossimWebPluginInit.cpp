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

static void setWebDescription(ossimString& description)
{
   description = "Web plugin\n";
   description += "\tsupport http, https web protocols\n";
}

extern "C"
{
   ossimSharedObjectInfo  myWebInfo;
   ossimString theWebDescription;
   std::vector<ossimString> theWebObjList;

   const char* getWebDescription()
   {
      return theWebDescription.c_str();
   }

   int getWebNumberOfClassNames()
   {
      return (int)theWebObjList.size();
   }

   const char* getWebClassName(int idx)
   {
      if(idx < (int)theWebObjList.size())
      {
         return theWebObjList[idx].c_str();
      }
      return (const char*)0;
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(
                                                       ossimSharedObjectInfo** info, 
                                                       const char* options)
   {    
      myWebInfo.getDescription = getWebDescription;
      myWebInfo.getNumberOfClassNames = getWebNumberOfClassNames;
      myWebInfo.getClassName = getWebClassName;
      
      *info = &myWebInfo;
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
      setWebDescription(theWebDescription);
      ossimWebPluginRequestFactory::instance()->getTypeNameList(theWebObjList);
   }
   
   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
   {
   }
}
