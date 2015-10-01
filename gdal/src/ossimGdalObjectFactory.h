//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: Factory for OSSIM objects from the gdal plugin.
//----------------------------------------------------------------------------
// $Id: ossimGdalObjectFactory.h 10110 2006-12-14 18:20:54Z dburken $
#ifndef ossimGdalObjectFactory_HEADER
#define ossimGdalObjectFactory_HEADER 1

#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/base/ossimRtti.h>
#include <ossim/base/ossimObjectFactory.h>

class OSSIM_PLUGINS_DLL ossimGdalObjectFactory : public ossimObjectFactory
{
public:

   static ossimGdalObjectFactory* instance();

   /** virtual destructor */
   virtual ~ossimGdalObjectFactory();

   /**
    * @brief Object from string.
    * @return Pointer to object or 0 if not in this factory.
    */
   virtual ossimObject* createObject(const ossimString& typeName)const;

   /**
    * @brief Object from keyword list.
    * @return Pointer to object or 0 if not in this factory.
    */
   virtual ossimObject* createObject(const ossimKeywordlist& kwl,
                                     const char* prefix=0)const;
   
   /**
    * @brief Adds list of objects this factory supports.
    */
   virtual void getTypeNameList(std::vector<ossimString>& typeList)const;
   
private:

   /** hidden from use default construtor. */
   ossimGdalObjectFactory();

   /** hidden from use copy construtor. */
   ossimGdalObjectFactory(const ossimGdalObjectFactory& rhs);

   /** hidden from use operator= . */
   const ossimGdalObjectFactory& operator=(const ossimGdalObjectFactory& rhs);

   /** The single instance of this class. */
   static ossimGdalObjectFactory* theInstance;

TYPE_DATA
};

#endif /* #ifndef ossimGdalObjectFactory_HEADER */
