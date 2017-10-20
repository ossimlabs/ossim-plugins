//*******************************************************************
//
// License:  LGPL
//
// See LICENSE.txt file in the top level directory for more details.
//
// Author: Garrett Potts
//*******************************************************************
//  $Id$
#include <ossim/plugin/ossimSharedObjectBridge.h>
#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/base/ossimObjectFactoryRegistry.h>
#include <ossim/imaging/ossimImageHandlerRegistry.h>
#include <ossim/projection/ossimProjectionFactoryRegistry.h>
#include <ossimPluginProjectionFactory.h>
#include <ossimPluginReaderFactory.h>


namespace ossimplugins
{


   static void setCnesDescription(ossimString& description)
   {
      description = "OSSIM Plugin\n\n";
      std::vector<ossimString> projectionTypes;

      ossimPluginProjectionFactory::instance()->getTypeNameList(projectionTypes);
      ossim_uint32 idx = 0;
      description = "Projecitons Supported:\n\n";
      for(idx = 0; idx < projectionTypes.size();++idx)
      {
         description += projectionTypes[idx] + "\n";
      }
   }


   extern "C"
   {
      ossimSharedObjectInfo  cnesInfo;
      ossimString cnesDescription;
      std::vector<ossimString> cnesObjList;
      const char* getCnesDescription()
      {
         return cnesDescription.c_str();
      }
      int getCnesNumberOfClassNames()
      {
         return (int)cnesObjList.size();
      }
      const char* getCnesClassName(int idx)
      {
         if(idx < (int)cnesObjList.size())
         {
            return cnesObjList[idx].c_str();
         }
         return (const char*)0;
      }

      /* Note symbols need to be exported on windoze... */
      OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(
         ossimSharedObjectInfo** info)
      {
         cnesInfo.getDescription = getCnesDescription;
         cnesInfo.getNumberOfClassNames = getCnesNumberOfClassNames;
         cnesInfo.getClassName = getCnesClassName;

         *info = &cnesInfo;

         /** Register the readers... */
         ossimImageHandlerRegistry::instance()->
            registerFactory(ossimPluginReaderFactory::instance());

         /**
          * Register the projection factory.
          * Note that this must be pushed to the front of the factory or bilinear
          * projection will be picked up.
          */
         ossimProjectionFactoryRegistry::instance()->
            registerFactoryToFront(ossimPluginProjectionFactory::instance());

         setCnesDescription(cnesDescription);
         ossimPluginReaderFactory::instance()->getTypeNameList(cnesObjList);
         ossimPluginProjectionFactory::instance()->getTypeNameList(cnesObjList);
      }

      /* Note symbols need to be exported on windoze... */
      OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
      {
         ossimImageHandlerRegistry::instance()->
            unregisterFactory(ossimPluginReaderFactory::instance());

         ossimProjectionFactoryRegistry::instance()->
            unregisterFactory(ossimPluginProjectionFactory::instance());
      }

   }
}
