//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: Factory for OSSIM Open JPEG reader.
//----------------------------------------------------------------------------
// $Id: ossimOpjReaderFactory.h 10110 2006-12-14 18:20:54Z dburken $
#ifndef ossimOpjReaderFactory_HEADER
#define ossimOpjReaderFactory_HEADER 1

#include <ossim/imaging/ossimImageHandlerFactoryBase.h>

class ossimString;
class ossimFilename;
class ossimKeywordlist;

/** @brief Factory for PNG image reader. */
class ossimOpjReaderFactory : public ossimImageHandlerFactoryBase
{
public:

   /** @brief virtual destructor */
   virtual ~ossimOpjReaderFactory();

   /**
    * @brief static method to return instance (the only one) of this class.
    * @return pointer to instance of this class.
    */
   static ossimOpjReaderFactory* instance();

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
    * @brief open that takes a keyword list and prefix.
    * @param kwl The keyword list.
    * @param prefix the keyword list prefix.
    * @return pointer to image handler on success, NULL on failure.
    */
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

   /**
    * @brief createObject that takes a class name (ossimOpjReader)
    * @param typeName Should be "ossimOpjReader".
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
    * @brief Adds ossimOpjWriter to the typeList.
    * @param typeList List to add to.
    */
   virtual void getTypeNameList(std::vector<ossimString>& typeList)const;

   /**
    * @brief Method to add supported extension to the list, like "png".
    *
    * @param extensionList The list to add to.
    */
   virtual void getSupportedExtensions(
      ossimImageHandlerFactoryBase::UniqueStringList& extensionList)const;

   /**
    * @brief Will add to the result list any handler that supports the passed in
    * extensions.
    */
   virtual void getImageHandlersBySuffix(
      ossimImageHandlerFactoryBase::ImageHandlerList& result,
      const ossimString& ext) const;
   
   /**
    * @brief Will add to the result list and handler that supports the passed
    * in mime type.
    */
   virtual void getImageHandlersByMimeType(
      ossimImageHandlerFactoryBase::ImageHandlerList& result,
      const ossimString& mimeType) const;
  
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
   ossimOpjReaderFactory();

   /** @brief hidden from use copy constructor */
   ossimOpjReaderFactory(const ossimOpjReaderFactory&);

   /** @brief hidden from use copy constructor */
   void operator=(const ossimOpjReaderFactory&);

   /** static instance of this class */
   static ossimOpjReaderFactory* theInstance;

TYPE_DATA
};

#endif /* end of #ifndef ossimOpjReaderFactory_HEADER */
