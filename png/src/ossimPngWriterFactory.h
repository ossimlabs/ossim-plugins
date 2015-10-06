//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: Factory for OSSIM Portable Network Graphics (PNG) writer.
//----------------------------------------------------------------------------
// $Id: ossimPngWriterFactory.h 18003 2010-08-30 18:02:52Z gpotts $

#ifndef ossimPngWriterFactory_HEADER
#define ossimPngWriterFactory_HEADER
#include <ossim/imaging/ossimImageWriterFactoryBase.h>

class ossimImageFileWriter;
class ossimKeywordlist;
class ossimImageWriterFactory;

/** @brief Factory for PNG image writer. */
class ossimPngWriterFactory: public ossimImageWriterFactoryBase
{   
public:

   /** @brief virtual destructor */
   virtual ~ossimPngWriterFactory();

   /**
    * @brief static method to return instance (the only one) of this class.
    * @return pointer to instance of this class.
    */
   static ossimPngWriterFactory* instance();

   /**
    * @brief Creates a writer from extension like "png".
    * @param fileExtension "png"
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
    * @brief createWriter that takes a class name (ossimPngWriter)
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
    * @brief createObject that takes a class name (ossimPngWriter)
    * @param typeName Should be "ossimPngReader".
    * @return pointer to image writer on success, NULL on failure.
    */
   virtual ossimObject* createObject(const ossimString& typeName)const;


   /**
    * @brief Adds "png" to list.
    * @param extList List to add to.
    */
   virtual void getExtensions(std::vector<ossimString>& extList)const;

   /**
    * @brief Adds "ossimPngWriter" to list.
    * @param typeList List to add to.
    */
   virtual void getTypeNameList(std::vector<ossimString>& typeList)const;
   
   /**
    * @brief Adds "ossim_png" to writer list.
    * @param imageTypeList List to append to.
    */
   virtual void getImageTypeList(std::vector<ossimString>& imageTypeList)const;
   
   virtual void getImageFileWritersBySuffix(ossimImageWriterFactoryBase::ImageFileWriterList& result,
                                            const ossimString& ext)const;
   virtual void getImageFileWritersByMimeType(ossimImageWriterFactoryBase::ImageFileWriterList& result,
                                              const ossimString& mimeType)const;
protected:
   /** @brief hidden from use default constructor */
   ossimPngWriterFactory();

   /** @brief hidden from use copy constructor */
   ossimPngWriterFactory(const ossimPngWriterFactory&);

   /** @brief hidden from use copy constructor */
   void operator=(const ossimPngWriterFactory&);

   /** static instance of this class */
   static ossimPngWriterFactory* theInstance;

TYPE_DATA
};

#endif /* end of #ifndef ossimPngWriterFactory_HEADER */
