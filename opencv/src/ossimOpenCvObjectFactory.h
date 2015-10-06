//----------------------------------------------------------------------------
//
// File: ossimOpenCvObjectFactory.h
// 
// License:  See top level LICENSE.txt file
//
// Author:  David Hicks
//
// Description: Factory for OSSIM OpenCvObject plugin.
//----------------------------------------------------------------------------

#ifndef ossimOpenCvObjectFactory_HEADER
#define ossimOpenCvObjectFactory_HEADER 1

#include <ossim/base/ossimObjectFactory.h>


class ossimString;
class ossimKeywordlist;

/** @brief Factory for ossimOpenCvObject. */
class ossimOpenCvObjectFactory : public ossimObjectFactory
{
public:

   /** @brief virtual destructor */
   virtual ~ossimOpenCvObjectFactory();

   /**
    * @brief static method to return instance (the only one) of this class.
    * @return pointer to instance of this class.
    */
   static ossimOpenCvObjectFactory* instance();

   /**
    * @brief createObject that takes a class name (eg. ossimTieMeasurementGenerator)
    * @param typeName (class, eg. "ossimTieMeasurementGenerator").
    */
   virtual ossimObject* createObject(const ossimString& typeName)const;
   
   /**
    * @brief Creates an object given a keyword list and prefix.
    * @param kwl The keyword list.
    * @param prefix the keyword list prefix.
    */
   virtual ossimObject* createObject(const ossimKeywordlist& kwl,
                                     const char* prefix=0)const;
   
   /**
    * @brief Adds ossimLasWriter to the typeList.
    * @param typeList List to add to.
    */
   virtual void getTypeNameList(std::vector<ossimString>& typeList)const;
  
protected:
   /** @brief hidden from use default constructor */
   ossimOpenCvObjectFactory();

   /** @brief hidden from use copy constructor */
   ossimOpenCvObjectFactory(const ossimOpenCvObjectFactory&);

   /** @brief hidden from use copy constructor */
   const ossimOpenCvObjectFactory& operator=(const ossimOpenCvObjectFactory&);

   /** static instance of this class */
   static ossimOpenCvObjectFactory* theInstance;

TYPE_DATA
};

#endif /* end of #ifndef ossimOpenCvObjectFactory_HEADER */
