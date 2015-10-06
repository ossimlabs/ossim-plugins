//----------------------------------------------------------------------------
//
// File: ossimGpkgNsgTileMatrixExtentRecord.h
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: Container class for nsg_tile_matrix_extent table.
//
//----------------------------------------------------------------------------
// $Id$

#ifndef ossimGpkgNsgTileMatrixExtentRecord_HEADER
#define ossimGpkgNsgTileMatrixExtentRecord_HEADER 1

#include "ossimGpkgDatabaseRecordBase.h"
#include <ossim/base/ossimConstants.h>

class ossimIrect;
class ossimDrect;
struct sqlite3;

class ossimGpkgNsgTileMatrixExtentRecord : public ossimGpkgDatabaseRecordBase
{
public:

   /** @brief default constructor */
   ossimGpkgNsgTileMatrixExtentRecord();

   /* @brief copy constructor */
   ossimGpkgNsgTileMatrixExtentRecord(const ossimGpkgNsgTileMatrixExtentRecord& obj);

   /* @brief assignment operator= */
   const ossimGpkgNsgTileMatrixExtentRecord& operator=
      (const ossimGpkgNsgTileMatrixExtentRecord& obj);

   /** @brief destructor */
   virtual ~ossimGpkgNsgTileMatrixExtentRecord();

   /**
    * @brief Get the table name "nsg_tile_matrix_extent".
    * @return table name
    */
   static const std::string& getTableName();

   /**
    * @brief Initialize from database.
    * @param pStmt SQL statement, i.e. result of sqlite3_prepare_v2(...) call.
    */
   virtual bool init( sqlite3_stmt* pStmt );

   /**
    * @brief Initialize from projection.
    * @param tableName e.g. "tiles"
    * @param zoom_level
    * @param imageRect 
    * @param projectionRect Edge to edge either Easting Northin or lat lon.
    * @return true on success, false on error.
    */
   bool init( const std::string& tableName,
              ossim_int32 zoom_level,
              const ossimIrect& imageRect,
              const ossimDrect& projectionRect );
   
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
   
   std::string   m_table_name;
   ossim_int32   m_zoom_level;
   
   /** “complete”,”missing”,“present”,”absent”,”mixed” */
   std::string   m_extent_type;

   ossim_int32   m_min_column; 
   ossim_int32   m_min_row;  
   ossim_int32   m_max_column; 
   ossim_int32   m_max_row;  
   ossim_float64 m_min_x;
   ossim_float64 m_min_y;
   ossim_float64 m_max_x;
   ossim_float64 m_max_y;
};

#endif /* #ifndef ossimGpkgNsgTileMatrixExtentRecord_HEADER */
