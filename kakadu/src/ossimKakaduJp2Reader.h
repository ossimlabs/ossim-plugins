//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description:  Class declaration for JPEG2000 JP2 reader.
//
// Specification: ISO/IEC 15444
//
//----------------------------------------------------------------------------
// $Id: ossimKakaduJp2Reader.h 22884 2014-09-12 13:14:35Z dburken $

#ifndef ossimKakaduJp2Reader_HEADER
#define ossimKakaduJp2Reader_HEADER 1


#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/imaging/ossimAppFixedTileCache.h>
#include <ossim/imaging/ossimImageData.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/support_data/ossimJ2kSizRecord.h>

#include <kdu_compressed.h>

#include <vector>

// Forward declarations:

class ossimKeywordlist;

class kdu_tile;

namespace kdu_core
{
   class kdu_compressed_source;
   class kdu_line_buf;
   class kdu_thread_queue;
}

namespace kdu_supp
{
   struct kdu_channel_mapping;
   class jp2_family_src;
}

/**
 * @brief ossimKakaduJp2Reader class for reading images with JPEG2000
 * (J2K) compressed blocks using kakadu library for decompression. 
 */
class OSSIM_PLUGINS_DLL ossimKakaduJp2Reader : public ossimImageHandler
{
public:

   /** default construtor */
   ossimKakaduJp2Reader();
   
   /** virtural destructor */
   virtual ~ossimKakaduJp2Reader();

   /**
    * @brief Returns short name.
    * @return "ossim_kakadu_jp2_reader"
    */
   virtual ossimString getShortName() const;
   
   /**
    * @brief Returns long  name.
    * @return "ossim kakadu jp2 reader"
    */
   virtual ossimString getLongName()  const;

   /**
    * @brief Returns class name.
    * @return "ossimKakaduJp2Reader"
    */
   virtual ossimString getClassName() const;

   /**
    *  @brief Method to grab a tile(rectangle) from image.
    *
    *  @param rect The zero based rectangle to grab.
    *
    *  @param resLevel The reduced resolution level to grab from.
    *
    *  @return The ref pointer with the image data pointer.
    */
   virtual ossimRefPtr<ossimImageData> getTile(const  ossimIrect& rect,
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
   virtual ossim_uint32 getNumberOfOutputBands()const;
   
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
    * Returns the output pixel type of the tile source.
    */
   virtual ossimScalarType getOutputScalarType() const;
   
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
    * @brief Gets number of lines for res level.
    *
    * Overrides ossimJ2kTileSource::getNumberOfLines
    *
    *  @param resLevel Reduced resolution level to return lines of.
    *  Default = 0
    *
    *  @return The number of lines for specified reduced resolution level.
    */
   virtual ossim_uint32 getNumberOfLines(ossim_uint32 resLevel = 0) const;

   /**
    *  @brief Gets the number of samples for res level.
    *
    *  Overrides ossimJ2kTileSource::getNumberOfSamples
    *
    *  @param resLevel Reduced resolution level to return samples of.
    *  Default = 0
    *
    *  @return The number of samples for specified reduced resolution level.
    */
   virtual ossim_uint32 getNumberOfSamples(ossim_uint32 resLevel = 0) const;

   /**
    * @brief Open method.
    * @return true on success, false on error.
    */
   virtual bool open();

   /**
    *  @brief Method to test for open file stream.
    *
    *  @return true if open, false if not.
    */
   virtual bool isOpen()const;

   /**
    * @brief Method to close current entry.
    *
    * @note There is a bool kdu_compressed_source::close() and a
    * void ossimImageHandler::close(); hence, a new close to avoid conflicting
    * return types.
    */
   virtual void closeEntry();

   /** Anonymous enumerations: */
   enum
   {
      SIGNATURE_BOX_SIZE = 12,
      GEOTIFF_UUID_SIZE  = 16
   };

   /**
    * Returns the image geometry object associated with this tile source or
    * NULL if non defined.  The geometry contains full-to-local image
    * transform as well as projection (image-to-world).
    */
   virtual ossimRefPtr<ossimImageGeometry> getImageGeometry();

   /**
    * @param Method to get geometry from the embedded JP2 GeoTIFF Box.
    */
   virtual ossimRefPtr<ossimImageGeometry> getInternalImageGeometry();

   /**
    * Method to the load (recreate) the state of an object from a keyword
    * list.  Return true if ok or false on error.
    */
   virtual bool loadState(const ossimKeywordlist& kwl, const char* prefix=0);   
   
protected:

   /**
   * @brief Gets an overview tile.
   * 
   * Overrides ossimImageHandler::getOverviewTile
   *
   * @param resLevel The resolution level to pull from with resLevel 0 being
   * full res.
   * 
   * @param result The tile to stuff.  Note The requested rectangle in full
   * image space and bands should be set in the result tile prior to
   * passing.  This method will subtract the subImageOffset if needed for
   * external overview call since they do not know about the sub image offset.
   *
   * @return true on success false on error.  Typically this will return false
   * if resLevel==0 unless the overview has r0.  If return is false, result
   * is undefined so caller should handle appropriately with makeBlank or
   * whatever.
   */
   virtual bool getOverviewTile(ossim_uint32 resLevel,
                                ossimImageData* result);   
   
private:

   /**
    * @param Method to get geometry from the various external files like
    * .prj, .j2w .xml and so on.
    */
   ossimRefPtr<ossimImageGeometry> getMetadataImageGeometry() const;

   /**
    * @brief Test first 12 bytes of file for the jp2 signature block.
    * @return true is theImageFile is a jp2, false if not.
    */
   bool isJp2();

   /**
    * @brief Opens the jp2 and initializes this object.
    * @return true on success, false on error.
    */
   bool openJp2File();
   
   /** @brief Initializes data member "theTile". */
   void initializeTile();

   /**
    * Loads a block of data to theCacheTile from the cache.
    * 
    * @param origin This is the point used for the cache query.
    *
    * @param clipRect The rectangle to fill.
    *
    * @return true on success, false on error.
    */
   bool loadTileFromCache(const ossimIpt& origin,
                          const ossimIrect& clipRect);
   
   /**
    * Loads a block of data to theCacheTile.
    * 
    * @param origin Starting position of block to load.
    *
    * @return true on success, false on error.
    */
   bool loadTile(const ossimIpt& origin);

   /**
    * Initializes m_channels to be used with copyRegionToTile if it makes sense.
    */
   void configureChannelMapping();

   kdu_supp::jp2_family_src*        theJp2FamilySrc;
   kdu_core::kdu_compressed_source* theJp2Source;
   kdu_supp::kdu_channel_mapping*   theChannels;
   kdu_core::kdu_codestream         theCodestream;
   kdu_core::kdu_thread_env*        theThreadEnv;
   kdu_core::kdu_thread_queue*      theOpenTileThreadQueue;
   
   ossim_uint32                theMinDwtLevels;
   ossim_uint32                theNumberOfBands;
   ossimIpt                    theCacheSize;
   ossimScalarType             theScalarType;
   ossimIrect                  theImageRect; /** Has sub image offset. */

   /** Image dimensions for each level. */
   std::vector<ossimIrect>      theJp2Dims;

   /** Tile dimensions for each level. */
   std::vector<ossimIrect>      theJp2TileDims;

   ossimRefPtr<ossimImageData>  theTile;
   ossimRefPtr<ossimImageData>  theCacheTile;

   /** Cache initialized to image rect with sub image offset */
   ossimAppFixedTileCache::ossimAppFixedCacheId theCacheId;
   
TYPE_DATA   
};

#endif /* #ifndef ossimKakaduJp2Reader_HEADER */
