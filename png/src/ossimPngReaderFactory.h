//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: Factory for OSSIM Portable Network Graphics plugin (PNG)
// reader.
//----------------------------------------------------------------------------
// $Id: ossimPngReaderFactory.h 22633 2014-02-20 00:57:42Z dburken $
#ifndef ossimPngReaderFactory_HEADER
#define ossimPngReaderFactory_HEADER 1

#include <ossim/imaging/ossimImageHandlerFactoryBase.h>

class ossimString;
class ossimFilename;
class ossimKeywordlist;

/** @brief Factory for PNG image reader. */
class ossimPngReaderFactory : public ossimImageHandlerFactoryBase
{
public:

   /** @brief virtual destructor */
   virtual ~ossimPngReaderFactory();

   /**
    * @brief static method to return instance (the only one) of this class.
    * @return pointer to instance of this class.
    */
   static ossimPngReaderFactory* instance();

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
    *  @brief Open method.
    *
    *  This open takes a stream, position and a flag.
    *
    *  @param str Open stream to image.
    *
    *  @param restartPosition Typically 0, this is the stream offset to the
    *  front of the image.
    *
    *  @param youOwnIt If true the opener takes owner ship of the stream
    *  pointer and will destroy on close.
    *  
    *  @return This implementation returns an ossimRefPtr with a null pointer.
    */
   virtual ossimRefPtr<ossimImageHandler> open( std::istream* str,
                                                std::streamoff restartPosition,
                                                bool youOwnit ) const;   

   /**
    * @brief createObject that takes a class name (ossimPngReader)
    * @param typeName Should be "ossimPngReader".
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
    * @brief Adds ossimPngWriter to the typeList.
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
  
   virtual void getImageHandlersBySuffix(ImageHandlerList& result,
                                         const ossimString& ext)const;
   virtual void getImageHandlersByMimeType(ImageHandlerList& result,
                                           const ossimString& mimeType)const;
protected:
   /** @brief hidden from use default constructor */
   ossimPngReaderFactory();

   /** @brief hidden from use copy constructor */
   ossimPngReaderFactory(const ossimPngReaderFactory&);

   /** @brief hidden from use copy constructor */
   void operator=(const ossimPngReaderFactory&);

   /** static instance of this class */
   static ossimPngReaderFactory* theInstance;

TYPE_DATA
};

#endif /* end of #ifndef ossimPngReaderFactory_HEADER */
