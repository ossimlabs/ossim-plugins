//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Mingjie Su
//
// Description: Factory for OSSIM MrSID writers.
//----------------------------------------------------------------------------
// $Id: ossimMrSidWriterFactory.h 899 2010-05-17 21:00:26Z david.burken $

#ifndef ossimMrSidWriterFactory_HEADER
#define ossimMrSidWriterFactory_HEADER

#ifdef OSSIM_ENABLE_MRSID_WRITE

#include <ossim/imaging/ossimImageWriterFactoryBase.h>

class ossimImageFileWriter;
class ossimKeywordlist;
class ossimImageWriterFactory;

/** @brief Factory for Kakadu based image writers. */
class ossimMrSidWriterFactory: public ossimImageWriterFactoryBase
{   
public:

   /** @brief virtual destructor */
   virtual ~ossimMrSidWriterFactory();

   /**
    * @brief static method to return instance (the only one) of this class.
    * @return pointer to instance of this class.
    */
   static ossimMrSidWriterFactory* instance();

   /**
    * @brief Creates a writer from extension like "sid".
    * @param fileExtension "sid"
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
    * @brief createWriter that takes a class name (ossimKakaduWriter)
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
    * @brief createObject that takes a class name (ossimKakaduWriter)
    * @param typeName Should be "ossimKakaduReader".
    * @return pointer to image writer on success, NULL on failure.
    */
   virtual ossimObject* createObject(const ossimString& typeName)const;


   /**
    * @brief Adds "ntf" to list.
    * @param extList List to add to.
    */
   virtual void getExtensions(std::vector<ossimString>& extList)const;

   /**
    * @brief Adds "ossimKakaduWriter" to list.
    * @param typeList List to add to.
    */
   virtual void getTypeNameList(std::vector<ossimString>& typeList)const;
   
   /**
    * @brief Adds "ossim_kakada_nitf_j2k" to writer list.
    * @param imageTypeList List to append to.
    */
   virtual void getImageTypeList(std::vector<ossimString>& imageTypeList)const;

   virtual void getImageFileWritersBySuffix(
      ossimImageWriterFactoryBase::ImageFileWriterList& result,
      const ossimString& ext)const;

   virtual void getImageFileWritersByMimeType(
      ossimImageWriterFactoryBase::ImageFileWriterList& result,
      const ossimString& mimeType)const;
   
protected:
   /** @brief hidden from use default constructor */
   ossimMrSidWriterFactory();

   /** @brief hidden from use copy constructor */
   ossimMrSidWriterFactory(const ossimMrSidWriterFactory&);

   /** @brief hidden from use copy constructor */
   void operator=(const ossimMrSidWriterFactory&);

   /** static instance of this class */
   static ossimMrSidWriterFactory* theInstance;

TYPE_DATA
};

#endif /* #ifdef OSSIM_ENABLE_MRSID_WRITE */

#endif /* end of #ifndef ossimMrSidWriterFactory_HEADER */
