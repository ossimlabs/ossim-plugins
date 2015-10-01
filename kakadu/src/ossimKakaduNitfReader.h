//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description:
//
// Class declaration for reader of NITF images with JPEG2000 (J2K) compressed
// blocks using kakadu library for decompression.
//
//----------------------------------------------------------------------------
// $Id: ossimKakaduNitfReader.h 22884 2014-09-12 13:14:35Z dburken $

#ifndef ossimKakaduNitfReader_HEADER
#define ossimKakaduNitfReader_HEADER 1

#include <ossim/plugin/ossimPluginConstants.h>
#include <ossim/imaging/ossimNitfTileSource.h>
#include <kdu_compressed.h>
#include <iosfwd>
#include <fstream>

// Forward declarations:

namespace kdu_core
{
   class kdu_thread_queue;
}

namespace kdu_supp
{
   class jp2_family_src;
   class jp2_source;
   struct kdu_channel_mapping;
}



/**
 * @brief ossimKakaduNitfReader class for reading NITF images with JPEG2000
 * (J2K) compressed blocks using kakadu library for decompression. 
 */
class OSSIM_PLUGINS_DLL ossimKakaduNitfReader :
   public ossimNitfTileSource, public kdu_core::kdu_compressed_source
{
public:

   /** Anonymous enumerations: */
   enum
   {
      SIGNATURE_BOX_SIZE = 12
   };

   /** default construtor */
   ossimKakaduNitfReader();
   
   /** virtural destructor */
   virtual ~ossimKakaduNitfReader();

   /**
    * @brief Returns short name.
    * @return "ossim_kakadu_nitf_reader"
    */
   virtual ossimString getShortName() const;
   
   /**
    * @brief Returns long  name.
    * @return "ossim kakadu nitf reader"
    */
   virtual ossimString getLongName()  const;

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
    * Overrides ossimNitfTileSource::getNumberOfLines
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
    *  Overrides ossimNitfTileSource::getNumberOfSamples
    *
    *  @param resLevel Reduced resolution level to return samples of.
    *  Default = 0
    *
    *  @return The number of samples for specified reduced resolution level.
    */
   virtual ossim_uint32 getNumberOfSamples(ossim_uint32 resLevel = 0) const;

   /**
    * @brief convenience method to set min pixel value.
    *
    * Added for overview readers so that the image handler that owns the
    * overview reader can pass on it's min value.
    *
    * @param band Zero based band to set.
    *
    * @param pix Min pixel value.
    */
   virtual void setMinPixelValue(ossim_uint32 band, const ossim_float64& pix);

   /**
    * @brief convenience method to set max pixel value.
    *
    * Added for overview readers so that the image handler that owns the
    * overview reader can pass on it's max value.
    *
    * @param band Zero based band to set.
    *
    * @param pix Max pixel value.
    */
   virtual void setMaxPixelValue(ossim_uint32 band, const ossim_float64& pix);

   /**
    * @brief convenience method to set null pixel value.
    *
    * Added for overview readers so that the image handler that owns the
    * overview reader can pass on it's max value.
    *
    * @param band Zero based band to set.
    *
    * @param pix Null pixel value.
    */
   virtual void setNullPixelValue(ossim_uint32 band, const ossim_float64& pix);

   /** @return the min pixel value either from base class call or theMinPixelValue if set. */
   virtual ossim_float64 getMinPixelValue(ossim_uint32 band=0)const;

   /** @return the max pixel value either from base class call or theMaxPixelValue if set. */
   virtual ossim_float64 getMaxPixelValue(ossim_uint32 band=0)const;

   /** @return the null pixel value either from base class call or theNullPixelValue if set. */
   virtual ossim_float64 getNullPixelValue(ossim_uint32 band=0)const;

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
  virtual bool loadBlock(ossim_uint32 x, ossim_uint32 y);
  
   /**
    * @brief Parses "theImageFile" and initializes all nitf headers.
    * 
    * Overrides ossimNitfTileSource::parseFile
    *
    * @return True on success, false on error.
    */
   virtual bool parseFile();

   /**
    * @brief Allocates everything for current entry.
    *
    * This is called on an open() or a entry change to an already open nitf
    * file.
    *
    * This does not allocate buffers and tiles to keep open and
    * setCurrentEntry times to a minimum.  Buffers are allocated on first
    * grab of pixel data by allocatBuffers method.
    *
    * Overrides ossimNitfTileSource::allocate
    * 
    * @return True on success, false on error.
    */
   virtual bool allocate(); 

   /**
    * @brief Allocates buffers for current entry.
    *
    * This is called on first getTile().
    * 
    * Overrides ossimNitfTileSource::allocateBuffers
    * 
    * @return True on success, false on error.
    */
   virtual bool allocateBuffers();
   
   /**
    * @brief Checks header to see is we can decompress entry.
    *
    * If compression code is not "C8" calls base canUncompress as you could
    * have a J2k entry and a jpeg entry.
    *
    * Overrides ossimNitfTileSource::canUncompress
    * 
    * @param hdr Pointer to image header.
    * 
    * @return true if reader can uncompress nitf.
    * */
   virtual bool canUncompress(const ossimNitfImageHeader* hdr) const;

   /**
    * @brief Initializes the data member "theReadMode" from the current entry.
    *
    * Overrides ossimNitfTileSource::initializeReadMode
    */
   virtual void initializeReadMode();

   /**
    * @brief This method simply checks for Start of Codestream (SOC) marker as
    * the Kakadu library handles finding tiles for us.
    *
    * Overrides ossimNitfTileSource::scanForJpegBlockOffsets
    * 
    * @return true on success, false on error.
    */
   virtual bool scanForJpegBlockOffsets();

   /**
    * @brief Sets the "theSwapBytesFlag" from the current entry.
    *
    * Overrides ossimNitfTileSource::initializeSwapBytesFlag
    */
   virtual void initializeSwapBytesFlag();

   /**
    * @brief Initializes the data member "theCacheTileInterLeaveType".
    *
    * Overrides ossimNitfTileSource::initializeCacheTileInterLeaveType
    */
   virtual void initializeCacheTileInterLeaveType();

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

   /**
    * @brief Checks current entry image header to see if it is j2k.
    * @return true if j2k, false if not.
    */
   bool isEntryJ2k() const;

   /**
    * @brief Checks for jp2 signature block.
    *
    * Note the stream is repositioned to start of data after the check so that the
    * jp2 open code does not have to reposition.
    * 
    * @return true if jp2, false if not.
    */
   bool checkJp2Signature();

   /**
    * Initializes m_channels to be used with copyRegionToTile if it makes sense.
    */
   void configureChannelMapping();

   

   /** @brief Method to print out tile info. For debug only.  */
   std::ostream& dumpTiles(std::ostream& out);

   std::streamoff m_startOfCodestreamOffset;

   kdu_supp::jp2_family_src*      m_jp2FamilySrc;
   kdu_supp::jp2_source*          m_jp2Source;
   kdu_supp::kdu_channel_mapping* m_channels;
   kdu_core::kdu_codestream       m_codestream;
   kdu_core::kdu_thread_env*      m_threadEnv;
   kdu_core::kdu_thread_queue*    m_openTileThreadQueue;
   
   ossim_uint32               m_sourcePrecisionBits;
   ossim_uint32               m_minDwtLevels;
   
   /** Image dimensions for each level. */
   std::vector<ossimIrect>    m_imageDims;

   /**
    * min/max/null These are class attributes so that when behaving as an overview the owner or
    * base image handler can pass its min/max/null to us.  For non eight bit data this is
    * critical so that the data gets stretched exactly as the base handler.  Did NOT make
    * these a vector the size of bands as it was not needed at time of coding.
    */
   ossim_float64              m_minSampleValue;
   ossim_float64              m_maxSampleValue;
   ossim_float64              m_nullSampleValue;

TYPE_DATA   
};

inline ossim_int32 ossimKakaduNitfReader::get_capabilities()
{
   return ( KDU_SOURCE_CAP_SEEKABLE | KDU_SOURCE_CAP_SEQUENTIAL );
}

inline ossim_int32 ossimKakaduNitfReader::read(kdu_core::kdu_byte *buf, ossim_int32 num_bytes)
{
   theFileStr.read((char*)buf, num_bytes);
   return theFileStr.gcount();
}

inline bool ossimKakaduNitfReader::seek(kdu_core::kdu_long offset)
{
   // If the last byte is read, the eofbit must be reset. 
   if ( theFileStr.eof() )
   {
      theFileStr.clear();
   }

   //---
   // All seeks are relative to the start of code stream.
   //---
   theFileStr.seekg(offset+m_startOfCodestreamOffset, ios_base::beg);
   return theFileStr.good();
}

inline kdu_core::kdu_long ossimKakaduNitfReader::get_pos()
{
   //---
   // Must subtract the SOC(start of code stream) from the real file position
   // since positions are relative to SOC.
   //---
   return static_cast<kdu_core::kdu_long>(theFileStr.tellg() - m_startOfCodestreamOffset );
}

#endif /* #ifndef ossimKakaduNitfReader_HEADER */
