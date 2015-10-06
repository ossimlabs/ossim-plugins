//----------------------------------------------------------------------------
//
// File: ossimGpkgWriter.h
//
// Author:  David Burken
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: OSSIM Geo Package writer.
//
//----------------------------------------------------------------------------
// $Id$
#ifndef ossimGpkgWriter_HEADER
#define ossimGpkgWriter_HEADER 1

#include <ossim/imaging/ossimGpkgWriterInterface.h>
#include <ossim/imaging/ossimImageFileWriter.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimRefPtr.h>
#include <ossim/imaging/ossimCodecBase.h>
#include <vector>

// Forward class declarations.
class ossimDpt;
class ossimImageData;
class ossimIrect;
struct jpeg_compress_struct;
struct sqlite3;
struct sqlite3_stmt;

/**
 * @class ossimGpkgWriter
 */
class ossimGpkgWriter :
   public ossimImageFileWriter, public ossimGpkgWriterInterface
{
public:

   // Anonymous enums:
   enum
   {
      DEFAULT_JPEG_QUALITY = 75
   };
   
   enum ossimGpkgWriterMode
   {
      OSSIM_GPGK_WRITER_MODE_UNKNOWN = 0,
      OSSIM_GPGK_WRITER_MODE_JPEG    = 1,  // RGB, 8-bit, JPEG compressed
      OSSIM_GPGK_WRITER_MODE_PNG     = 2,  // PNG,
      OSSIM_GPGK_WRITER_MODE_PNGA    = 3,  // PNG with alpha
      OSSIM_GPGK_WRITER_MODE_MIXED   = 4   // full tiles=jpeg, partials=pnga
   };

   /* default constructor */
   ossimGpkgWriter();

   /* virtual destructor */
   virtual ~ossimGpkgWriter();

   /** @return "gpkg writer" */
   virtual ossimString getShortName() const;

   /** @return "ossim gpkg writer" */
   virtual ossimString getLongName()  const;

   /** @return "ossimGpkgReader" */
   virtual ossimString getClassName()    const;

   /**
    * Returns a 3-letter extension from the image type descriptor 
    * (theOutputImageType) that can be used for image file extensions.
    *
    * @param imageType string representing image type.
    *
    * @return the 3-letter string extension.
    */
   virtual ossimString getExtension() const;

   /**
    * void getImageTypeList(std::vector<ossimString>& imageTypeList)const
    *
    * Appends this writer image types to list "imageTypeList".
    *
    * This writer only has one type "gpkg".
    *
    * @param imageTypeList stl::vector<ossimString> list to append to.
    */
   virtual void getImageTypeList(std::vector<ossimString>& imageTypeList)const;
   
   /**
    * saves the state of the object.
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
    * Will set the property whose name matches the argument
    * "property->getName()".
    *
    * @param property Object containing property to set.
    */
   virtual void setProperty(ossimRefPtr<ossimProperty> property);

   /**
    * @param name Name of property to return.
    * 
    * @returns A pointer to a property object which matches "name".
    */
   virtual ossimRefPtr<ossimProperty> getProperty(const ossimString& name)const;

   /**
    * Pushes this's names onto the list of property names.
    *
    * @param propertyNames array to add this's property names to.
    */
   virtual void getPropertyNames(std::vector<ossimString>& propertyNames)const;

   bool hasImageType(const ossimString& imageType) const;

   /**
    * Get the gpkg compression level as a string.
    *
    * @return The level which will be one of:
    * z_no_compression
    * z_best_speed
    * z_best_compression
    * z_default_compression
    */
   ossimString getCompressionLevel() const;

   /**
    * Set the gpkg compression level from a string.
    *
    * @param level Should be one of:
    * z_no_compression
    * z_best_speed
    * z_best_compression
    * z_default_compression
    *
    * @return true on success, false if level is not accepted.
    */
   bool setCompressionLevel(const ossimString& level);

       
   virtual bool isOpen()const;   
   
   virtual bool open();

   /**
    * @brief Opens file for writing, appending, merging without an input
    * connection. I.e. opening, then calling writeTile directly.
    *
    * Option keywords:
    * append: bool false=create_new, true=append_existing
    * epsg: 4326, 3857
    * filename: output_file.gpkg
    * tile_table_name: default="tiles"
    * writer_mode: mixed(default), jpeg, png, pnga
    * 
    * 
    * @param options.  Keyword list containing all options.
    */
   virtual bool openFile( const ossimKeywordlist& options );
   
   virtual void close();

   /**
    * @brief Gets the writer mode.
    *
    * Default mode = jpeg.
    *
    * Potential mode are:
    * jpeg
    * png
    * pnga
    * 
    * @return Writer mode.  Default mode = jpeg.
    */
   ossimGpkgWriterMode getWriterMode() const;

   /**
    * @brief Gets the writer mode as string.
    *
    * Default mode = jpeg.
    *
    * Potential mode are:
    * jpeg
    * png
    * pnga
    * 
    * @return Writer mode.  Default mode = jpeg.
    */
   std::string getWriterModeString( ossimGpkgWriterMode mode ) const;

   void setCompressionQuality( const std::string& quality );

   /**
    * @brief Gets the compression quality.
    *
    * Result is pulled from options keyword list where
    * key=compression_quality.
    * 
    * @return Compression quality.or 0 if not found.
    */
   ossim_uint32 getCompressionQuality() const;

   /**
    * @brief Calls initial sqlite3_prepare_v2 statement.  Must be called
    * prior to calling writeTile.
    * @return SQLITE_OK(0) on success, something other(non-zero) on failure.
    */
   virtual ossim_int32 beginTileProcessing();

   /**
    * @brief Direct interface to writing a tile to database.
    * @param tile to write.
    * @param zoolLevel
    * @param row
    * @param col
    * @note Can throw ossimException.
    * @return true on success, false on error.
    */
   virtual bool writeTile( ossimRefPtr<ossimImageData>& tile,
                           ossim_int32 zoomLevel,
                           ossim_int64 row,
                           ossim_int64 col);

   /**
    * @brief Direct interface to writing a Codec tile to database.
    * @param codecTile to write.
    * @param codecTileSize 
    * @param zoolLevel
    * @param row
    * @param col
    * @note Can throw ossimException.
    * @return true on success, false on error.
    */
   virtual bool writeCodecTile( ossim_uint8* codecTile,
                           ossim_int32 codecTileSize,
                           ossim_int32 zoomLevel,
                           ossim_int64 row,
                           ossim_int64 col);

   /**
    * @brief Calls sqlite3_finalize(pStmt) terminating tile processing.
    */
   virtual void finalizeTileProcessing();
   
private:

   /**
    * @brief Writes the file to disk or a stream.
    * @return true on success, false on error.
    */
   virtual bool writeFile();

   /**
    * @brief Writes an entry to gpkg.  This could be either a new file, or a
    * new tile table.
    * @return true on success, false on error.
    */
   bool writeEntry();

   /**
    * @brief Adds levels to an existing an gpkg.
    * @return true on success, false on error.
    */  
   bool addLevels();
   
   bool createTables( sqlite3* db );

   /**
    * @return the "srs_id" number pointing to the
    * gpkg_spatial_ref_sys record for our projection.
    */
   ossim_int32 writeGpkgSpatialRefSysTable(
      sqlite3* db, const ossimMapProjection* proj );

   bool writeGpkgContentsTable( sqlite3* db,
                                const ossimDrect& boundingRect );
   
   bool writeGpkgTileMatrixSetTable( sqlite3* db,
                                     const ossimDrect& boundingRect );

   /**
    * @brief Initialize method.
    * @param db
    * @param zoom_level.  Zero being whole Earth...
    * @param matrixSize Size of tile matrix, i.e. number of horizontal
    * vertical tiles.
    * @param gsd Size of one pixel either in meters or lat lon.
    * @return true on success, false on error.
    */  
   bool writeGpkgTileMatrixTable( sqlite3* db,
                                  ossim_int32 zoom_level,
                                  const ossimIpt& matrixSize,
                                  const ossimDpt& gsd );

   
   bool writeGpkgNsgTileMatrixExtentTable( sqlite3* db,
                                           ossim_int32 zoom_level,
                                           const ossimIrect& expandedAoi,
                                           const ossimIrect& clippedAoi);

   void writeZoomLevels( sqlite3* db,
                         ossimMapProjection* proj,
                         const std::vector<ossim_int32>& zoomLevels );

   void writeTiles( sqlite3* db,
                    const ossimIrect& aoi,
                    ossim_int32 zoomLevel,
                    const ossim_float64& totalTiles,
                    ossim_float64& tilesWritten );  

   void writeTile( sqlite3_stmt* pStmt,
                   sqlite3* db,
                   ossimRefPtr<ossimImageData>& tile,
                   ossim_int32 zoomLevel,
                   ossim_int64 row,
                   ossim_int64 col);
   void writeCodecTile( sqlite3_stmt* pStmt,
                   sqlite3* db,
                   ossim_uint8* codecTile,
                   ossim_int32 codecTileSize,
                   ossim_int32 zoomLevel,
                   ossim_int64 row,
                   ossim_int64 col);
   
/*
   void writeJpegTiles( sqlite3* db,
                        const ossimIrect& aoi,
                        ossim_int32 zoomLevel,
                        ossim_uint32 quality,
                        const ossim_float64& totalTiles,
                        ossim_float64& tilesWritten );  

   void writeJpegTile( sqlite3_stmt* pStmt,
                       sqlite3* db,
                       const ossimRefPtr<ossimImageData>& tile,
                       ossim_int32 zoomLevel,
                       ossim_uint32 row,
                       ossim_uint32 col,
                       ossim_uint32 quality );
*/
   void getGsd( const ossimDpt& fullResGsd,
                ossim_int32 fullResZoomLevel,
                ossim_int32 currentZoomLevel,
                ossimDpt& gsd );

   /**
    * @brief Get the gsd.
    *
    * Gets gsd in either meters or decimal degrees dependent on if the
    * projection is "geographic" or not.
    *
    * @param proj
    * @param gsd Initialized by this.
    */
   void getGsd( const ossimMapProjection* proj,
                ossimDpt& gsd ) const;

   /**
    * @brief Get the gsd.
    *
    * Gets gsd in either meters or decimal degrees dependent on if the
    * projection is "geographic" or not.
    *
    * @param geom
    * @param gsd Initialized by this.
    */
   void getGsd( const ossimImageGeometry* geom,
                ossimDpt& gsd ) const;

   /**
    * @brief Get the gsd.
    *
    * Gets gsd in either meters or decimal degrees dependent on if the
    * projection is "geographic" or not.
    *
    * This method assumes zoom level are aligned to the projection grid. If
    * align_to_grid==false gsd will be "nan"ed.
    *
    * @param proj Output projection. 
    * @param zoomLevel
    * @param gsd Initialized by this.
    */
   void getGsd( const ossimMapProjection* proj,
                ossim_int32 zoomLevel,
                ossimDpt& gsd ) const;

   /** @return true if align to grid option is set; false, if not. */
   bool alignToGrid() const;

   /**
    * @brief Check if file is to be open new or appended.
    *
    * Checks for keywords: add_entry, add_levels
    * 
    * @return true if any append option is set; false, if not.
    */
   bool append() const;

   /** @return true if "add_levels" key is set to true; false, if not. */
   bool addLevels() const;

   /** @return true if "add_entry" key is set to true; false, if not. */
   bool addEntry() const;

   /**
    * @brief Get the output projection.
    *
    * Output projection type is determined in this order:
    * 1) User set epsg code, e.g. "3395".
    * 2) Input projection.
    * 3) Default output (currently geographic.
    * @return Projection wrapped in ref pointer.
    */
   ossimRefPtr<ossimMapProjection> getNewOutputProjection(
      ossimImageGeometry* geom ) const;

   /** @brief Gets projection from "epsg" code if in options list. */
   ossimRefPtr<ossimMapProjection> getNewOutputProjection() const;

   ossimRefPtr<ossimMapProjection> getNewGeographicProjection() const;
   
   ossimRefPtr<ossimMapProjection> getNewWorldMercatorProjection() const;

   // Propagates view to all resamplers.
   void setView( ossimMapProjection* proj );

   /**
    * @brief Finds all combiners and calls initialize to reset the bounding box
    * after a view change.
    */
   void reInitializeCombiners();

   /**
    * @brief Finds all ossimRectangleCutter and calls setRectangle with a nan
    * rect to reset the bounding box after a view change.
    */
   void reInitializeCutters( const ossimMapProjection* proj );
   
   /** @return true if key is set to true; false, if not. */
   bool keyIsTrue( const std::string& key ) const;

   void getTileSize( ossimIpt& tileSize ) const;

   /**
    * @brief This is the number of transactions batched before being executed.
    * @return sqlite transaction batch size.
    */
   ossim_uint64 getBatchSize() const;

   /**
    * @brief Zoom levels needed to get AOI down to one tile.
    * @param aoi Area of Interest.
    */
   ossim_int32 getNumberOfZoomLevels( const ossimIrect& aoi ) const;

   /**
    * @brief Gets zoom levels from options keyword list if set.
    *
    * Keyword: "zoom_levels"
    *
    * Splits comma separated list, e.g. "(8,9,10,11)"
    *
    * If align to grid is on levels are based on projection grid; else,
    * stop zoom level is 0, start is the full res level.
    * @param proj
    * @param aoi
    * @param zoomLevels Intitialized by this.
    * @param fullResGsd Intitialized by this.
    */
   void getZoomLevels( std::vector<ossim_int32>& zoomLevels ) const;
   

   /**
    * @brief Computes start and stop level.
    *
    * If align to grid is on levels are based on projection grid; else,
    * stop zoom level is 0, start is the full res level.
    * @param proj
    * @param aoi
    * @param sourceGsd Full res gsd of input.
    * @param zoomLevels Intitialized by this.
    * @param fullResGsd Intitialized by this to the highest resolution of zoom
    * levels.  This may or may not be the same as the sourceGsd.
    */
   void getZoomLevels( const ossimMapProjection* proj,
                       const ossimIrect& aoi,
                       const ossimDpt& sourceGsd,
                       std::vector<ossim_int32>& zoomLevels,
                       ossimDpt& fullResGsd ) const;

   /**
    * @brief Get the view coordinates for edge to edge rect.
    */
   void getAoiFromRect( const ossimMapProjection* proj,
                        const ossimDrect& rect,
                        ossimIrect& aoi );
   
   /**
    * @brief Gets aoi expanded to tile boundaries.
    *
    * This initializes expandedAoi with the aoi expanded to tile boundaries.
    * 
    * @param aoi Area of interest.
    * @param expandedAoi Initialized by this.
    */
   void getExpandedAoi( const ossimIrect& aoi, ossimIrect& expandedAoi ) const;
   
   void getMatrixSize( const ossimIrect& rect,
                       ossimIpt& matrixSize ) const;

   void setProjectionTie( ossimMapProjection* proj ) const;

   bool requiresEightBit() const;

   ossim_uint32 getEpsgCode() const;

   /**
    * @brief Gets the projection dimensions in meters.
    * @param proj Projection
    * @param dims Initialized by this.
    */
   void getProjectionDimensionsInMeters(
      const ossimMapProjection* proj, ossimDpt& dims ) const;

#if 0
   void clipToProjectionBounds(
      const ossimMapProjection* proj,
      const ossimGpt& inUlGpt, const ossimGpt& inLrGpt,
      ossimGpt& outUlGpt, ossimGpt& outLrGpt ) const;
#endif

   void initializeProjectionRect( const ossimMapProjection* productProj );

   void initializeRect( const ossimMapProjection* proj,
                        const ossimIrect& aoi,
                        ossimDrect& rect );

   /**
    * @brief Gets the tile table name.
    *
    * Looks for "tile_table_name" and returns value if found;
    * else, default="tiles".
    *
    * @return tile table name.
    */
   void getTileTableName( std::string& tileTableName ) const;

   bool getFilename( ossimFilename& file ) const;

   /**
    * Initializes m_fullTileCodec and m_partialTileCodec.
    */
   void initializeCodec();

   /**
    * @brief Initializes the output gpkg file.  This method is used for
    * non-connected writing, e.g. openFile(...), writeTile(...)
    *
    * Assumes new gpkg with no input connection.
    * 
    * @return true on success, false on error.
    */
   bool initializeGpkg();

   /**
    * @brief Get rectangle in projected space from key: cut_wms_bbox
    * key:value form:
    * cut_wms_bbox: <minx>,<miny>,<maxx>,<maxy>
    * @param rect Initialized by this.  This is in output projection
    * coordinate space.
    * @return true on success; false on error.
    */
   bool getWmsCutBox( ossimDrect& rect ) const;

   /**
    * @brief Get clip rectangle in projected space from key: clip_extents
    * key:value form:
    * clip_extents: <minx>,<miny>,<maxx>,<maxy>
    * @param rect Initialized by this.  This is in output projection
    * coordinate space.
    * @param alignToGridFlag. Indicates if the clip extents need to be aligned.
    * The flag will be defaulted to true.
    * @return true on success; false, on error.
    */
   bool getClipExtents( ossimDrect& rect, bool& alignToGridFlag ) const;
  

    /**
    * @brief Gets rectangle.
    * Method assumes form of:
    * key: <minx>,<miny>,<maxx>,<maxy>
    * e.g. "clip_extents: <minx>,<miny>,<maxx>,<maxy>"
    * @param rect Initialized by this.  This is in output projection
    * coordinate space.
    * @return true on success; false, on error.
    */
   bool getRect( const std::string& key, ossimDrect& rect ) const;

   /**
    * @brief Checks for:
    * new level lower then existing.
    * new level already present
    *
    * Throws exception on error.
    */
   void checkLevels( const std::vector<ossim_int32>& currentZoomLevels,
                     const std::vector<ossim_int32>& newZoomLevels ) const;

   /**
    * @breif Checks to see if level, row, column are within range of existing
    * gpkg.
    * @param level Zero base level.
    * @param row Zero based tile row index.
    * @param col Zero based tile column index.
    */
   bool isValidZoomLevelRowCol( ossim_int32 level,
                                ossim_int32 row,
                                ossim_int32 col  ) const;

   /**
    * @brief Get the current gsd from projection.  Computes the scale, and
    * calls proj->applyScale(...) recentering tie point.
    * @param proj
    * @param desiredGsd Desired gsd after scale change.
    */
   void applyScaleToProjection( ossimMapProjection* proj,
                                const ossimDpt& desiredGsd ) const;

   /** database connection */
   sqlite3* m_db;

   /** Working variable for holding the current batch count */
   ossim_uint64 m_batchCount;

   /** Number of transactions batched before being executed. */
   ossim_uint64 m_batchSize;   

   /**
    * Holds the bounding rect of the output projection edges either in decimal
    * degrees for geographic projection or Easting/Northings(meters) for map
    * projection.  This is "edge to edge" bounds.
    */
   ossimDrect m_projectionBoundingRect;

   /**
    * Holds the bounding rect of the scene edges either in decimal degrees
    * for geographic projection or Easting/Northings(meters) for map projection.
    * or Easting/Northings(meters).  This is "edge to edge" bounds.
    */
   ossimDrect m_sceneBoundingRect;

   /** AOI clipped to projection rect. This is "edge to edge" bounds. */ 
   ossimDrect m_clipRect; 

   /**
    * Expanded(final) AOI clipped to projection rect.
    * This is "edge to edge" bounds.
    */
   ossimDrect m_outputRect;

   ossimIpt m_tileSize;

   std::string m_tileTableName;

   ossim_int32 m_srs_id;

   /** Hold all options. */
   ossimRefPtr<ossimKeywordlist> m_kwl;

   /**
    * Will cache and hold the allocated codecs to use for the encoding.
    * There is a full and partial as in mixed mode you could use jpeg for full
    * tiles and png with alpha for partial (edge) tiles.
    */
   ossimRefPtr<ossimCodecBase> m_fullTileCodec;
   ossimRefPtr<ossimCodecBase> m_partialTileCodec;

   /** true if codec requires alpha channel. */
   bool m_fullTileCodecAlpha;
   bool m_partialTileCodecAlpha;

   /** Holds zoom level indexes for connectionless write tile. */
   std::vector<ossim_int32> m_zoomLevels;

   /** Hold zoom level matrix sizes for connectionless write tile. */
   std::vector<ossimIpt> m_zoomLevelMatrixSizes;

   /**
    * Holds Statement handle from sqlite3_prepare_v2(...) for connectionless
    * write tile.
    */
   sqlite3_stmt* m_pStmt;

   /** Controlled by option key: "include_blank_tiles" */
   bool m_writeBlanks;

   TYPE_DATA
};

#endif /* #ifndef ossimGpkgWriter_HEADER */
