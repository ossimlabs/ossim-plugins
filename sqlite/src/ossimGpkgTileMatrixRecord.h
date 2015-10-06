//----------------------------------------------------------------------------
//
// File: ossimGpkgTileMatrixRecord.h
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: Container class for gpkg_tile_matrix table.
//
//----------------------------------------------------------------------------
// $Id$

#ifndef ossimGpkgTileMatrixRecord_HEADER
#define ossimGpkgTileMatrixRecord_HEADER 1

#include "ossimGpkgDatabaseRecordBase.h"
#include <ossim/base/ossimConstants.h>

class ossimDpt;
class ossimIpt;
struct sqlite3;

class ossimGpkgTileMatrixRecord : public ossimGpkgDatabaseRecordBase
{
public:

   /** @brief default constructor */
   ossimGpkgTileMatrixRecord();

   /* @brief copy constructor */
   ossimGpkgTileMatrixRecord(const ossimGpkgTileMatrixRecord& obj);

   /* @brief assignment operator= */
   const ossimGpkgTileMatrixRecord& operator=
      (const ossimGpkgTileMatrixRecord& obj);

   /** @brief destructor */
   virtual ~ossimGpkgTileMatrixRecord();

   /**
    * @brief Get the table name "gpkg_tile_matrix".
    * @return table name
    */
   static const std::string& getTableName();
   
   /**
    * @brief Initialize from database.
    * @param pStmt SQL statement, i.e. result of sqlite3_prepare_v2(...) call.
    */
   virtual bool init( sqlite3_stmt* pStmt );
   
   /**
    * @brief Initialize method.
    * @param tableName e.g. "tiles"
    * @param Zoom level.  Zero being whole Earth...
    * @param matrixSize Size of tile matrix, i.e. number of horizontal
    * vertical tiles.
    * @param tileSize Size of one tile, e.g. 256 x 256.
    * @param gsd Size of one pixel either in meters or lat lon.
    * @return true on success, false on error.
    */
   bool init( const std::string& tableName,
              ossim_int32 zoom_level,
              const ossimIpt& matrixSize,
              const ossimIpt& tileSize,
              const ossimDpt& gsd );
   
   /**
    * @brief Creates  table in database.
    * @param db
    * @return true on success, false on error.
    */   
   static bool createTable( sqlite3* db );

   /**
    * @brief Inserst this record into gpkg_spatial_ref_sys table.
    * @param db
    * @return true on success, false on error.
    */   
   bool insert( sqlite3* db );
   
   /**
    * @brief Saves the state of object.
    * @param kwl Initialized by this.
    * @param prefix e.g. "image0.". Can be empty.
    */
   virtual void saveState( ossimKeywordlist& kwl,
                           const std::string& prefix ) const;

   /**
    * @brief Get matrix size. 
    * @param size Initialized with matrix_width and matrix_height.
    */
   void getMatrixSize( ossimIpt& size ) const;

   /**
    * @brief Get tile size.
    * @param size Initializes with tile_width and tile_height.
    */
   void getTileSize( ossimIpt& size ) const; 

   /**
    * @brief Get gsd.
    * @param gsd Initializes with pixel_x_size and pixel_y_size. */
   void getGsd( ossimDpt& gsd ) const;
   
   std::string   m_table_name;
   ossim_int32   m_zoom_level;
   ossim_int32   m_matrix_width;
   ossim_int32   m_matrix_height;   
   ossim_int32   m_tile_width;
   ossim_int32   m_tile_height;   
   ossim_float64 m_pixel_x_size;
   ossim_float64 m_pixel_y_size;
};

#endif /* #ifndef ossimGpkgTileMatrixRecord_HEADER */
