//**************************************************************************************************
//
//     OSSIM Open Source Geospatial Data Processing Library
//     See top level LICENSE.txt file for license information
//
//**************************************************************************************************


#include "ossimFftw3Factory.h"
#include "ossimFftw3Filter.h"
#include <ossim/imaging/ossimImageSourceFactoryRegistry.h>
#include <ossim/base/ossimTrace.h>

using namespace std;

RTTI_DEF1(ossimFftw3Factory, "ossimFftw3Factory", ossimImageSourceFactoryBase);

static ossimTrace traceDebug("ossimImageSourceFactory:debug");

ossimFftw3Factory* ossimFftw3Factory::theInstance=NULL;

ossimFftw3Factory::~ossimFftw3Factory()
{
   theInstance = NULL;
   ossimImageSourceFactoryRegistry::instance()->unregisterFactory(this);
}
ossimFftw3Factory* ossimFftw3Factory::instance()
{
   if(!theInstance)
   {
      theInstance = new ossimFftw3Factory;
   }

   return theInstance;
}

ossimObject* ossimFftw3Factory::createObject(const ossimString& name)const
{

   // lets do the filters first
   if( name == STATIC_TYPE_NAME(ossimFftw3Filter))
      return new ossimFftw3Filter;

   return NULL;
}

ossimObject* ossimFftw3Factory::createObject(const ossimKeywordlist& kwl,
                                                   const char* prefix)const
{
   static const char* MODULE = "ossimFftw3Factory::createSource";
   
   ossimString copyPrefix;
   if (prefix)
   {
      copyPrefix = prefix;
   }
   
   ossimObject* result = NULL;
   
   if(traceDebug())
   {
      CLOG << "looking up type keyword for prefix = " << copyPrefix << endl;
   }

   const char* lookup = kwl.find(copyPrefix, "type");
   if(lookup)
   {
      ossimString name = lookup;
      result           = createObject(name);
      
      if(result)
      {
         if(traceDebug())
         {
            CLOG << "found source " << result->getClassName() << " now loading state" << endl;
         }
         result->loadState(kwl, copyPrefix.c_str());
      }
      else
      {
         if(traceDebug())
         {
            CLOG << "type not found " << lookup << endl;
         }
      }
   }
   else
   {
      if(traceDebug())
      {
         CLOG << "type keyword not found" << endl;
      }
   }
   return result;
}

void ossimFftw3Factory::getTypeNameList(std::vector<ossimString>& typeList)const
{
   typeList.push_back(STATIC_TYPE_NAME(ossimFftw3Filter));
}

// Hide from use...
ossimFftw3Factory::ossimFftw3Factory()
   :ossimImageSourceFactoryBase()
{}

ossimFftw3Factory::ossimFftw3Factory(const ossimFftw3Factory&)
   :ossimImageSourceFactoryBase()
{}

const ossimFftw3Factory& ossimFftw3Factory::operator=(ossimFftw3Factory&)
{
   return *this;
}
