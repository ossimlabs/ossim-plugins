//----------------------------------------------------------------------------
// 
// See top level LICENSE.txt file.
//
// Author:  David Burken
//
// Description: .
//
//----------------------------------------------------------------------------
// $Id: ossimGdalOverviewBuilderFactory.cpp 16273 2010-01-06 00:42:09Z gpotts $

#include <ossimGdalOverviewBuilderFactory.h>
#include <ossimGdalOverviewBuilder.h>

ossimGdalOverviewBuilderFactory*
ossimGdalOverviewBuilderFactory::theInstance = 0;

ossimGdalOverviewBuilderFactory* ossimGdalOverviewBuilderFactory::instance()
{
   if ( !theInstance )
   {
      theInstance = new ossimGdalOverviewBuilderFactory();
   }
   return theInstance;
}

ossimGdalOverviewBuilderFactory::~ossimGdalOverviewBuilderFactory()
{
   theInstance = 0;
}

ossimOverviewBuilderBase* ossimGdalOverviewBuilderFactory::createBuilder(
   const ossimString& typeName) const
{
   ossimRefPtr<ossimOverviewBuilderBase> result = new  ossimGdalOverviewBuilder();
   if ( result->hasOverviewType(typeName) == true )
   {
      // Capture the type.  (This builder has more than one.)
      result->setOverviewType(typeName);
   }
   else
   {
      result = 0;
   }
   
   return result.release();
}

void ossimGdalOverviewBuilderFactory::getTypeNameList(
   std::vector<ossimString>& typeList) const
{
   ossimRefPtr<ossimOverviewBuilderBase> builder = new  ossimGdalOverviewBuilder();
   builder->getTypeNameList(typeList);
}

ossimGdalOverviewBuilderFactory::ossimGdalOverviewBuilderFactory()
{
}

ossimGdalOverviewBuilderFactory::ossimGdalOverviewBuilderFactory(
   const ossimGdalOverviewBuilderFactory& /* obj */)
{
}

void ossimGdalOverviewBuilderFactory::operator=(
   const ossimGdalOverviewBuilderFactory& /* rhs */)
{
}
