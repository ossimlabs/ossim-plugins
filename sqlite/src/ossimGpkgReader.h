//----------------------------------------------------------------------------
//
// File: ossimGpkgReader.h
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: OSSIM GeoPackage reader (tile source).
//
//----------------------------------------------------------------------------
// $Id$
#ifndef ossimGpkgReader_HEADER
#define ossimGpkgReader_HEADER 1

#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/base/ossimRefPtr.h>
#include "ossimGpkgTileEntry.h"
#include <ossim/imaging/ossimCodecBase.h>

#include <vector>

class ossimGpkgTileRecord;
class ossimImageData;
struct sqlite3;

class ossimGpkgReader : public ossimImageHandler
{
public:

   /** default constructor */
   ossimGpkgReader();

   /** virtual destructor */
   virtual ~ossimGpkgReader();

   /** @return "gpkg" */
   virtual ossimString getShortName() const;

   /** @return "ossim gpkg" */
   virtual ossimString getLongName()  const;

   /** @return "ossimGpkgReader" */
   virtual ossimString getClassName()    const;

   /**
    *  Returns a pointer to a tile given an origin representing the upper
    *  left corner of the tile to grab from the image.
    *  Satisfies pure virtual from TileSource class.
    */
   virtual ossimRefPtr<ossimImageData> getTile(const  ossimIrect& rect,
                                               ossim_uint32 resLevel=0);
   /**
    * Method to get a tile.   
    *
    * @param result The tile to stuff.  Note The requested rectangle in full
    * image space and bands should be set in the result tile prior to
    * passing.  It will be an error if:
    * result.getNumberOfBands() != this->getNumberOfOutputBands()
    *
    * @return true on success false on error.  If return is false, result
    *  is undefined so caller should handle appropriately with makeBlank or
    * whatever.
    */
   virtual bool getTile(ossimImageData* result, ossim_uint32 resLevel=0);

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
   virtual ossim_uint32 getNumberOfOutputBands()const;

   /**
     *  Returns the number of lines in the image.
     *  Satisfies pure virtual from ImageHandler class.
     */
   virtual ossim_uint32 getNumberOfLines(ossim_uint32 resLevel = 0) const;

   /**
    *  Returns the number of samples in the image.
    *  Satisfies pure virtual from ImageHandler class.
    */
   virtual ossim_uint32 getNumberOfSamples(ossim_uint32 resLevel = 0) const;

   /**
    * Returns the zero based image rectangle for the reduced resolution data
    * set (rrds) passed in.  Note that rrds 0 is the highest resolution rrds.
    */
   virtual ossimIrect getImageRectangle(ossim_uint32 resLevel = 0) const;

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
    * Returns the output pixel type of the tile source.
    */
   virtual ossimScalarType getOutputScalarType() const;

   /**
    * Returns the width of the output tile.
    */
   virtual ossim_uint32    getTileWidth() const;

   /**
    * Returns the height of the output tile.
    */
   virtual ossim_uint32    getTileHeight() const;

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

   bool isOpen()const;

   /** Close method. */
   virtual void close();
   
   /** @return The number of entries (images) in the image file. */
   virtual ossim_uint32 getNumberOfEntries() const;

   /** @return The current entry number. */
   virtual ossim_uint32 getCurrentEntry()const;

   /**
    * @brief Gets the entry list.
    * @param entryList This is the list to initialize with entry indexes.
    */
   virtual void getEntryList(std::vector<ossim_uint32>& entryList) const;
   
   /**
    * @brief Get the list of entry names.
    *
    * This implementation uses the entry tile table name.
    * 
    * @param getEntryNames List to initialize with strings associated with
    * entries.
    */
   virtual void getEntryNames(std::vector<ossimString>& entryNames) const;
   
   /**
    * @param entryIdx Entry number to select.
    *
    * @note The implementation does nothing.  Derived classes that handle
    * multiple images should override.
    *
    * @return true if it was able to set the current entry and false otherwise.
    */
   virtual bool setCurrentEntry(ossim_uint32 entryIdx);

   /**
    * @brief Returns the number of decimation levels.
    * 
    * This returns the total number of decimation levels.  It is important to
    * note that res level 0 or full resolution is included in the list and has
    * decimation values 1.0, 1.0
    * 
    * @return The number of decimation levels.
    */
   virtual ossim_uint32 getNumberOfDecimationLevels()const;

   /**
    * @return Returns the image geometry object associated with this tile
    * source or NULL if non defined.  The geometry contains full-to-local image
    * transform as well as projection (image-to-world).
    */
   virtual ossimRefPtr<ossimImageGeometry> getImageGeometry();
   
   /**
    * @param Method to get geometry from the embedded JP2 GeoTIFF Box.
    */
   virtual ossimRefPtr<ossimImageGeometry> getInternalImageGeometry();
   
protected:

   /**
    *  @brief open method.
    *  @return true on success, false on error.
    */
   virtual bool open();

private:

   bool initImageParams();

   /** @brief Allocates m_tile.  Called on first getTile. */
   void allocate();

   /**
    * @note this method assumes that setImageRectangle has been called on
    * theTile.
    */
   void fillTile( ossim_uint32 resLevel,
                  const ossimIrect& tileRect,
                  const ossimIrect& clipRect,
                  ossimImageData* tile );

   ossimRefPtr<ossimImageData> getTile( ossim_uint32 resLevel,
                                        ossimIpt index );

   /**
    * @brief Uncompresses png tile to m_cacheTile.
    * @param tile Tile record.
    * @param tileSize Size of tile.
    */
   ossimRefPtr<ossimImageData> uncompressPngTile( const ossimGpkgTileRecord& tile,
                                                  const ossimIpt& tileSize );

   void getTileIndexes( ossim_uint32 resLevel,
                        const ossimIrect& clipRect,
                        std::vector<ossimIpt>& tileIndexes ) const;

   void getTileSize( ossim_uint32 resLevel, ossimIpt& tileSize ) const;

   /** @return Number of internal zoom levels. */
   ossim_uint32 getNumberOfZoomLevels() const;

   void computeGsd( ossimDpt& gsd) const;

   ossimRefPtr<ossimImageHandler> m_ih;
   
   ossimRefPtr<ossimImageData> m_tile;
   ossimRefPtr<ossimImageData> m_cacheTile;
   // std::ifstream               m_str;
   sqlite3*                    m_db;
   ossim_uint32                m_currentEntry;
   ossim_uint32                m_bands;
   ossimScalarType             m_scalar;
   ossim_uint32                m_tileWidth;
   ossim_uint32                m_tileHeight;
   
   std::vector<ossimGpkgTileEntry> m_entries;

   mutable ossimRefPtr<ossimCodecBase> m_jpegCodec;
   mutable ossimRefPtr<ossimCodecBase> m_pngCodec;

TYPE_DATA
};

#endif 
