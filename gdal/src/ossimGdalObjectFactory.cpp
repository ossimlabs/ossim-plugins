//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: Factory for OSSIM objects from the gdal plugin.
//----------------------------------------------------------------------------

#include <ossimGdalObjectFactory.h>
#include <ossimShapeDatabase.h>
#include <ossimShapeFile.h>
#include <ossimEsriShapeFileFilter.h>

RTTI_DEF1(ossimGdalObjectFactory,
          "ossimGdalObjectFactory",
          ossimObjectFactory);

ossimGdalObjectFactory* ossimGdalObjectFactory::theInstance = 0;

ossimGdalObjectFactory* ossimGdalObjectFactory::instance()
{
   if ( !theInstance )
   {
      theInstance = new ossimGdalObjectFactory();
   }
   return theInstance;
}

ossimGdalObjectFactory::~ossimGdalObjectFactory()
{}

ossimObject* ossimGdalObjectFactory::createObject(
   const ossimString& typeName)const
{
   ossimObject* result = 0;

   if (typeName == "ossimShapeFile")
   {
      result = new ossimShapeFile();
   }
   else if (typeName == "ossimShapeDatabase")
   {
      result = new ossimShapeDatabase();
   }
   else if (typeName == "ossimEsriShapeFileFilter")
   {
      result = new ossimEsriShapeFileFilter();
   }
   
   return result;
   
}

ossimObject* ossimGdalObjectFactory::createObject(const ossimKeywordlist& kwl,
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
   
void ossimGdalObjectFactory::getTypeNameList(
   std::vector<ossimString>& typeList)const
{
   typeList.push_back(ossimString("ossimShapeFile"));
   typeList.push_back(ossimString("ossimShapeDatabase"));
   typeList.push_back(ossimString("ossimEsriShapeFileFilter"));
}

ossimGdalObjectFactory::ossimGdalObjectFactory()
   : ossimObjectFactory()
{
}

ossimGdalObjectFactory::ossimGdalObjectFactory(
   const ossimGdalObjectFactory& /* rhs */)
   : ossimObjectFactory()
{
}

const ossimGdalObjectFactory& ossimGdalObjectFactory::operator=(
   const ossimGdalObjectFactory& /* rhs */ )
{
   return *this;
}
