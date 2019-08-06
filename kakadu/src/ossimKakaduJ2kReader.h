//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description:
//
// Class declaration for JPEG2000 (J2K) reader.
//
//----------------------------------------------------------------------------
// $Id: ossimKakaduJ2kReader.h 22884 2014-09-12 13:14:35Z dburken $

#ifndef ossimKakaduJ2kReader_HEADER
#define ossimKakaduJ2kReader_HEADER 1

#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/support_data/ossimJ2kSizRecord.h>
#include <ossim/imaging/ossimAppFixedTileCache.h>
#include <iosfwd>
#include <fstream>

// Forward class declarations.
class ossimImageData;

//---
// Kakadu put multiple classes in header:  kdu_compressed_source, kd_thread_env
//---
#include <kdu_compressed.h>
// #include <kdu_region_decompressor.h>


namespace kdu_core
{
   class kdu_line_buf;
   class kdu_thread_queue;
}

/**
 * @brief ossimKakaduJ2kReader class for reading images with JPEG2000
 * (J2K) compressed blocks using kakadu library for decompression. 
 */
class OSSIM_PLUGINS_DLL ossimKakaduJ2kReader :
   public ossimImageHandler, public kdu_core::kdu_compressed_source
{
public:

   /** default construtor */
   ossimKakaduJ2kReader();
   
   /** virtural destructor */
   virtual ~ossimKakaduJ2kReader();

   /**
    * @brief Returns short name.
    * @return "ossim_kakadu_j2k_reader"
    */
   virtual ossimString getShortName() const;
   
   /**
    * @brief Returns long  name.
    * @return "ossim kakadu j2k reader"
    */
   virtual ossimString getLongName()  const;

   /**
    * @brief Returns short name.
    * @return "ossim_kakadu_j2k_reader"
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
    * @brief Gets the decimation factor for a resLevel.
    * @param resLevel Reduced resolution set for requested decimation.
    * @param result ossimDpt to initialize with requested decimation.
    */
   virtual void getDecimationFactor(ossim_uint32 resLevel,
                                    ossimDpt& result) const;

   /**
    * @brief Get array of decimations for all levels.
    * @param decimations Vector to initialize with decimations.
    */
   virtual void getDecimationFactors(vector<ossimDpt>& decimations) const; 

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

   /**
    * Method to the load (recreate) the state of an object from a keyword
    * list.  Return true if ok or false on error.
    */
   virtual bool loadState(const ossimKeywordlist& kwl,
                          const char* prefix=0);   

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

   /**
    * @brief Gets kdu source capability.
    *
    * Overrides kdu_compressed_source::get_capabilities
    *
    * @return KDU_SOURCE_CAP_SEEKABLE
    */
   virtual ossim_int32 get_capabilities();
   
   /**
    * @bried Read method.
    *
    * Implements pure virtual kdu_compressed_source::read
    * @param buf Buffer to fill.
    *
    * @param num_bytes Bytes to read.
    *
    * @return Actual bytes read.
    */
   virtual ossim_int32 read(kdu_core::kdu_byte *buf, ossim_int32 num_bytes);

   /**
    * @brief Seek method.
    *
    * Overrides kdu_compressed_source::seek
    *
    * @param offset Seek position relative to the start of code stream offset.
    *
    * @return true on success, false on error.
    */
   virtual bool seek(kdu_core::kdu_long offset);

   /**
    * @brief Get file position relative to the start of code stream offset.
    *
    * Overrides kdu_compressed_source::get_pos
    *
    * @return Position in codestream.
    */   
   virtual kdu_core::kdu_long get_pos();

private:

   /** @brief Initializes data member "theTile". */
   void initializeTile();

   /**
    * Loads a block of data to theCacheTile from the cache.
    * 
    * @param x Starting x position of block to load.
    *
    * @param y Starting y position of block to load.
    *
    * @param clipRect The rectangle to fill.
    *
    * @return true on success, false on error.
    *
    * @note x and y are zero based relative to the upper left corner so any
    * sub image offset should be subtracted off.
    */
   bool loadTileFromCache(ossim_uint32 x, ossim_uint32 y,
                          const ossimIrect& clipRect);
   
   /**
    * Loads a block of data to theCacheTile.
    * 
    * @param x Starting x position of block to load.
    *
    * @param y Starting y position of block to load.
    *
    * @return true on success, false on error.
    *
    * @note x and y are zero based relative to the upper left corner so any
    * sub image offset should be subtracted off.
    */
   bool loadTile(ossim_uint32 x, ossim_uint32 y);

   kdu_core::kdu_codestream     theCodestream;
   kdu_core::kdu_thread_env*    theThreadEnv;
   kdu_core::kdu_thread_queue*  theOpenTileThreadQueue;
   ossim_uint32                 theSourcePrecisionBits;
   ossim_uint32                 theMinDwtLevels;
   std::ifstream                theFileStr;
   ossimJ2kSizRecord            theSizRecord;
   ossimScalarType              theScalarType;
   ossimIrect                   theImageRect;
   ossimRefPtr<ossimImageData>  theTile;
   ossimRefPtr<ossimImageData>  theCacheTile;
   ossim_uint32                 theTileSizeX;
   ossim_uint32                 theTileSizeY;
   

   ossimAppFixedTileCache::ossimAppFixedCacheId theCacheId;
   
TYPE_DATA   
};

inline ossim_int32 ossimKakaduJ2kReader::get_capabilities()
{
   return ( KDU_SOURCE_CAP_SEEKABLE );
}

inline ossim_int32 ossimKakaduJ2kReader::read(kdu_core::kdu_byte *buf,
                                              ossim_int32 num_bytes)
{
   theFileStr.read((char*)buf, num_bytes);
   return theFileStr.gcount();
}

inline bool ossimKakaduJ2kReader::seek(kdu_core::kdu_long offset)
{
   // If the last byte is read, the eofbit must be reset. 
   if ( theFileStr.eof() )
   {
      theFileStr.clear();
   }

   //---
   // All seeks are relative to the start of code stream.
   //---
   theFileStr.seekg(offset, ios_base::beg);

   return theFileStr.good();
}

inline kdu_core::kdu_long ossimKakaduJ2kReader::get_pos()
{
   //---
   // Must subtract the SOC(start of code stream) from the real file position
   // since positions are relative to SOC.
   //---
   return static_cast<kdu_core::kdu_long>(theFileStr.tellg());
}

#endif /* #ifndef ossimKakaduJ2kReader_HEADER */
