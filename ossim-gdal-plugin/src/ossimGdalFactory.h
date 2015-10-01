//*******************************************************************
// Copyright (C) 2000 ImageLinks Inc.
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Garrett Potts
//*******************************************************************
// $Id: ossimGdalFactory.h 22229 2013-04-12 14:13:46Z dburken $

#ifndef ossimGdalFactory_HEADER
#define ossimGdalFactory_HEADER 1

#include <ossim/imaging/ossimImageHandlerFactoryBase.h>
#include <ossim/base/ossimString.h>

class ossimGdal;
class ossimFilename;
class ossimKeywordlist;

//*******************************************************************
// CLASS:  ossimGdalFactory
//*******************************************************************
class ossimGdalFactory : public ossimImageHandlerFactoryBase
{
public:
   virtual ~ossimGdalFactory();
   static ossimGdalFactory* instance();
   
   /**
    * @brief open that takes a filename.
    * @param fileName File to open.
    * @param trySuffixFirst If true calls code to try to open by suffix first,
    * then goes through the list of available handlers. default=true.
    * @param openOverview If true image handler will attempt to open overview.
    * default = true
    * @return Pointer to image handler or null if cannot open.
    */
   virtual ossimImageHandler* open(const ossimFilename& fileName,
                                   bool openOverview=true)const;
   virtual ossimImageHandler* open(const ossimKeywordlist& kwl,
                                   const char* prefix=0)const;

   /**
    * @brief Open overview that takes a file name.
    *
    * @param file File to open.
    *
    * @return ossimRefPtr to image handler on success or null on failure.
    */
   virtual ossimRefPtr<ossimImageHandler> openOverview(
      const ossimFilename& file ) const;
   
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
   virtual void getSupportedExtensions(ossimImageHandlerFactoryBase::UniqueStringList& extensionList)const;
   virtual void getImageHandlersBySuffix(ossimImageHandlerFactoryBase::ImageHandlerList& result, const ossimString& ext)const;
   virtual void getImageHandlersByMimeType(ossimImageHandlerFactoryBase::ImageHandlerList& result, const ossimString& mimeType)const;
  
protected:
   ossimGdalFactory(){}
   ossimGdalFactory(const ossimGdalFactory&){}
   void operator = (const ossimGdalFactory&){}

   static ossimGdalFactory* theInstance;

TYPE_DATA
};

#endif
