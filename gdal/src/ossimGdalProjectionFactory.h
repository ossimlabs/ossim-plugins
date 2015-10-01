//*******************************************************************
// Copyright (C) 2005 Garrett Potts
//
// License:  See top level LICENSE.txt file.
// 
// Author:  Garrett Potts
//
//*******************************************************************
//  $Id: ossimGdalProjectionFactory.h 19908 2011-08-05 19:57:34Z dburken $

#ifndef ossimGdalProjectionFactory_HEADER
#define ossimGdalProjectionFactory_HEADER

#include <ossim/projection/ossimProjectionFactoryBase.h>
#include "ossimOgcWktTranslator.h"
#include <ossim/plugin/ossimPluginConstants.h>
#include <list>
class ossimProjection;
class ossimString;

class ossimGdalProjectionFactory : public ossimProjectionFactoryBase
{
public:
   /*!
    * METHOD: instance()
    * Instantiates singleton instance of this class:
    */
   static ossimGdalProjectionFactory* instance();

   virtual ossimProjection* createProjection(const ossimFilename& filename,
                                             ossim_uint32 entryIdx)const;
   /*!
    * METHOD: create()
    * Attempts to create an instance of the projection specified by name.
    * Returns successfully constructed projection or NULL.
    */
   virtual ossimProjection* createProjection(const ossimString& name)const;
   virtual ossimProjection* createProjection(const ossimKeywordlist& kwl,
                                             const char* prefix = 0)const;

   virtual ossimObject* createObject(const ossimString& typeName)const;

   /*!
    * Creates and object given a keyword list.
    */
   virtual ossimObject* createObject(const ossimKeywordlist& kwl,
                                     const char* prefix=0)const;
   
   /*!
    * This should return the type name of all objects in all factories.
    * This is the name used to construct the objects dynamially and this
    * name must be unique.
    */
   virtual void getTypeNameList(std::vector<ossimString>& typeList)const;

   /*!
    * METHOD: getList()
    * Returns list of all projections represented by this factory:
    */
   virtual std::list<ossimString> getList()const;

protected:
   ossimGdalProjectionFactory() {}
   ossimOgcWktTranslator theWktTranslator;
   static ossimGdalProjectionFactory*  theInstance;
};

#endif
