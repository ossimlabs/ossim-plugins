//----------------------------------------------------------------------------
//
// "Copyright Centre National d'Etudes Spatiales"
//
// License:  LGPL
//
// See LICENSE.txt file in the top level directory for more details.
//
//----------------------------------------------------------------------------
// $Id$

//***
// Note to programmer: To add a new model, search this file for "ADD_MODEL"
// to locate places requiring editing. Functional example below...
//
// ADD_MODEL: Include all sensor model headers here:
//***
#include "ossimPluginProjectionFactory.h"
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/projection/ossimProjection.h>
#include "ossimRadarSatModel.h"
#include "ossimRadarSat2RPCModel.h"
#include "ossimEnvisatAsarModel.h"
#include "ossimTerraSarModel.h"
//#include <ossim/projection/ossimCosmoSkymedModel.h>
#include "ossimRadarSat2Model.h"
#include "ossimErsSarModel.h"
#include "ossimAlosPalsarModel.h"
#include "ossimPleiadesModel.h"
#include <ossim/base/ossimNotifyContext.h>
#include "ossimTileMapModel.h"
#include "ossimSpot6Model.h"

//***
// Define Trace flags for use within this file:
//***
#include <ossim/base/ossimTrace.h>
static ossimTrace traceExec  = ossimTrace("ossimPluginProjectionFactory:exec");
static ossimTrace traceDebug = ossimTrace("ossimPluginProjectionFactory:debug");

#define TRACE_DEBUG if (traceDebug()) ossimNotify(ossimNotifyLevel_DEBUG)<<MODULE<<" DEBUG: "

#include <ossimFormosatModel.h>
#include <ossimFormosatDimapSupportData.h>

namespace ossimplugins
{


ossimPluginProjectionFactory* ossimPluginProjectionFactory::instance()
{
   static ossimPluginProjectionFactory* factoryInstance =
      new ossimPluginProjectionFactory();

   return factoryInstance;
}


ossimProjection* ossimPluginProjectionFactory::createProjection(
   const ossimFilename& filename, ossim_uint32 /*entryIdx*/)const
{
   static const char MODULE[] = "ossimPluginProjectionFactory::createProjection(ossimFilename& filename)";
   ossimRefPtr<ossimProjection> projection = 0;
   //traceDebug.setTraceFlag(true);

   if ( !projection )
   {
      TRACE_DEBUG<<"testing ossimRadarSat2RPCModel"<<std::endl;
   
      ossimRefPtr<ossimRadarSat2RPCModel> model = new ossimRadarSat2RPCModel();
      projection = model->open(filename) ? model.get() : 0;
   }
   
   if ( !projection )
   {
      TRACE_DEBUG<<"testing ossimRadarSat2Model" << std::endl;

      ossimRefPtr<ossimRadarSat2Model> model = new ossimRadarSat2Model();
      if (model->open(filename))
      {
         // Check if a coarse grid was generated, and use it instead:
         projection = model->getReplacementOcgModel().get();
         if (!projection)
            projection = model.get();
      }
   }

   // Pleiades
   if ( !projection )
   {
      TRACE_DEBUG<<"testing ossimPleiadesModel" << std::endl;

      ossimRefPtr<ossimPleiadesModel> model = new ossimPleiadesModel();
      projection = model->open(filename) ? model.get() : 0;
   }

   if ( !projection )
   {
      TRACE_DEBUG<<"testing ossimTerraSarModel" << std::endl;

      ossimRefPtr<ossimTerraSarModel> model = new ossimTerraSarModel();
      if (model->open(filename))
      {
         // Check if a coarse grid was generated, and use it instead:
         projection = model->getReplacementOcgModel().get();
         if (!projection)
            projection = model.get();
      }
   }

   if ( !projection )
   {
      TRACE_DEBUG<<"testing ossimErsSarModel" << std::endl;

      ossimRefPtr<ossimErsSarModel> model = new ossimErsSarModel();
      projection = model->open(filename) ? model.get() : 0;
   }

   if (!projection)
   {
      TRACE_DEBUG<<"testing ossimEnvisatSarModel" << std::endl;

      ossimRefPtr<ossimEnvisatAsarModel> model = new ossimEnvisatAsarModel();
      projection = model->open(filename) ? model.get() : 0;
   }

   if (!projection)
   {
      TRACE_DEBUG<<"testing ossimRadarSatModel" << std::endl;

      ossimRefPtr<ossimRadarSatModel> model = new ossimRadarSatModel();
      projection = model->open(filename) ? model.get() : 0;
   }

   if (!projection)
   {
      TRACE_DEBUG<<"testing ossimAlosPalsarModel" << std::endl;

      ossimRefPtr<ossimAlosPalsarModel> model = new ossimAlosPalsarModel();
      projection = model->open(filename) ? model.get() : 0;
   }

   if (!projection)
   {
      TRACE_DEBUG<<"testing ossimFormosatModel" << std::endl;

      ossimFilename formosatTest = filename;
      formosatTest = formosatTest.setExtension("geom");
      if (!formosatTest.exists())
      {
         formosatTest = filename.path();
         formosatTest = formosatTest.dirCat(ossimFilename("METADATA.DIM"));
         if (formosatTest.exists() == false)
         {
            formosatTest = filename.path();
            formosatTest = formosatTest.dirCat(ossimFilename("metadata.dim"));
         }
      }
      if (formosatTest.exists())
      {
         //---
         // Check the basename of the input file. So we don't create a projection
         // for ancillary files, icon.jpg amd preview.jpg.
         //---
         ossimFilename baseName = filename.file();
         baseName.downcase();

         if ((baseName != "icon.jpg") && (baseName != "preview.jpg"))
         {
            ossimRefPtr<ossimFormosatDimapSupportData> meta = new ossimFormosatDimapSupportData;
            if (meta->loadXmlFile(formosatTest))
            {
               ossimRefPtr<ossimFormosatModel> model = new ossimFormosatModel(meta.get());
               projection = model->getErrorStatus() ? 0 : model.get();
            }
         }
      }
   }

   if (!projection)
   {
      TRACE_DEBUG<<"testing ossimTileMapModel" << std::endl;

      ossimRefPtr<ossimTileMapModel> model = new ossimTileMapModel();
      projection = model->open(filename) ? model.get() : 0;
   }

   // Spot6
   if ( !projection )
   {
      TRACE_DEBUG<<"testing ossimSpot6Model" << std::endl;

      ossimRefPtr<ossimSpot6Model> model = new ossimSpot6Model();
      projection = model->open(filename) ? model.get() : 0;
   }

   //***
   // ADD_MODEL: (Please leave this comment for the next programmer)
   //***
   // MY_NEW_MODEL
   //if ( !projection )
   //{
   //    TRACE_DEBUG<<"testing MY_NEW_MODEL"<<std::endl;
   //    ossimRefPtr<MY_NEW_MODEL> model = new MY_NEW_MODEL();
   //    projection = model->open(filename) ? model.get() : 0;
   //}

   return projection.release();
}

ossimProjection* ossimPluginProjectionFactory::createProjection(
   const ossimString& name)const
{
   static const char MODULE[] = "ossimPluginProjectionFactory::createProjection(ossimString& name)";

   TRACE_DEBUG<<"Entering ...." << std::endl;

   //   else if (name == STATIC_TYPE_NAME(ossimCosmoSkymedModel))
   //    {
   //      return new ossimCosmoSkymedModel;
   //   }
   if (name == STATIC_TYPE_NAME(ossimRadarSat2Model))
   {
      return new ossimRadarSat2Model();
   }
   else if (name == STATIC_TYPE_NAME(ossimTerraSarModel))
   {
      return new ossimTerraSarModel();
   }
   else if (name == STATIC_TYPE_NAME(ossimErsSarModel))
   {
     return new ossimErsSarModel;
   }
   else if (name == STATIC_TYPE_NAME(ossimEnvisatAsarModel))
   {
     return new ossimEnvisatAsarModel;
   }
   else if (name == STATIC_TYPE_NAME(ossimRadarSatModel))
   {
     return new ossimRadarSatModel;
   }
   else if (name == STATIC_TYPE_NAME(ossimAlosPalsarModel))
   {
     return new ossimAlosPalsarModel;
   }
   else if (name == STATIC_TYPE_NAME(ossimFormosatModel))
   {
     return new ossimFormosatModel;
   }
   else if (name == STATIC_TYPE_NAME(ossimTileMapModel))
   {
     return new ossimTileMapModel;
   }
   else if (name == STATIC_TYPE_NAME(ossimPleiadesModel))
   {
     return new ossimPleiadesModel;
   }
   else if (name == STATIC_TYPE_NAME(ossimSpot6Model))
   {
     return new ossimSpot6Model;
   }

   //***
   // ADD_MODEL: (Please leave this comment for the next programmer)
   //***
//   if(name == MY_NEW_MODEL)
//      return new myNewModel;

   TRACE_DEBUG<<"Leaving ...." << std::endl;

   return 0;
}

ossimProjection* ossimPluginProjectionFactory::createProjection(
   const ossimKeywordlist& kwl, const char* prefix)const
{
   ossimRefPtr<ossimProjection> result = 0;
   static const char MODULE[] = "ossimPluginProjectionFactory::createProjection(ossimKeywordlist& kwl)";

   TRACE_DEBUG<<"Start ...." << std::endl;

   const char* lookup = kwl.find(prefix, ossimKeywordNames::TYPE_KW);
   if (lookup)
   {
      ossimString type = lookup;

      if (type == "ossimRadarSat2Model")
      {
         result = new ossimRadarSat2Model();
         if ( !result->loadState(kwl, prefix) )
         {
            result = 0;
         }
      }
      else if (type == "ossimTerraSarModel")
      {
         result = new ossimTerraSarModel();
         if ( !result->loadState(kwl, prefix) )
         {
            result = 0;
         }
      }
      else if (type == "ossimErsSarModel")
      {
         result = new ossimErsSarModel();
         if ( !result->loadState(kwl, prefix) )
         {
            result = 0;
         }
      }
      else if (type == "ossimEnvisatAsarModel")
      {
         result = new ossimEnvisatAsarModel();
         if ( !result->loadState(kwl, prefix) )
         {
            result = 0;
         }
      }
      else if (type == "ossimRadarSatModel")
      {
         result = new ossimRadarSatModel();
         if ( !result->loadState(kwl, prefix) )
         {
            result = 0;
         }
      }
      else if (type == "ossimAlosPalsarModel")
      {
         result = new ossimAlosPalsarModel();
         if ( !result->loadState(kwl, prefix) )
         {
            result = 0;
         }
      }
      else if (type == "ossimFormosatModel")
      {
         result = new ossimFormosatModel();
         if ( !result->loadState(kwl, prefix) )
         {
            result = 0;
         }
      }
      else if (type == "ossimTileMapModel")
      {
         result = new ossimTileMapModel();
         if ( !result->loadState(kwl, prefix) )
         {
            result = 0;
         }
      }
      else if (type == "ossimPleiadesModel")
      {
         result = new ossimPleiadesModel();
         if ( !result->loadState(kwl, prefix) )
         {
            result = 0;
         }
      }
      else if (type == "ossimSpot6Model")
      {
         result = new ossimSpot6Model();
         if ( !result->loadState(kwl, prefix) )
         {
            result = 0;
         }
      }

   //***
   // ADD_MODEL: (Please leave this comment for the next programmer)
   //***
//      else if (type == "ossimSpot6Model")
//      {
//         result = new ossimSpot6Model();
//         if ( !result->loadState(kwl, prefix) )
//         {
//            result = 0;
//         }
//      }

   }

   TRACE_DEBUG<<"End ...." << std::endl;

   return result.release();
}

ossimObject* ossimPluginProjectionFactory::createObject(
   const ossimString& typeName)const
{
   return createProjection(typeName);
}

ossimObject* ossimPluginProjectionFactory::createObject(
   const ossimKeywordlist& kwl, const char* prefix)const
{
   return createProjection(kwl, prefix);
}


void ossimPluginProjectionFactory::getTypeNameList(std::vector<ossimString>& typeList)const
{
   typeList.push_back(STATIC_TYPE_NAME(ossimRadarSatModel));
   typeList.push_back(STATIC_TYPE_NAME(ossimRadarSat2Model));
   typeList.push_back(STATIC_TYPE_NAME(ossimTerraSarModel));
   //   result.push_back(STATIC_TYPE_NAME(ossimCosmoSkymedModel));
   typeList.push_back(STATIC_TYPE_NAME(ossimEnvisatAsarModel));
   typeList.push_back(STATIC_TYPE_NAME(ossimErsSarModel));
   typeList.push_back(STATIC_TYPE_NAME(ossimAlosPalsarModel));
   typeList.push_back(STATIC_TYPE_NAME(ossimFormosatModel));
   typeList.push_back(STATIC_TYPE_NAME(ossimTileMapModel));
   typeList.push_back(STATIC_TYPE_NAME(ossimPleiadesModel));
   typeList.push_back(STATIC_TYPE_NAME(ossimSpot6Model));

   //***
   // ADD_MODEL: Please leave this comment for the next programmer. Add above.
   //***
   //typeList.push_back(STATIC_TYPE_NAME(MY_NEW_MODEL));
}

bool ossimPluginProjectionFactory::isTileMap(const ossimFilename& filename)const
{
   ossimFilename temp(filename);
   temp.downcase();
  
   ossimString os = temp.beforePos(4);
  
   if(temp.ext()=="otb")
   {
      return true;
   }
   else if(os == "http")
   {
      return true;
   }
   return false;
}

} // End: namespace ossimplugins
