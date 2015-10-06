//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: Factory for OSSIM SQLite reader factory.
//----------------------------------------------------------------------------
// $Id$

#ifndef ossimSqliteReaderFactory_HEADER
#define ossimSqliteReaderFactory_HEADER 1

#include <ossim/imaging/ossimImageHandlerFactoryBase.h>

class ossimString;
class ossimFilename;
class ossimKeywordlist;

/** @brief Factory for SQLITE image reader. */
class ossimSqliteReaderFactory : public ossimImageHandlerFactoryBase
{
public:

   /** @brief virtual destructor */
   virtual ~ossimSqliteReaderFactory();

   /**
    * @brief static method to return instance (the only one) of this class.
    * @return pointer to instance of this class.
    */
   static ossimSqliteReaderFactory* instance();

   /**
    * @brief open that takes a file name.
    * @param file The file to open.
    * @param openOverview If true image handler will attempt to open overview.
    * default = true 
    * @return pointer to image handler on success, NULL on failure.
    */
   virtual ossimImageHandler* open(const ossimFilename& file,
                                   bool openOverview=true) const;

   /**
    * @brief open that takes a keyword list and prefix.
    * @param kwl The keyword list.
    * @param prefix the keyword list prefix.
    * @return pointer to image handler on success, NULL on failure.
    */
   virtual ossimImageHandler* open(const ossimKeywordlist& kwl,
                                   const char* prefix=0)const;

   /**
    * @brief createObject that takes a class name (ossimSqliteReader)
    * @param typeName Should be "ossimSqliteReader".
    * @return pointer to image writer on success, NULL on failure.
    */
   virtual ossimObject* createObject(const ossimString& typeName)const;
   
   /**
    * @brief Creates and object given a keyword list and prefix.
    * @param kwl The keyword list.
    * @param prefix the keyword list prefix.
    * @return pointer to image handler on success, NULL on failure.
    */
   virtual ossimObject* createObject(const ossimKeywordlist& kwl,
                                     const char* prefix=0)const;
   
   /**
    * @brief Adds ossimSqliteWriter to the typeList.
    * @param typeList List to add to.
    */
   virtual void getTypeNameList(std::vector<ossimString>& typeList)const;

   /**
    * @brief Method to add supported extension to the list, like "sqlite".
    *
    * @param extensionList The list to add to.
    */
   virtual void getSupportedExtensions(
      ossimImageHandlerFactoryBase::UniqueStringList& extensionList)const;
  
   virtual void getImageHandlersBySuffix(ImageHandlerList& result,
                                         const ossimString& ext)const;
   virtual void getImageHandlersByMimeType(ImageHandlerList& result,
                                           const ossimString& mimeType)const;
protected:
   /** @brief hidden from use default constructor */
   ossimSqliteReaderFactory();

   /** @brief hidden from use copy constructor */
   ossimSqliteReaderFactory(const ossimSqliteReaderFactory&);

   /** @brief hidden from use copy constructor */
   void operator=(const ossimSqliteReaderFactory&);

   /** static instance of this class */
   static ossimSqliteReaderFactory* theInstance;

TYPE_DATA
};

#endif /* end of #ifndef ossimSqliteReaderFactory_HEADER */
