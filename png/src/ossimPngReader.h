//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  David Burken
//
// Description: OSSIM Portable Network Graphics (PNG) reader (tile source).
//
//----------------------------------------------------------------------------
// $Id: ossimPngReader.h 23355 2015-06-01 23:55:15Z dburken $
#ifndef ossimPngReader_HEADER
#define ossimPngReader_HEADER 1

#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/imaging/ossimStreamReaderInterface.h>
#include <ossim/imaging/ossimAppFixedTileCache.h>
#include <png.h>
#include <vector>

class ossimImageData;

class ossimPngReader : public ossimImageHandler, public ossimStreamReaderInterface
{
public:

   enum ossimPngReadMode
   {
      ossimPngReadUnknown = 0,
      ossimPngRead8       = 1,
      ossimPngRead16      = 2,
      ossimPngRead8a      = 3,
      ossimPngRead16a     = 4 
   };

   /** default constructor */
   ossimPngReader();

   /** virtual destructor */
   virtual ~ossimPngReader();

   /** @return "png" */
   virtual ossimString getShortName() const;

   /** @return "ossim png" */
   virtual ossimString getLongName()  const;

   /** @return "ossimPngReader" */
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
   virtual ossim_uint32 getNumberOfLines(ossim_uint32 reduced_res_level = 0) const;

   /**
    *  Returns the number of samples in the image.
    *  Satisfies pure virtual from ImageHandler class.
    */
   virtual ossim_uint32 getNumberOfSamples(ossim_uint32 reduced_res_level = 0) const;

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

   virtual double getMaxPixelValue(ossim_uint32 band = 0)const;
   
   /**
    *  @brief open method.
    *
    *  This open takes a stream, postition and a flag.
    *
    *  @param str Open stream to image.
    *
    *  @param restartPosition Typically 0, this is the stream offset to the
    *  front of the image.
    *
    *  @param youOwnIt If true this object takes owner ship of the pointer
    *  memory and will destroy on close.
    *  
    *  @return true on success, false on error.
    */
   virtual bool open( std::istream* str,
                      std::streamoff restartPosition,
                      bool youOwnIt );

   /** Close method. */
   virtual void close();

   /**
    * @return true if first 8 bytes matches png signature, false if not.
    */
   bool checkSignature(std::istream* str);

   /**
    * @brief Gets the image geometry.
    *
    * This method overrides ossimImageHandler::getImageGeometry to NOT
    * call ossimImageGeometryRegistry::instance()->extendGeometry().
    * 
    * @return the image geometry object associated with this tile source.
    * In many case the underlying ossimImageGeometry projection may be NULL
    * with this reader so callers should check.
    */
   virtual ossimRefPtr<ossimImageGeometry> getImageGeometry();
   
protected:
   
   /**
    * @brief Performs signature check and initializes png_structp and png_infop.
    * @return true on success, false on error.
    */
   bool readPngInit();

   /**
    * @brief Initializes this reader from libpng m_pngPtr and infoPtr.
    * @return true on success, false on error.
    */
   bool initReader();
   
   void readPngVersionInfo();
   ossimString getPngColorTypeString() const;

   /**
    * @Sets the max pixel value.  Attempts to get the sBIT chunk.
    */
   void setMaxPixelValue();

   /**
    *  @brief open method.
    *
    *  This open assumes base class ossimImageHandler::theImageFile has been
    *  set.
    *
    *  @return true on success, false on error.
    */
   virtual bool open();

   /**
    * @brief Initializes tiles and buffers.  Called once on first
    * getTile request.
    */ 
   void allocate();

   /**
    * @brief Free tile and buffer memory.  Deletes stream if own it flag
    * is set.
    */
   void destroy();

   /**
    * @brief Method to restart reading from the beginning (for backing up).
    * This is needed as libpng requires sequential read from start of the
    * image.
    */
   void restart();

   /**
    * @note this method assumes that setImageRectangle has been called on
    * theTile.
    */
   void fillTile(const ossimIrect& clip_rect, ossimImageData* tile);

   template <class T> void copyLines(T dummy,  ossim_uint32 stopLine);
   template <class T> void copyLinesWithAlpha(T, ossim_uint32 stopLine);

   /**
    * @brief Callback method for reading from a stream.  This allows us to choose
    * the stream; hence, writing to a file or memory.  This will be passed
    * to libpng's png_set_read_fn.
    */
   static void pngReadData(
      png_structp png_ptr, png_bytep data, png_size_t length);

   ossimRefPtr<ossimImageData>  m_tile;
   ossimRefPtr<ossimImageData>  m_cacheTile;

   ossim_uint8*  m_lineBuffer;
   ossim_uint32  m_lineBufferSizeInBytes;

   std::istream*  m_str;
   std::streamoff m_restartPosition;
   bool           m_ownsStream;
   
   ossimIrect    m_bufferRect;
   ossimIrect    m_imageRect;
   ossim_uint32  m_numberOfInputBands;
   ossim_uint32  m_numberOfOutputBands;
   ossim_uint32  m_bytePerPixelPerBand;
   ossimIpt      m_cacheSize;


   ossimAppFixedTileCache::ossimAppFixedCacheId m_cacheId;

   png_structp      m_pngReadPtr;
   png_infop        m_pngReadInfoPtr;
   ossim_int8       m_pngColorType;
   ossim_uint32     m_currentRow; // 0 at start or first line
   ossimScalarType  m_outputScalarType;
   ossim_int32      m_interlacePasses;
   ossim_int8       m_bitDepth;
   ossimPngReadMode m_readMode;

   std::vector<ossim_float64> m_maxPixelValue;

   bool m_swapFlag;

   // If true the alpha channel will be passed on as a band.
   bool m_useAlphaChannelFlag;
   
TYPE_DATA
};

#endif /* #ifndef ossimPngReader_HEADER */
