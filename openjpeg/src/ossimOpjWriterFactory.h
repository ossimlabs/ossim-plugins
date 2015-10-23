//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: Factory for OSSIM Open JPEG writer.
//----------------------------------------------------------------------------
// $Id: ossimOpjWriterFactory.h 10121 2006-12-14 22:38:33Z dburken $

#ifndef ossimOpjWriterFactory_HEADER
#define ossimOpjWriterFactory_HEADER
#include <ossim/imaging/ossimImageWriterFactoryBase.h>

class ossimImageFileWriter;
class ossimKeywordlist;
class ossimImageWriterFactory;

/** @brief Factory for OpenJPEG image writer. */
class ossimOpjWriterFactory: public ossimImageWriterFactoryBase
{   
public:

   /** @brief virtual destructor */
   virtual ~ossimOpjWriterFactory();

   /**
    * @brief static method to return instance (the only one) of this class.
    * @return pointer to instance of this class.
    */
   static ossimOpjWriterFactory* instance();

   /**
    * @brief Creates a writer from extension like "jp2".
    * @param fileExtension e.g. "jpg"
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
    * @brief createWriter that takes a class name (ossimOpjWriter)
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
    * @brief createObject that takes a class name (ossimOpjWriter)
    * @param typeName Should be "ossimOpjReader".
    * @return pointer to image writer on success, NULL on failure.
    */
   virtual ossimObject* createObject(const ossimString& typeName)const;

   /**
    * @brief Adds "jp2" to list.
    * @param extList List to add to.
    */
   virtual void getExtensions(std::vector<ossimString>& extList)const;

   /**
    * @brief Adds "ossimOpjWriter" to list.
    * @param typeList List to add to.
    */
   virtual void getTypeNameList(std::vector<ossimString>& typeList)const;
   
   /**
    * @brief Adds "ossim_opj_jp2", "ossim_opj_geojp2", and "ossim_opj_gmljp2" to writer list.
    * @param imageTypeList List to append to.
    */
   virtual void getImageTypeList(std::vector<ossimString>& imageTypeList)const;

   /**
    * @brief Adds an ossimOpjJp2Writer instance to writer list.
    * @param result List to append to.
    * @param ext file extension should be either "jp2" or "j2k"
    */
   virtual void getImageFileWritersBySuffix(
      ossimImageWriterFactoryBase::ImageFileWriterList& result,
      const ossimString& ext ) const;

   virtual void getImageFileWritersByMimeType(
      ossimImageWriterFactoryBase::ImageFileWriterList& result,
      const ossimString& mimeType ) const;
   
protected:
   /** @brief hidden from use default constructor */
   ossimOpjWriterFactory();

   /** @brief hidden from use copy constructor */
   ossimOpjWriterFactory(const ossimOpjWriterFactory&);

   /** @brief hidden from use copy constructor */
   void operator=(const ossimOpjWriterFactory&);

   /** static instance of this class */
   static ossimOpjWriterFactory* theInstance;

TYPE_DATA
};

#endif /* end of #ifndef ossimOpjWriterFactory_HEADER */
