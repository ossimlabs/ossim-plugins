//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: Factory for OSSIM SQLite writer factory.
//----------------------------------------------------------------------------
// $Id$ 

#ifndef ossimSqliteWriterFactory_HEADER
#define ossimSqliteWriterFactory_HEADER 1
#include <ossim/imaging/ossimImageWriterFactoryBase.h>

class ossimImageFileWriter;
class ossimKeywordlist;
class ossimImageWriterFactory;

/** @brief Factory for SQLITE image writer. */
class ossimSqliteWriterFactory: public ossimImageWriterFactoryBase
{   
public:

   /** @brief virtual destructor */
   virtual ~ossimSqliteWriterFactory();

   /**
    * @brief static method to return instance (the only one) of this class.
    * @return pointer to instance of this class.
    */
   static ossimSqliteWriterFactory* instance();

   /**
    * @brief Creates a writer from extension like "sqlite".
    * @param fileExtension "sqlite"
    */
   virtual ossimImageFileWriter *createWriterFromExtension(
      const ossimString& fileExtension)const;

   /**
    * @brief Create that takes a keyword list and prefix.
    * @param kwl The keyword list.
    * @param prefix the keyword list prefix.
    * @return pointer to image handler on success, NULL on failure.
    */
   virtual ossimImageFileWriter* createWriter(const ossimKeywordlist& kwl,
                                              const char *prefix=0)const;

   /**
    * @brief createWriter that takes a class name (ossimSqliteWriter)
    * @param kwl The keyword list.
    * @param prefix the keyword list prefix.
    * @return pointer to image writer on success, NULL on failure.
    */
   virtual ossimImageFileWriter* createWriter(const ossimString& typeName)const;

   /**
    * @brief Creates and object given a keyword list.
    * @param kwl The keyword list.
    * @param prefix the keyword list prefix.
    * @return pointer to image writer on success, NULL on failure.
    */
   virtual ossimObject* createObject(const ossimKeywordlist& kwl,
                                     const char *prefix=0)const;

   /**
    * @brief createObject that takes a class name (ossimSqliteWriter)
    * @param typeName Should be "ossimSqliteReader".
    * @return pointer to image writer on success, NULL on failure.
    */
   virtual ossimObject* createObject(const ossimString& typeName)const;


   /**
    * @brief Adds "sqlite" to list.
    * @param extList List to add to.
    */
   virtual void getExtensions(std::vector<ossimString>& extList)const;

   /**
    * @brief Adds "ossimSqliteWriter" to list.
    * @param typeList List to add to.
    */
   virtual void getTypeNameList(std::vector<ossimString>& typeList)const;
   
   /**
    * @brief Adds "ossim_sqlite" to writer list.
    * @param imageTypeList List to append to.
    */
   virtual void getImageTypeList(std::vector<ossimString>& imageTypeList)const;
   
   virtual void getImageFileWritersBySuffix(ossimImageWriterFactoryBase::ImageFileWriterList& result,
                                            const ossimString& ext)const;
   virtual void getImageFileWritersByMimeType(ossimImageWriterFactoryBase::ImageFileWriterList& result,
                                              const ossimString& mimeType)const;
protected:
   /** @brief hidden from use default constructor */
   ossimSqliteWriterFactory();

   /** @brief hidden from use copy constructor */
   ossimSqliteWriterFactory(const ossimSqliteWriterFactory&);

   /** @brief hidden from use copy constructor */
   void operator=(const ossimSqliteWriterFactory&);

   /** static instance of this class */
   static ossimSqliteWriterFactory* theInstance;

TYPE_DATA
};

#endif /* end of #ifndef ossimSqliteWriterFactory_HEADER */
