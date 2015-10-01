
//*******************************************************************
// Copyright (C) 2000 ImageLinks Inc.
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Mingjie Su
//
// Description:
//
// Contains class definition for the class "ossimOgrVectorTileSource".
// ossimOgrVectorTileSource is derived from ImageHandler which is derived from
// TileSource. It is for displaying the information from OGR vector dataset
// like SDE and VPF.
//
//*******************************************************************
//  $Id: ossimOgrVectorTileSource.h 1347 2010-08-23 17:03:06Z oscar.kramer $

#ifndef ossimOgrVectorTileSource_HEADER
#define ossimOgrVectorTileSource_HEADER
#include <vector>
#include <list>
#include <map>

#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimDrect.h>

//OGR Includes
  // #include <ogrsf_frmts/ogrsf_frmts.h>
#include <ogrsf_frmts.h>
#include <gdal.h>

class ossimProjection;
class ossimMapProjection;
class ossimImageProjectionModel;
class ossimOgrVectorLayerNode;

class ossimOgrVectorTileSource :
   public ossimImageHandler
{
public:
   ossimOgrVectorTileSource();
   virtual ~ossimOgrVectorTileSource();

   virtual void close();

   /**
    *  @return Returns true on success, false on error.
    *
    *  @note This method relies on the data member ossimImageData::theImageFile
    *  being set.  Callers should do a "setFilename" prior to calling this
    *  method or use the ossimImageHandler::open that takes a file name and an
    *  entry index.
    */      
   virtual bool open();

   /*!
    *  Returns a pointer to a tile given an origin representing the upper
    *  left corner of the tile to grab from the image.
    *  Satisfies pure virtual from TileSource class.
    */
   virtual ossimRefPtr<ossimImageData> getTile(const ossimIrect& tileRect,
                                               ossim_uint32 resLevel=0);

   /*!
    *  Returns the number of bands in the image.
    *  Satisfies pure virtual from ImageHandler class.
    */
   virtual ossim_uint32 getNumberOfInputBands() const;

    /*!
    * Returns the number of bands in a tile returned from this TileSource.
    * Note: we are supporting sources that can have multiple data objects.
    * If you want to know the scalar type of an object you can pass in the 
    */
   virtual ossim_uint32 getNumberOfOutputBands() const;
  
   /*!
    *  Returns the number of bands in the image.
    *  Satisfies pure virtual from ImageHandler class.
    */
   virtual ossim_uint32 getNumberOfLines(ossim_uint32 reduced_res_level = 0) const;

   /*!
    *  Returns the number of bands available from an image.
    *  Satisfies pure virtual from ImageHandler class.
    */
   virtual ossim_uint32 getNumberOfSamples(ossim_uint32 reduced_res_level = 0) const;

   /*!
    * Returns the number of reduced resolution data sets (rrds).
    * 
    * Note:  Shape files should never have reduced res sets so this method is
    * returns "8" to avoid the question of "Do you want to build res sets".
    */
   virtual ossim_uint32 getNumberOfDecimationLevels() const;

   /*!
    * Returns the zero based image rectangle for the reduced resolution data
    * set (rrds) passed in.  Note that rrds 0 is the highest resolution rrds.
    */
   virtual ossimIrect getImageRectangle(ossim_uint32 reduced_res_level = 0) const;

   /*!
    * Method to save the state of an object to a keyword list.
    * Return true if ok or false on error.
    */
   virtual bool saveState(ossimKeywordlist& kwl,
                          const char* prefix=0)const;

   /*!
    * Method to the load (recreate) the state of an object from a keyword
    * list.  Return true if ok or false on error.
    */
   virtual bool loadState(const ossimKeywordlist& kwl,
                          const char* prefix=0);
   
   //! Returns the image geometry object associated with this tile source or NULL if non defined.
   //! The geometry contains full-to-local image transform as well as projection (image-to-world)
   virtual ossimRefPtr<ossimImageGeometry> getInternalImageGeometry() const;

   virtual ossimRefPtr<ossimImageGeometry> getImageGeometry();

   /*!
    * Returns the output pixel type of the tile source.
    */
   virtual ossimScalarType getOutputScalarType() const;

   /*!
    * Returns the width of the output tile.
    */
   virtual ossim_uint32 getTileWidth() const;
   
   /*!
    * Returns the height of the output tile.
    */
   virtual ossim_uint32 getTileHeight() const;

   /*!
    * Returns the tile width of the image or 0 if the image is not tiled.
    * Note: this is not the same as the ossimImageSource::getTileWidth which
    * returns the output tile width which can be different than the internal
    * image tile width on disk.
    */
   virtual ossim_uint32 getImageTileWidth() const;

   /*!
    * Returns the tile width of the image or 0 if the image is not tiled.
    * Note: this is not the same as the ossimImageSource::getTileWidth which
    * returns the output tile width which can be different than the internal
    * image tile width on disk.
    */
   virtual ossim_uint32 getImageTileHeight() const;   

   virtual bool isOpen()const;
   
   virtual double getNullPixelValue(ossim_uint32 band=0)const;

   virtual double getMinPixelValue(ossim_uint32 band=0)const;
      
   virtual double getMaxPixelValue(ossim_uint32 band=0)const;

   virtual bool setCurrentEntry(ossim_uint32 entryIdx);

    /**
    * @return The number of entries (images) in the image file.
    */
   virtual ossim_uint32 getNumberOfEntries()const;
   
   /**
    * @param entryList This is the list to initialize with entry indexes.
    *
    * @note This implementation returns puts one entry "0" in the list.
    */
   virtual void getEntryList(std::vector<ossim_uint32>& entryList) const;

   ossimProjection* createProjFromReference(OGRSpatialReference* reference) const;

   ossimMapProjection* createDefaultProj();

   bool isOgrVectorDataSource()const;

private:
   std::vector<ossimOgrVectorLayerNode*> theLayerVector;
   OGRDataSource*                      theDataSource;
   ossimRefPtr<ossimImageGeometry>     theImageGeometry;
   ossimDrect                          theImageBound;
   OGREnvelope                         theBoundingExtent;
TYPE_DATA
};

#endif
