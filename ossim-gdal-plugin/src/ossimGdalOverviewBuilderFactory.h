//----------------------------------------------------------------------------
// 
// See top level LICENSE.txt file.
//
// Author:  David Burken
//
// Description: The ossim overview builder factory.
//
//----------------------------------------------------------------------------
// $Id: ossimGdalOverviewBuilderFactory.h 15833 2009-10-29 01:41:53Z eshirschorn $

#ifndef ossimGdalOverviewBuilderFactory_HEADER
#define ossimGdalOverviewBuilderFactory_HEADER

#include <ossim/base/ossimConstants.h>
#include <ossim/imaging/ossimOverviewBuilderFactoryBase.h>


class ossimOverviewBuilderBase;
class ossimString;

/**
 * @class ossimGdalOverviewBuilderFactory
 * @brief The ossim overview builder factory.
 */
class ossimGdalOverviewBuilderFactory:
   public ossimOverviewBuilderFactoryBase
{
public:

   /** @brief static instance method. */
   static ossimGdalOverviewBuilderFactory* instance();
   
   /** virtual destructor */
   virtual ~ossimGdalOverviewBuilderFactory();

   /**
    * @brief Creates a builder from a string.  This should match a string from
    * the getTypeNameList() method.  Pure virtual.
    * 
    * @return Pointer to ossimOverviewBuilderBase or NULL is not found
    * within registered factories.
    */
   virtual ossimOverviewBuilderBase* createBuilder(
      const ossimString& typeName) const;

   /**
    * @brief Method to populate a list of supported types for the factory.
    * registered to this registry.  Pure virtual.
    *
    * @param typeList List of ossimStrings to add to.
    */
   virtual void getTypeNameList(std::vector<ossimString>& typeList)const;

private:
   /** default constructor hidden from use */
   ossimGdalOverviewBuilderFactory();

   /** copy constructor hidden from use */
   ossimGdalOverviewBuilderFactory(const ossimGdalOverviewBuilderFactory& obj);

   /** operator= hidden from use. */
   void operator=(const ossimGdalOverviewBuilderFactory& rhs);

   static ossimGdalOverviewBuilderFactory* theInstance;
};

#endif /* #ifndef ossimGdalOverviewBuilderFactory_HEADER */
