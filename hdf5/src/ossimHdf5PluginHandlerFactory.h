//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author: David Burken
//
// Copied from Mingjie Su's ossimHdfReaderFactory.
//
// Description: Factory for OSSIM HDF reader using HDF5 libraries.
//----------------------------------------------------------------------------
// $Id$

#ifndef ossimHdf5HandlerFactory_HEADER
#define ossimHdf5HandlerFactory_HEADER 1

#include <ossim/imaging/ossimImageHandlerFactoryBase.h>

class ossimString;
class ossimFilename;
class ossimKeywordlist;

/** @brief Factory for Hdf image reader. */
class ossimHdf5PluginHandlerFactory : public ossimImageHandlerFactoryBase
{
public:

   /** @brief virtual destructor */
   virtual ~ossimHdf5PluginHandlerFactory();

   /**
    * @brief static method to return instance (the only one) of this class.
    * @return pointer to instance of this class.
    */
   static ossimHdf5PluginHandlerFactory* instance();

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
    * @brief createObject that takes a class name (ossimH5Reader)
    * @param typeName Should be "ossimH5Reader".
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
    * @brief Adds ossimH5Reader to the typeList.
    * @param typeList List to add to.
    */
   virtual void getTypeNameList(std::vector<ossimString>& typeList)const;

   /**
    * @brief Method to add supported extension to the list, like "hdf".
    *
    * @param extensionList The list to add to.
    */
   virtual void getSupportedExtensions(ossimImageHandlerFactoryBase::UniqueStringList& list) const;

   /**
    * HDF5 defines different datasets depending on the sensor/format. This static method
    * provides all dataset names to the plugin intialization for populating a description.
    */
   virtual void getSupportedRenderableNames(std::vector<ossimString>& names) const;
  
protected:

   /** @brief hidden from use default constructor */
   ossimHdf5PluginHandlerFactory();

   /** @brief hidden from use copy constructor */
   ossimHdf5PluginHandlerFactory(const ossimHdf5PluginHandlerFactory&);

   /** @brief hidden from use copy constructor */
   void operator=(const ossimHdf5PluginHandlerFactory&);

   /** static instance of this class */
   static ossimHdf5PluginHandlerFactory* theInstance;

TYPE_DATA
};

#endif /* end of #ifndef ossimHdf5HandlerFactory_HEADER */

