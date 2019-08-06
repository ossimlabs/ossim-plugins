//*******************************************************************
//
// License:  LGPL
//
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Garrett Potts
//
// Description:
//
// Contains class definition for the class "ossimGdalTileSource".
//
//*******************************************************************
//  $Id: ossimGdalTileSource.h 22960 2014-11-06 15:42:13Z okramer $

#ifndef ossimGdalTileSource_HEADER
#define ossimGdalTileSource_HEADER 1

#include <ossim/imaging/ossimImageHandler.h>
#include "ossim/plugin/ossimPluginConstants.h"
#include <ossim/base/ossimConstants.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimString.h>
#include <ossim/imaging/ossimImageData.h>
#include <gdal.h>
#include <vector>
#include <ossim/imaging/ossimAppFixedTileCache.h>


class ossimImageData;

class OSSIM_PLUGINS_DLL ossimGdalTileSource : public ossimImageHandler
{
public:
   ossimGdalTileSource();
   virtual ~ossimGdalTileSource();

   /**
    * @return "gdal_"+"driver_shortname", e.g. "gdal_GIF" or just gdal if not
    * initialized."
    */
   virtual ossimString getShortName()const;
   
   virtual ossimString getLongName()const;
   virtual ossimString getClassName()const;

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

   /**
    *  Returns a pointer to a tile given an origin representing the upper
    *  left corner of the tile to grab from the image.
    *  Satisfies pure virtual from TileSource class.
    */
   virtual ossimRefPtr<ossimImageData> getTile(const ossimIrect& tileRect,
                                   ossim_uint32 resLevel=0);

   /**
    *  Returns the number of bands in the image.
    *  Satisfies pure virtual from ImageHandler class.
    */
   virtual ossim_uint32 getNumberOfInputBands() const;

    /**
    * Returns the number of bands in a tile returned from this TileSource.
    * Note: we are supporting sources that can have multiple data objects.
    * If you want to know the scalar type of an object you can pass in the 
    */
   virtual ossim_uint32 getNumberOfOutputBands() const;
  
   /**
    *  Returns the number of bands in the image.
    *  Satisfies pure virtual from ImageHandler class.
    */
   virtual ossim_uint32 getNumberOfLines(ossim_uint32 reduced_res_level = 0) const;

   /**
    *  Returns the number of bands available from an image.
    *  Satisfies pure virtual from ImageHandler class.
    */
   virtual ossim_uint32 getNumberOfSamples(ossim_uint32 reduced_res_level = 0) const;

   /**
    * Returns the number of reduced resolution data sets (rrds).
    * Note:  The full res image is counted as a data set so an image with no
    *        reduced resolution data set will have a count of one.
    */
   virtual ossim_uint32 getNumberOfDecimationLevels() const;

   /**
    * Returns the zero based image rectangle for the reduced resolution data
    * set (rrds) passed in.  Note that rrds 0 is the highest resolution rrds.
    */
   virtual ossimIrect getImageRectangle(ossim_uint32 reduced_res_level = 0) const;

   /**
    * Method to save the state of an object to a keyword list.
    * Return true if ok or false on error.
    */
   virtual bool saveState(ossimKeywordlist& kwl,
                          const char* prefix=0)const;

   /**
    * Method to the load (recreate) the state of an object from a keyword
    * list.  Return true if ok or false on error.
    */
   virtual bool loadState(const ossimKeywordlist& kwl,
                          const char* prefix=0);

   /**
    * Returns the image geometry object associated with this tile source or
    * NULL if non defined.  The geometry contains full-to-local image
    * transform as well as projection (image-to-world).
    */
   virtual ossimRefPtr<ossimImageGeometry> getImageGeometry();
   
   /**
    * Returns the image geometry object associated with this tile source or NULL if non defined.
    * The geometry contains full-to-local image transform as well as projection (image-to-world)
    */
   virtual ossimRefPtr<ossimImageGeometry> getInternalImageGeometry() const;

   /**
    * Returns the output pixel type of the tile source.
    */
   virtual ossimScalarType getOutputScalarType() const;

   /**
    * @brief Gets the input scalar type of the tile source. Note this could be
    * different than the output scalar type if for instance the input is
    * lut data or complex data.
    * @return The input scalar type
    */
   ossimScalarType getInputScalarType() const;

   /**
    * Returns the width of the output tile.
    */
   virtual ossim_uint32 getTileWidth() const;
   
   /**
    * Returns the height of the output tile.
    */
   virtual ossim_uint32 getTileHeight() const;

   /**
    * Returns the tile width of the image or 0 if the image is not tiled.
    * Note: this is not the same as the ossimImageSource::getTileWidth which
    * returns the output tile width which can be different than the internal
    * image tile width on disk.
    */
   virtual ossim_uint32 getImageTileWidth() const;

   /**
    * Returns the tile width of the image or 0 if the image is not tiled.
    * Note: this is not the same as the ossimImageSource::getTileWidth which
    * returns the output tile width which can be different than the internal
    * image tile width on disk.
    */
   virtual ossim_uint32 getImageTileHeight() const;

   /**
    * @return The current entry number.
    */
   virtual ossim_uint32 getCurrentEntry()const;
   
   /**
    * @param entryIdx Entry number to select.  This will set the data member
    * "theEntryNumberToRender", then call open().
    */
   virtual bool setCurrentEntry(ossim_uint32 entryIdx);

   /**
    * @param entryList This is the list to initialize with entry indexes.
    */
   virtual void getEntryList(std::vector<ossim_uint32>& entryList)const;

   /**
    * @param entryStringList List to initialize with strings associated with
    * entries.
    */
   virtual void getEntryNames(
      std::vector<ossimString>& entryStringList) const;
   
   virtual bool isOpen()const;
   
   virtual double getNullPixelValue(ossim_uint32 band=0)const;
   virtual double getMinPixelValue(ossim_uint32 band=0)const;
   virtual double getMaxPixelValue(ossim_uint32 band=0)const;

   /**
    * @brief Set propterty method. Overrides ossimImageHandler::setProperty.
    *
    * Current property name handled:
    * "scale" One double value representing the scale in meters per pixel. It is
    * assumed the scale is same for x and y direction.
    * 
    * @param property to set.
    */
   virtual void setProperty(ossimRefPtr<ossimProperty> property);
   
   /**
    * @brief Get propterty method. Overrides ossimImageHandler::getProperty.
    * @param name Property name to get.
    */
   virtual ossimRefPtr<ossimProperty> getProperty(const ossimString& name) const;

   /**
    * @brief Get propterty names. Overrides ossimImageHandler::getPropertyNames.
    * @param propertyNames Array to initialize.
    */
   virtual void getPropertyNames(std::vector<ossimString>& propertyNames) const; 

   /**
    * @param resLevel Reduced resolution set for requested decimation.
    *
    * @param result ossimDpt to initialize with requested decimation.
    * 
    * @note Initialized "result" with the decimation factor for the passed in
    * resLevel.
    * Most of the time the returned factor is a square decimation along x
    * and y indicated by result.x and .y  = 1.0/(resLevel^2) where ^
    * means rasing to the power of.  If the resLevel is 1 then the return
    * decimation .5, .5. this is not the decimation to each resolution
    * level but the total decimation from res level 0.
    * So if resLevel is 2 then the return is .25, .25.
    *
    * @note Derived classes should override if the decimation is anything other
    * than a power of two change in each direction per res level.
    */
   virtual void getDecimationFactor(ossim_uint32 resLevel,
                                    ossimDpt& result) const;

   GDALDriverH getDriver();

   virtual bool setOutputBandList(const std::vector<ossim_uint32>& band_list);

   /**
    * @brief Sets preserve palette indexes flag.
    *
    * If true and the image is paletted, the tile returned will contain the
    * indexes instead of the LUT values. Default=false.
    * 
    * @param flag
    */
   void setPreservePaletteIndexesFlag(bool flag);

   /** @return Preserve palette flag. */
   bool getPreservePaletteIndexesFlag() const;

   /**
    * @brief Indicated data is indexed.
    *
    * Overrides ossimImageSource::isIndexedData to indicate we are passing
    * palette indexes not rgb pixel data down the stream.
    * 
    * @return Flag indicating the data contains pallete indexes.
    */
   virtual bool isIndexedData() const;
   
private:

   /**
    * @param clipRect The requested tile rectangle clipped  to the image
    * bounds.
    *
    * @param resLevel Reduced resolution level to load from.
    */
   ossimRefPtr<ossimImageData> getTileBlockRead(const ossimIrect& tileRect,
                                                ossim_uint32 resLevel);

   /**
    * Filters string from "GDALGetMetadata( theDataset, "SUBDATASETS" )
    *
    * @return Filtered string that GDALOpen will handle.
    */
   ossimString filterSubDatasetsString(const ossimString& subString) const;
   
   void computeMinMax();
   void loadIndexTo3BandTile(const ossimIrect& clipRect,
                             ossim_uint32 aGdalBandStart = 1,
                             ossim_uint32 anOssimBandStart = 0);
   template<class InputType, class OutputType>
   void loadIndexTo3BandTileTemplate(InputType in,
                                     OutputType out,
                                     const ossimIrect& clipRect,
                                     ossim_uint32 aGdalBandStart = 1,
                                     ossim_uint32 anOssimBandStart = 0);
   
   bool isIndexTo3Band(int bandNumber = 1)const;
   bool isIndexTo1Band(int bandNumber = 1)const;
   ossim_uint32 getIndexBandOutputNumber(int bandNumber)const;
   bool isIndexed(int aGdalBandNumber = 1)const;
   void getMaxSize(ossim_uint32 resLevel,
                   int& maxX,
                   int& maxY)const;
   bool isBlocked(int band)const;
   void populateLut();

   /**
    * For the given resolution level and GDAL band index, 
    * return a corresponding GDAL raster band.
    */
   GDALRasterBandH resolveRasterBand( ossim_uint32 resLevel,
                                      int gdalBandIndex ) const;

   ossimRefPtr<ossimImageGeometry> getExternalImageGeometryFromXml() const;
   void deleteRlevelCache();
   void setRlevelCache();
   /** @brief Checks prefences for default settings. */
   void getDefaults();  
   
   GDALDatasetH        theDataset;
   GDALDriverH         theDriver;

   ossimRefPtr<ossimImageData> theTile;
   ossimRefPtr<ossimImageData> theSingleBandTile;
   std::vector<ossim_uint8>    theGdalBuffer;
   ossimIrect                  theImageBound;
   mutable GDALDataType        theGdtType;
   mutable GDALDataType        theOutputGdtType;
   double*                     theMinPixValues;
   double*                     theMaxPixValues;
   double*                     theNullPixValues;
   ossim_uint32                theEntryNumberToRender;
   std::vector<ossimString>    theSubDatasets;
   bool                        theIsComplexFlag;
   bool                        theAlphaChannelFlag;
   bool                        m_preservePaletteIndexesFlag;
   std::vector<ossim_uint32>        m_outputBandList;
   bool                        m_isBlocked;

   std::vector<ossimAppFixedTileCache::ossimAppFixedCacheId> m_rlevelBlockCache;
  
TYPE_DATA
};

#endif
