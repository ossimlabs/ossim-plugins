//----------------------------------------------------------------------------
//
// File: ossimOpenCvObjectFactory.cpp
// 
// License:  See top level LICENSE.txt file
//
// Author:  David Hicks
//
// Description: Factory for OSSIM OpenCvObject plugin.
//----------------------------------------------------------------------------

#include "ossimOpenCvObjectFactory.h"
#include "ossimTieMeasurementGenerator.h"
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimTrace.h>
#include <ossim/base/ossimKeywordNames.h>

static const ossimTrace traceDebug("ossimOpenCvObjectFactory:debug");

RTTI_DEF1(ossimOpenCvObjectFactory,
          "ossimOpenCvObjectFactory",
          ossimObjectFactory);

ossimOpenCvObjectFactory* ossimOpenCvObjectFactory::theInstance = 0;

ossimOpenCvObjectFactory::~ossimOpenCvObjectFactory()
{
   theInstance = 0;
}

ossimOpenCvObjectFactory* ossimOpenCvObjectFactory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimOpenCvObjectFactory;
   }
   return theInstance;
}


ossimObject* ossimOpenCvObjectFactory::createObject(
   const ossimString& typeName)const
{
   ossimObject* result = 0;

   if(typeName == "ossimTieMeasurementGenerator")
   {
      result = new ossimTieMeasurementGenerator;
   }
   return result;
}


ossimObject* ossimOpenCvObjectFactory::createObject(const ossimKeywordlist& kwl,
                                                    const char* prefix)const
{
   ossimObject* result = 0;

   const char* type = kwl.find(prefix, "type");
   if(type)
   {
      result = createObject(ossimString(type));
      if(result)
      {
         result->loadState(kwl, prefix);
      }
   }

   return result; 
}
 
void ossimOpenCvObjectFactory::getTypeNameList(std::vector<ossimString>& typeList)const
{
   typeList.push_back(ossimString("ossimTieMeasurementGenerator"));
}

ossimOpenCvObjectFactory::ossimOpenCvObjectFactory()
{
}

ossimOpenCvObjectFactory::ossimOpenCvObjectFactory(const ossimOpenCvObjectFactory&)
{
}

const ossimOpenCvObjectFactory& ossimOpenCvObjectFactory::operator=(const ossimOpenCvObjectFactory&)
{
   return *this;
}
