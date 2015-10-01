//*******************************************************************
// Copyright (C) 2000 ImageLinks Inc.
//
// License:  LGPL
//
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Garrett Potts
//
//*******************************************************************
//  $Id: ossimGdalImageWriterFactory.h 18003 2010-08-30 18:02:52Z gpotts $

#ifndef ossimGdalImageWriterFactory_HEADER
#define ossimGdalImageWriterFactory_HEADER
#include <ossim/imaging/ossimImageWriterFactoryBase.h>
#include <gdal.h>
class ossimImageFileWriter;
class ossimKeywordlist;
class ossimImageWriterFactory;

class ossimGdalImageWriterFactory: public ossimImageWriterFactoryBase
{   
public:
   virtual ~ossimGdalImageWriterFactory();
   static ossimGdalImageWriterFactory* instance();
   virtual ossimImageFileWriter* createWriter(const ossimKeywordlist& kwl,
                                              const char *prefix=0)const;
   virtual ossimImageFileWriter* createWriter(const ossimString& typeName)const;
   
   virtual ossimObject* createObject(const ossimKeywordlist& kwl,
                                     const char *prefix=0)const;
   virtual ossimObject* createObject(const ossimString& typeName)const;
   
   virtual void getExtensions(std::vector<ossimString>& result)const;
   
   virtual void getTypeNameList(std::vector<ossimString>& typeList)const;
   
   /**
    * void getImageTypeList(std::vector<ossimString>& imageTypeList)const
    *
    * Appends this writer image types to list "imageTypeList".
    *
    * This writer can have the following types dependent upon how the gdal
    * library was compiled:
    * gdal_imagine_hfa
    * gdal_nitf_rgb_band_separate
    * gdal_jpeg2000
    * gdal_arc_info_aig
    * gdal_arc_info_gio
    * gdal_arc_info_ascii_grid
    *
    * @param imageTypeList stl::vector<ossimString> list to append to.
    */
   virtual void getImageTypeList(std::vector<ossimString>& imageTypeList)const;
   
   virtual void getImageFileWritersBySuffix(ossimImageWriterFactoryBase::ImageFileWriterList& result,
                                            const ossimString& ext)const;
   virtual void getImageFileWritersByMimeType(ossimImageWriterFactoryBase::ImageFileWriterList& result,
                                              const ossimString& mimeType)const;
protected:
   ossimGdalImageWriterFactory() {theInstance = this;}

   static ossimGdalImageWriterFactory* theInstance;
   
   ossimString convertToDriverName(const ossimString& imageTypeName)const;
   bool canWrite(GDALDatasetH handle)const;
};

#endif
