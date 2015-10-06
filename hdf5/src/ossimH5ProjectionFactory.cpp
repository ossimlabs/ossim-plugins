//----------------------------------------------------------------------------
// License:  LGPL
//
// See LICENSE.txt file in the top level directory for more details.
//----------------------------------------------------------------------------
// $Id$

#include "ossimH5ProjectionFactory.h"
#include <ossim/base/ossimKeywordNames.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/projection/ossimProjection.h>
#include "ossimH5GridModel.h"

//---
// Define Trace flags for use within this file:
//---
#include <ossim/base/ossimTrace.h>
static ossimTrace traceExec  = ossimTrace("ossimH5ProjectionFactory:exec");
static ossimTrace traceDebug = ossimTrace("ossimH5ProjectionFactory:debug");

ossimH5ProjectionFactory* ossimH5ProjectionFactory::instance()
{
   static ossimH5ProjectionFactory* factoryInstance =
      new ossimH5ProjectionFactory();

   return factoryInstance;
}
   
ossimProjection* ossimH5ProjectionFactory::createProjection(
   const ossimFilename& filename, ossim_uint32 /*entryIdx*/)const
{
   static const char MODULE[] = "ossimH5ProjectionFactory::createProjection(ossimFilename& filename)";
   ossimRefPtr<ossimProjection> projection = 0;

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " DEBUG: testing ossimH5GridModel" << std::endl;
   }

   ossimKeywordlist kwl;
   ossimRefPtr<ossimProjection> model = 0;
   ossimFilename geomFile = filename;
   geomFile = geomFile.setExtension("geom");
   
   if(geomFile.exists()&&
      kwl.addFile(filename.c_str()))
   {
      ossimFilename coarseGrid;
      
      const char* type = kwl.find(ossimKeywordNames::TYPE_KW);
      if(type)
      {
         // ossimHdfGridModel included for backward compatibility.
         if( ( ossimString(type) == ossimString("ossimH5GridModel") ) ||
             ( ossimString(type) == ossimString("ossimHdfGridModel") ) )
         {
            ossimFilename coarseGrid = geomFile;
            geomFile.setFile(coarseGrid.fileNoExtension()+"_ocg");
            
            if(coarseGrid.exists() &&(coarseGrid != ""))
            {
               kwl.add("grid_file_name",
                       coarseGrid.c_str(),
                       true);
               projection = new ossimH5GridModel();
               if ( projection->loadState( kwl ) == false )
               {
                  projection = 0;
               }
            }
         }
      }
   }

   // Must release or pointer will self destruct when it goes out of scope.
   return projection.release();
}

ossimProjection* ossimH5ProjectionFactory::createProjection(
   const ossimString& name)const
{
   static const char MODULE[] = "ossimH5ProjectionFactory::createProjection(ossimString& name)";

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " DEBUG: Entering ...." << std::endl;
   }

   // ossimHdfGridModel included for backward compatibility.
   if ( ( name == "ossimH5GridModel" ) || ( name == "ossimHdfGridModel" ) )
   {
      return new ossimH5GridModel();
   }

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
           << MODULE << " DEBUG: Leaving ...." << std::endl;
   }

   return 0;
}

ossimProjection* ossimH5ProjectionFactory::createProjection(
   const ossimKeywordlist& kwl, const char* prefix)const
{
   ossimRefPtr<ossimProjection> result = 0;
   static const char MODULE[] = "ossimH5ProjectionFactory::createProjection(ossimKeywordlist& kwl)";

   if(traceDebug())
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " DEBUG: Start ...." << std::endl;
   }
   
   const char* lookup = kwl.find(prefix, ossimKeywordNames::TYPE_KW);
   if (lookup)
   {
      ossimString type = lookup;
      
      if ( (type == "ossimH5GridModel") || ( type == "ossimHdfGridModel" ) )
      {
         result = new ossimH5GridModel();
         if ( !result->loadState(kwl, prefix) )
         {
            result = 0;
         }
      }
   }

   if(traceDebug())
   {
    	ossimNotify(ossimNotifyLevel_DEBUG)
        	   << MODULE << " DEBUG: End ...." << std::endl;
   }
   
   return result.release();
}

ossimObject* ossimH5ProjectionFactory::createObject(
   const ossimString& typeName)const
{
   return createProjection(typeName);
}

ossimObject* ossimH5ProjectionFactory::createObject(
   const ossimKeywordlist& kwl, const char* prefix)const
{
   return createProjection(kwl, prefix);
}


void ossimH5ProjectionFactory::getTypeNameList(std::vector<ossimString>& typeList)const
{
   typeList.push_back(ossimString("ossimH5GridModel"));
}

ossimH5ProjectionFactory::ossimH5ProjectionFactory()
{
}
