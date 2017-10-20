//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************

#include "ossimFftw3Factory.h"
#include <ossim/plugin/ossimSharedObjectBridge.h>
#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/base/ossimString.h>
#include <ossim/imaging/ossimImageSourceFactoryRegistry.h>

static void setFftw3Description(ossimString& description)
{
   description = "FFTW3 Filter plugin\n\n";
}

extern "C"
{
   ossimSharedObjectInfo  fftw3Info;
   ossimString fftw3Description;
   std::vector<ossimString> fftw3ObjList;

   const char* getFftw3Description()
   {
      return fftw3Description.c_str();
   }

   int getFftw3NumberOfClassNames()
   {
      return (int)fftw3ObjList.size();
   }

   const char* getFftw3ClassName(int idx)
   {
      if(idx < (int)fftw3ObjList.size())
      {
         return fftw3ObjList[idx].c_str();
      }
      return (const char*)0;
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryInitialize(
      ossimSharedObjectInfo** info)
   {    
      fftw3ObjList.push_back("ossimFftw3Filter");

      fftw3Info.getDescription = getFftw3Description;
      fftw3Info.getNumberOfClassNames = getFftw3NumberOfClassNames;
      fftw3Info.getClassName = getFftw3ClassName;
      
      *info = &fftw3Info;
      
      /* Register the filter... */
      ossimImageSourceFactoryRegistry::instance()->registerFactory(ossimFftw3Factory::instance());
      
      setFftw3Description(fftw3Description);
   }

   /* Note symbols need to be exported on windoze... */ 
   OSSIM_PLUGINS_DLL void ossimSharedLibraryFinalize()
   {
      ossimImageSourceFactoryRegistry::instance()->unregisterFactory(ossimFftw3Factory::instance());
   }
}
