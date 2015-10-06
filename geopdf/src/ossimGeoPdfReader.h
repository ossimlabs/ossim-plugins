//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Mingjie Su
//
// Description:  Class declaration GeoPdf reader.
//
// Specification: ISO/IEC 15444
//
//----------------------------------------------------------------------------
// $Id: ossimGeoPdfReader.h 21634 2012-09-06 18:15:26Z dburken $

#ifndef ossimGeoPdfReader_HEADER
#define ossimGeoPdfReader_HEADER 1

#include <iosfwd>
#include <fstream>
#include <vector>

//PoDoFo includes
#include <podofo/podofo.h>

//ossim includes
#include "../ossimPluginConstants.h"
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/imaging/ossimImageHandler.h>
#include <ossim/support_data/ossimJ2kSizRecord.h>
#include <ossim/imaging/ossimAppFixedTileCache.h>

// Forward class declarations.
class ossimImageData;
class ossimDpt;
class ossimProjection;
class PdfMemDocument;
class PdfObject;
class ossimGeoPdfVectorPageNode;

class OSSIM_PLUGINS_DLL ossimGeoPdfReader : public ossimImageHandler
{
public:

   /** default construtor */
   ossimGeoPdfReader();
   
   /** virtural destructor */
   virtual ~ossimGeoPdfReader();

   /**
    * @brief Returns short name.
    * @return "ossim_mrsid_reader"
    */
   virtual ossimString getShortName() const;
   
   /**
    * @brief Returns long  name.
    * @return "ossim mrsid reader"
    */
   virtual ossimString getLongName()  const;

   /**
    * @brief Returns class name.
    * @return "ossimGeoPdfReader"
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
    * Returns the tile width of the image or 0 if the image is not tiled.
    * Note: this is not the same as the ossimImageSource::getTileWidth which
    * returns the output tile width which can be different than the internal
    * image tile width on disk.
    */
   virtual ossim_uint32 getImageTileWidth() const;

   /**
    * Returns the tile width of the image or 0 if the image is not tiled.
    * Note: this is not the same as the ossimImageSource::getTileHeight which
    * returns the output tile height which can be different than the internal
    * image tile height on disk.
    */
   virtual ossim_uint32 getImageTileHeight() const;

   /**
    * Returns the output pixel type of the tile source.
    */
   virtual ossimScalarType getOutputScalarType() const;
   
   /**
    * @brief Gets number of lines for res level.
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
    * Returns the image geometry object associated with this tile source or
    * NULL if non defined.  The geometry contains full-to-local image
    * transform as well as projection (image-to-world).
    */
   virtual ossimRefPtr<ossimImageGeometry> getImageGeometry();

   /**
    * @param Method to get geometry.
    */
   virtual ossimRefPtr<ossimImageGeometry> getInternalImageGeometry() const;

   /**
    * Method to the load (recreate) the state of an object from a keyword
    * list.  Return true if ok or false on error.
    */
   virtual bool loadState(const ossimKeywordlist& kwl,
                          const char* prefix=0);   

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

   /*!
    * Method to save the state of an object to a keyword list.
    * Return true if ok or false on error.
    */
   virtual bool saveState(ossimKeywordlist& kwl,
                          const char* prefix=0)const;

protected:

  struct ossimFrameEntryData
  {
    ossimFrameEntryData()
      :theRow(-1),
      theCol(-1),
      thePixelRow(-1),
      thePixelCol(-1)
    {}
    ossimFrameEntryData(ossim_int32 row,
      ossim_int32 col,
      ossim_int32 pixelRow,
      ossim_int32 pixelCol,
      std::vector<PoDoFo::PdfObject*> entry)
      :theRow(row),
      theCol(col),
      thePixelRow(pixelRow),
      thePixelCol(pixelCol),
      theFrameEntry(entry)
    {}
    ossimFrameEntryData(const ossimFrameEntryData& rhs)
      :theRow(rhs.theRow),
      theCol(rhs.theCol),
      thePixelRow(rhs.thePixelRow),
      thePixelCol(rhs.thePixelCol),
      theFrameEntry(rhs.theFrameEntry)
    {}
    ossim_int32 theRow;
    ossim_int32 theCol;
    ossim_int32 thePixelRow;
    ossim_int32 thePixelCol;
    std::vector<PoDoFo::PdfObject*> theFrameEntry;
  };

  /**
    * It is important to note that each frame is organized into an easting northing
    * type orientation.  This means that a frame at 0,0 is at the lower left corner.
    * Each frame's pixel data is has 0,0 at the upper left.
    *
    * It will take the curent region to render and then find all entries that intersect
    * that region.
    *
    * @param rect the current region to render.
    * @return The list of entry data objects found for this rect.
    */
  std::vector<ossimFrameEntryData> getIntersectingEntries(const ossimIrect& rect);
   
private:

  ossimProjection* getGeoProjection();

  ossimProjection* getVPGeoProjection();

  ossimProjection* getLGIDictGeoProjection();

  ossimProjection* getProjectionFromStr(ossimString projContents);

  ossimDrect computeBoundingRect(ossimRefPtr<ossimImageGeometry> geoImage);

  void setPodofoInfo(PoDoFo::PdfObject* object);

  void setPodofoRefInfo(PoDoFo::PdfObject* object);

  void setPodofoArrayInfo(PoDoFo::PdfObject* object);

  void setPodofoDictInfo(PoDoFo::PdfObject* object);

  void parseTileStructure(std::vector<ossimString> tileInfo);

  void setPodofoImageInfo();

  void buildFrameEntryArray();

  void buildTileInfo(ossimString tileInfo);

  void resetCacheBuffer(ossimFrameEntryData entry);

  /**
    * @note this method assumes that setImageRectangle has been called on
    * theTile.
    */
  template <class T>
  void fillTile(T, // dummy template variable
                const ossimIrect& clip_rect, 
                ossimImageData* tile);
   
  ossimIrect                                                   m_imageRect; /** Has sub image offset. */
  ossim_uint32                                                 m_numberOfBands;
  ossim_uint32                                                 m_numberOfLines;
  ossim_uint32                                                 m_numberOfSamples;
  ossim_int32                                                  m_numOfFramesVertical;
  ossim_int32                                                  m_numOfFramesHorizontal;
  ossim_int32                                                  m_currentRow;
  ossim_int32                                                  m_currentCol;
  std::map<ossim_int32, ossim_int32>                           m_frameWidthVector;
  std::map<ossim_int32, ossim_int32>                           m_frameHeightVector;
  ossimScalarType                                              m_scalarType;
  ossimRefPtr<ossimImageData>                                  m_tile;
  
  std::map<ossimString, ossimString, ossimStringLtstr>         m_podofoProjInfo;

  //std::map<tileIndex, std::map<row, column> >
  std::map<ossim_int32, std::pair<ossim_int32, ossim_int32> >  m_podofoTileInfo;  
  std::map<ossim_int32, PoDoFo::PdfObject*>                    m_podofoImageObjs;
  PoDoFo::PdfMemDocument*                                      m_pdfMemDocument;
  bool                                                         m_isLGIDict;
  bool                                                         m_isJpeg;
  std::vector<ossimGeoPdfVectorPageNode*>                      m_pageVector;

  ossimRefPtr<ossimImageData>                                  m_cacheTile;
  ossimIpt                                                     m_cacheSize;
  ossimAppFixedTileCache::ossimAppFixedCacheId                 m_cacheId;
  std::vector<std::vector< std::vector<PoDoFo::PdfObject*> > > m_frameEntryArray;
TYPE_DATA
};

#endif /* #ifndef ossimGeoPdfReader_HEADER */
