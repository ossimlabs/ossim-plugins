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
// $Id: ossimKakaduPluginInit.cpp 20398 2011-12-20 16:34:24Z gpotts $

#include <ossim/plugin/ossimPluginConstants.h>
#include "ossimKakaduOverviewBuilderFactory.h"
#include "ossimKakaduReaderFactory.h"
#include "ossimKakaduWriterFactory.h"
#include <ossim/plugin/ossimSharedObjectBridge.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/imaging/ossimImageWriterFactoryRegistry.h>
#include <ossim/imaging/ossimOverviewBuilderFactoryRegistry.h>
#include "ossimKakaduJpipHandlerFactory.h"
#include "ossimKakaduJpipInfoFactory.h"
#include "ossimJpipProjectionFactory.h"
#include "ossimKakaduJpipImageGeometryFactory.h"
#include <ossim/support_data/ossimInfoFactoryRegistry.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <ossim/imaging/ossimImageGeometryRegistry.h>

static void setKakaduDescription(ossimString& description)
{
   description = "Kakadu (j2k) reader / writer plugin\n";
   description += "Kakadu (jpip) reader plugin\n";
   description += "\tsupports ingest via a keywordlist lr from a url of the form\n";
   description += "\tjpip://<server>:<port>/<path>\n";
}


extern "C"
{
   ossimSharedObjectInfo  myKakaduInfo;
   ossimString theKakaduDescription;
   std::vector<ossimString> theKakaduObjList;

   const char* getKakaduDescription()
   {
      return theKakaduDescription.c_str();
   }

   int getKakaduNumberOfClassNames()
   {
      return (int)theKakaduObjList.size();
   }

   const char* getKakaduClassName(int idx)
   {
      if(idx < (int)theKakaduObjList.size())
      {
         return theKakaduObjList[idx].c_str();
      }
      return (const char*)0;
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(
                                                       ossimSharedObjectInfo** info, 
                                                       const char* options)
   {    
      myKakaduInfo.getDescription = getKakaduDescription;
      myKakaduInfo.getNumberOfClassNames = getKakaduNumberOfClassNames;
      myKakaduInfo.getClassName = getKakaduClassName;
      
      *info = &myKakaduInfo;
      ossimKeywordlist kwl;
      kwl.parseString(ossimString(options));
      if(ossimString(kwl.find("reader_factory.location")).downcase() == "front")
      {
         /* Register the readers... */
         ossimImageHandlerRegistry::instance()->
         registerFactoryToFront(ossimKakaduReaderFactory::instance());

         ossimImageHandlerRegistry::instance()->
         registerFactoryToFront(ossimKakaduJpipHandlerFactory::instance());
      }
      else
      {
         /* Register the readers... */
         ossimImageHandlerRegistry::instance()->
         registerFactory(ossimKakaduReaderFactory::instance());
         ossimImageHandlerRegistry::instance()->
         registerFactory(ossimKakaduJpipHandlerFactory::instance());
      }


      if(ossimString(kwl.find("overview_builder_factory.location")).downcase() == "front")
      {
         /* Register the overview buildeds. */
         ossimOverviewBuilderFactoryRegistry::instance()->
         registerFactoryToFront(ossimKakaduOverviewBuilderFactory::instance());
      }
      else 
      {
         /* Register the overview buildeds. */
         ossimOverviewBuilderFactoryRegistry::instance()->
         registerFactory(ossimKakaduOverviewBuilderFactory::instance(),
                         false);
      }
      
      if(ossimString(kwl.find("writer_factory.location")).downcase() == "front")
      {
         /* Register the writers... */
         ossimImageWriterFactoryRegistry::instance()->
         registerFactoryToFront(ossimKakaduWriterFactory::instance());
         
      }
      else 
      {
      
         /* Register the writers... */
         ossimImageWriterFactoryRegistry::instance()->
         registerFactory(ossimKakaduWriterFactory::instance());
         
      }
      if(ossimString(kwl.find("projection_factory.location")).downcase() == "front")
      {
         ossimProjectionFactoryRegistry::instance()->registerFactoryToFront(ossimJpipProjectionFactory::instance());
         
      }
      else
      {
         ossimProjectionFactoryRegistry::instance()->registerFactory(ossimJpipProjectionFactory::instance());
      }
      if(ossimString(kwl.find("info_factory.location")).downcase() == "front")
      {
         ossimInfoFactoryRegistry::instance()->registerFactoryToFront(ossimKakaduJpipInfoFactory::instance());
         
      }
      else
      {
         ossimInfoFactoryRegistry::instance()->registerFactory(ossimKakaduJpipInfoFactory::instance());
      }
      if(ossimString(kwl.find("image_geometry_factory.location")).downcase() == "front")
      {
         ossimImageGeometryRegistry::instance()->registerFactoryToFront(ossimKakaduJpipImageGeometryFactory::instance());
      }
      else
      {
         ossimImageGeometryRegistry::instance()->registerFactory(ossimKakaduJpipImageGeometryFactory::instance());
      }
      setKakaduDescription(theKakaduDescription);
      ossimKakaduReaderFactory::instance()->getTypeNameList(theKakaduObjList);
      ossimKakaduJpipHandlerFactory::instance()->getTypeNameList(theKakaduObjList);
      ossimKakaduOverviewBuilderFactory::instance()->getTypeNameList(theKakaduObjList);
      ossimKakaduWriterFactory::instance()->getTypeNameList(theKakaduObjList);
      ossimJpipProjectionFactory::instance()->getTypeNameList(theKakaduObjList);
      ossimKakaduJpipImageGeometryFactory::instance()->getTypeNameList(theKakaduObjList);
      theKakaduObjList.push_back("ossimKakaduJpipInfo");
   }
   
   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
   {
      ossimImageHandlerRegistry::instance()->
         unregisterFactory(ossimKakaduReaderFactory::instance());
      
      ossimOverviewBuilderFactoryRegistry::instance()->
         unregisterFactory(ossimKakaduOverviewBuilderFactory::instance());
      
      ossimImageWriterFactoryRegistry::instance()->
         unregisterFactory(ossimKakaduWriterFactory::instance());
      
      ossimImageGeometryRegistry::instance()->unregisterFactory(ossimKakaduJpipImageGeometryFactory::instance());

   }
}
