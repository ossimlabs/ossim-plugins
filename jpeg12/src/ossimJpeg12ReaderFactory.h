//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: Factory for OSSIM NITF 12 bit jpeg reader using jpeg library.
//----------------------------------------------------------------------------
// $Id: ossimJpeg12ReaderFactory.h 22884 2014-09-12 13:14:35Z dburken $

#ifndef ossimJpeg12ReaderFactory_HEADER
#define ossimJpeg12ReaderFactory_HEADER 1

#include <ossim/imaging/ossimImageHandlerFactoryBase.h>

class ossimString;
class ossimFilename;
class ossimKeywordlist;

/** @brief Factory for image reader. */
class ossimJpeg12ReaderFactory : public ossimImageHandlerFactoryBase
{
public:

   /** @brief virtual destructor */
   virtual ~ossimJpeg12ReaderFactory();

   /**
    * @brief static method to return instance (the only one) of this class.
    * @return pointer to instance of this class.
    */
   static ossimJpeg12ReaderFactory* instance();

   /**
    * @brief open that takes a file name.
    * @param file The file to open.
    * @param openOverview If true image handler will attempt to open overview.
    * default = true
    * @return pointer to image handler on success, NULL on failure.
    */
   virtual ossimImageHandler* open(const ossimFilename& fileName,
                                   bool openOverview=true) const;

   /**
    * @brief Open overview that takes a file name.
    * 
    * @param file File to open.
    * 
    * @return ossimRefPtr to image handler on success or null on failure.
    */
   virtual ossimImageHandler* open(const ossimKeywordlist& kwl,
                                   const char* prefix=0)const;
   
   /**
    * @brief createObject that takes a class name (ossimJpeg12Reader)
    * @param typeName Should be "ossimJpeg12Reader".
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
    * @brief Adds ossimJpeg12Writer to the typeList.
    * @param typeList List to add to.
    */
   virtual void getTypeNameList(std::vector<ossimString>& typeList)const;

   /**
    * @brief Method to add supported extension to the list, like "jp2".
    *
    * @param extensionList The list to add to.
    */
   virtual void getSupportedExtensions(
      ossimImageHandlerFactoryBase::UniqueStringList& extensionList)const;
  
   /**
    *
    * Will add to the result list any handler that supports the passed in extensions
    *
    */
   virtual void getImageHandlersBySuffix(ossimImageHandlerFactoryBase::ImageHandlerList& result,
                                         const ossimString& ext)const;
   /**
    *
    * Will add to the result list and handler that supports the passed in mime type
    *
    */
   virtual void getImageHandlersByMimeType(ossimImageHandlerFactoryBase::ImageHandlerList& result,
                                           const ossimString& mimeType)const;
protected:

   /**
    * @brief Method to weed out extensions that this plugin knows it does
    * not support.  This is to avoid a costly open on say a tiff or jpeg that
    * is not handled by this plugin.
    *
    * @return true if extension, false if not.
    */
   bool hasExcludedExtension(const ossimFilename& file) const;
   
   /** @brief hidden from use default constructor */
   ossimJpeg12ReaderFactory();

   /** @brief hidden from use copy constructor */
   ossimJpeg12ReaderFactory(const ossimJpeg12ReaderFactory&);

   /** @brief hidden from use copy constructor */
   void operator=(const ossimJpeg12ReaderFactory&);

TYPE_DATA
};

#endif /* end of #ifndef ossimJpeg12ReaderFactory_HEADER */
