//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  Mingjie Su, Harsh Govind
//
// Description: Factory for ossim KmlSuperOverlay reader using libkml library.
//----------------------------------------------------------------------------
// $Id: ossimKmlSuperOverlayReaderFactory.h 2472 2011-04-26 15:47:50Z ming.su $

#ifndef ossimKmlSuperOverlayReaderFactory_HEADER
#define ossimKmlSuperOverlayReaderFactory_HEADER

#include <ossim/imaging/ossimImageHandlerFactoryBase.h>

class ossimString;
class ossimFilename;
class ossimKeywordlist;

/** @brief Factory for Kml SuperOverlay reader. */
class ossimKmlSuperOverlayReaderFactory : public ossimImageHandlerFactoryBase
{
public:

   /** @brief virtual destructor */
   virtual ~ossimKmlSuperOverlayReaderFactory();

   /**
    * @brief static method to return instance (the only one) of this class.
    * @return pointer to instance of this class.
    */
   static ossimKmlSuperOverlayReaderFactory* instance();

   /**
    * @brief open that takes a file name.
    * @param file The file to open.
    * @return pointer to image handler on success, NULL on failure.
    */
   virtual ossimImageHandler* open(const ossimFilename& fileName, bool openOverview=true) const;

   /**
    * @brief open that takes a keyword list and prefix.
    * @param kwl The keyword list.
    * @param prefix the keyword list prefix.
    * @return pointer to image handler on success, NULL on failure.
    */
   virtual ossimImageHandler* open(const ossimKeywordlist& kwl,
                                   const char* prefix=0)const;

   /**
    * @brief createObject that takes a class name (ossimKmlSuperOverlayReader)
    * @param typeName Should be "ossimKmlSuperOverlayReader".
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
    * @brief Adds ossimKmlSuperOverlayReader to the typeList.
    * @param typeList List to add to.
    */
   virtual void getTypeNameList(std::vector<ossimString>& typeList)const;

   /**
    * @brief Method to add supported extension to the list, like "kml" or "kmz".
    *
    * @param extensionList The list to add to.
    */
   virtual void getSupportedExtensions(
      ossimImageHandlerFactoryBase::UniqueStringList& extensionList)const;
  
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
   ossimKmlSuperOverlayReaderFactory();

   /** @brief hidden from use copy constructor */
   ossimKmlSuperOverlayReaderFactory(const ossimKmlSuperOverlayReaderFactory&);

   /** @brief hidden from use copy constructor */
   void operator=(const ossimKmlSuperOverlayReaderFactory&);

   /** static instance of this class */
   static ossimKmlSuperOverlayReaderFactory* theInstance;

TYPE_DATA
};

#endif /* end of #ifndef ossimKmlSuperOverlayReaderFactory_HEADER */
