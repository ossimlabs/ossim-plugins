//----------------------------------------------------------------------------
//
// License:  See top level LICENSE.txt file
//
// Author:  David Burken
//
// Description: OSSIM Open JPEG JP2 reader (tile source).
//
// http://www.openjpeg.org/
//
//----------------------------------------------------------------------------
// $Id$

#ifndef ossimOpjJp2Reader_HEADER
#define ossimOpjJp2Reader_HEADER 1

#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/support_data/ossimJ2kSizRecord.h>
#include <vector>

// Forward class declarations.
class ossimImageData;
class ossimJ2kCodRecord;

class ossimOpjJp2Reader : public ossimImageHandler
{
public:

   /** Anonymous enumerations: */
   enum
   {
      SIGNATURE_BOX_SIZE = 12,
      GEOTIFF_UUID_SIZE  = 16
   };
   
   enum ossimOpjJp2ReadMode
   {
      ossimOpjJp2ReadUnknown = 0
   };

   /** default constructor */
   ossimOpjJp2Reader();

   /** virtual destructor */
   virtual ~ossimOpjJp2Reader();

   /** @return "ossim_openjpeg_reader" */
   virtual ossimString getShortName() const;

   /** @return "ossim open jpeg reader" */
   virtual ossimString getLongName()  const;

   /** @return "ossimOpjJp2Reader" */
   virtual ossimString getClassName()    const;

   /**
    *  Returns a pointer to a tile given an origin representing the upper
    *  left corner of the tile to grab from the image.
    *  Satisfies pure virtual from TileSource class.
    */
   virtual ossimRefPtr<ossimImageData> getTile(const ossimIrect& rect,
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
    * Returns the output pixel type of the tile source.
    */
   virtual ossimScalarType getOutputScalarType() const;

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

   virtual bool isOpen()const;

   virtual double getMaxPixelValue(ossim_uint32 band = 0)const;

   /**
    *  @brief open method.
    *  @return true on success, false on error.
    */
   virtual bool open();

   /** close method */
   virtual void close();

   /**
    * Returns the image geometry object associated with this tile source or
    * NULL if non defined.  The geometry contains full-to-local image
    * transform as well as projection (image-to-world).
    */
   virtual ossimRefPtr<ossimImageGeometry> getImageGeometry();

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
    * @param Method to get geometry from the embedded JP2 Boxes.
    */
   virtual ossimRefPtr<ossimImageGeometry> getInternalImageGeometry();

   /**
    * @param Method to get geometry from the embedded JP2 GeoTIFF Box.
    */
   virtual ossimRefPtr<ossimImageGeometry> getImageGeometryFromGeotiffBox();

   /**
    * @param Method to get geometry from the embedded JP2 GML Box.
    */
   virtual ossimRefPtr<ossimImageGeometry> getImageGeometryFromGmlBox();
   
   /**
    * @param Method to get geometry from the various external files like
    * .prj, .j2w .xml and so on.
    */
   ossimRefPtr<ossimImageGeometry> getMetadataImageGeometry() const;

   // Cleans memory.  Called on close or destruct.
   void destroy();
   

   bool initSizRecord( std::istream* str,
                       ossimJ2kSizRecord& sizRecord) const;

   bool initCodRecord( std::istream* str,
                       ossimJ2kCodRecord& sizRecord) const;

   /**
    * @brief Initializes tiles.  Called once on first getTile request.
    */ 
   void allocate();

   ossimJ2kSizRecord m_sizRecord;
   
   ossimRefPtr<ossimImageData>  m_tile;
   ossimRefPtr<ossimImageData>  m_cacheTile;   
   std::ifstream*               m_str;
   ossim_uint32                 m_minDwtLevels;
   ossim_int32                  m_format; // OPJ_CODEC_FORMAT
   
TYPE_DATA
};

#endif
