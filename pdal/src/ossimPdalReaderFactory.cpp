//**************************************************************************************************
//
// OSSIM (http://trac.osgeo.org/ossim/)
//
// License:  LGPL -- See LICENSE.txt file in the top level directory for more details.
//
//**************************************************************************************************
// $Id$

#include "ossimPdalReaderFactory.h"
#include <ossim/base/ossimTrace.h>
#include "ossimPdalFileReader.h"
#include "ossimRialtoReader.h"

RTTI_DEF1(ossimPdalReaderFactory, "ossimPdalReaderFactory", ossimPointCloudHandlerFactory);

ossimPdalReaderFactory* ossimPdalReaderFactory::m_instance = 0;

ossimPdalReaderFactory::~ossimPdalReaderFactory()
{
   m_instance = 0;
}

ossimPdalReaderFactory* ossimPdalReaderFactory::instance()
{
   if (!m_instance)
   {
      m_instance = new ossimPdalReaderFactory;
   }
   return m_instance;
}

ossimPdalReader* ossimPdalReaderFactory::open(const ossimFilename& fileName) const
{
   ossimRefPtr<ossimPdalReader> reader = 0;

   reader = new ossimPdalFileReader;
   if (!reader->open(fileName))
   {
      reader = new ossimRialtoReader;
      if (!reader->open(fileName))
         reader = 0;
   }
   return reader.release();
}

ossimPdalReader* ossimPdalReaderFactory::open(const ossimKeywordlist& kwl,
                                                const char* prefix) const
{
   ossimRefPtr<ossimPdalReader> reader = new ossimPdalFileReader;
   if (!reader->loadState(kwl, prefix))
   {
      reader = new ossimRialtoReader;
      if (!reader->loadState(kwl, prefix))
         reader = 0;
   }

   return reader.release();
}

ossimObject* ossimPdalReaderFactory::createObject(const ossimString& typeName) const
{
   ossimRefPtr<ossimObject> result = 0;
   if (typeName == "ossimPdalFileReader")
   {
      result = new ossimPdalFileReader;
   }
   else if (typeName == "ossimPdalTileDbReader")
   {
      result = new ossimRialtoReader;
   }
   return result.release();
}

ossimObject* ossimPdalReaderFactory::createObject(const ossimKeywordlist& kwl,
                                                  const char* prefix) const
{
    return (ossimObject*) open(kwl, prefix);
}

void ossimPdalReaderFactory::getTypeNameList(std::vector<ossimString>& typeList) const
{
   typeList.push_back(ossimString("ossimPdalFileReader"));
   typeList.push_back(ossimString("ossimPdalTileDbReader"));
}

void ossimPdalReaderFactory::getSupportedExtensions(std::vector<ossimString>& extensionList) const
{
   extensionList.push_back(ossimString("las"));
   extensionList.push_back(ossimString("laz"));
   extensionList.push_back(ossimString("sqlite"));
}

ossimPdalReaderFactory::ossimPdalReaderFactory()
{
}

ossimPdalReaderFactory::ossimPdalReaderFactory(const ossimPdalReaderFactory&)
{
}

void ossimPdalReaderFactory::operator=(const ossimPdalReaderFactory&)
{
}


