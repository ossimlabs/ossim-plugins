//**************************************************************************************************
//
// OSSIM (http://trac.osgeo.org/ossim/)
//
// License:  LGPL -- See LICENSE.txt file in the top level directory for more details.
//
//**************************************************************************************************
// $Id$

#ifndef ossimPdalReaderFactory_HEADER
#define ossimPdalReaderFactory_HEADER 1

#include <ossim/point_cloud/ossimPointCloudHandlerFactory.h>
#include <ossim/plugin/ossimPluginConstants.h>
#include "ossimPdalReader.h"

class ossimString;
class ossimFilename;
class ossimKeywordlist;

/** @brief Factory for PNG image reader. */
class OSSIM_PLUGINS_DLL ossimPdalReaderFactory: public ossimPointCloudHandlerFactory
{
public:

   /** @brief virtual destructor */
   virtual ~ossimPdalReaderFactory();

   static ossimPdalReaderFactory* instance();

   virtual ossimPdalReader* open(const ossimFilename& fileName) const;

   virtual ossimPdalReader* open(const ossimKeywordlist& kwl, const char* prefix = 0) const;

   virtual ossimObject* createObject(const ossimString& typeName) const;

   virtual ossimObject* createObject(const ossimKeywordlist& kwl, const char* prefix = 0) const;

   virtual void getTypeNameList(std::vector<ossimString>& typeList) const;

   virtual void getSupportedExtensions(std::vector<ossimString>& extList) const;

protected:
   /** @brief hidden from use default constructor */
   ossimPdalReaderFactory();

   /** @brief hidden from use copy constructor */
   ossimPdalReaderFactory(const ossimPdalReaderFactory&);

   /** @brief hidden from use copy constructor */
   void operator=(const ossimPdalReaderFactory&);

   /** static instance of this class */
   static ossimPdalReaderFactory* m_instance;

TYPE_DATA
};

#endif /* end of #ifndef ossimPdalReaderFactory_HEADER */
