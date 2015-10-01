//----------------------------------------------------------------------------
// 
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  David Burken
//
// Description: The ossim kakadu overview builder factory.
//
//----------------------------------------------------------------------------
// $Id: ossimKakaduOverviewBuilderFactory.h 22884 2014-09-12 13:14:35Z dburken $

#ifndef ossimKakaduOverviewBuilderFactory_HEADER
#define ossimKakaduOverviewBuilderFactory_HEADER 1

#include <ossim/base/ossimConstants.h>
#include <ossim/imaging/ossimOverviewBuilderFactoryBase.h>

class ossimOverviewBuilderBase;
class ossimString;

/**
 * @class ossimKakaduOverviewBuilderFactory
 * @brief The ossim overview builder factory.
 */
class ossimKakaduOverviewBuilderFactory:
   public ossimOverviewBuilderFactoryBase
{
public:

   /** @brief static instance method. */
   static ossimKakaduOverviewBuilderFactory* instance();
   
   /** virtual destructor */
   virtual ~ossimKakaduOverviewBuilderFactory();

   /**
    * @brief Creates a builder from a string.  This should match a string from
    * the getTypeNameList() method.  Pure virtual.
    * 
    * @return Pointer to ossimOverviewBuilderInterface or NULL is not found
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
   ossimKakaduOverviewBuilderFactory();

   /** copy constructor hidden from use */
   ossimKakaduOverviewBuilderFactory(
      const ossimKakaduOverviewBuilderFactory& obj);

   /** operator= hidden from use. */
   void operator=(const ossimKakaduOverviewBuilderFactory& rhs);

};

#endif /* #ifndef ossimKakaduOverviewBuilderFactory_HEADER */
