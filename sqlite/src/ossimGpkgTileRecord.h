//----------------------------------------------------------------------------
//
// File: ossimGpkgTileRecord.h
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: Container class for GeoPackage tile record.
//
//----------------------------------------------------------------------------
// $Id$

#ifndef ossimGpkgTileRecord_HEADER
#define ossimonGpkgTileRecord_HEADER 1

#include <ossim/base/ossimConstants.h>
#include <iosfwd>
#include <vector>

class ossimIpt;
struct sqlite3_stmt;
struct sqlite3;

class ossimGpkgTileRecord
{
public:

   enum ossimGpkgTileType
   {
      OSSIM_GPKG_UNKNOWN = 0,
      OSSIM_GPKG_JPEG    = 1,
      OSSIM_GPKG_PNG     = 2
   };

   /** @brief default constructor */
   ossimGpkgTileRecord();

   /* @brief copy constructor */
   ossimGpkgTileRecord(const ossimGpkgTileRecord& obj);

   /* @brief assignment operator= */
   const ossimGpkgTileRecord& operator=
      (const ossimGpkgTileRecord& obj);

   /** @brief destructor */
   ~ossimGpkgTileRecord();

   /**
    * @brief Initialize from database.
    * @param pStmt SQL statement, i.e. result of sqlite3_prepare_v2(...) call. 
    */
   bool init( sqlite3_stmt* pStmt );

   /**
    * @brief Creates  table in database.
    * @param db
    * @param tableName e.g. "tiles", "map_tiles"
    * @return true on success, false on error.
    */   
   static bool createTable( sqlite3* db, const std::string& tableName );  

   void setCopyTileFlag( bool flag );

   /**
    * @brief Print method.
    *
    * Does not print the image blob.
    * 
    * @param out Stream to print to.
    * @return Stream reference.
    */
   std::ostream& print(std::ostream& out) const;
   
   /**
    * @brief Convenience operator << function.
    * @param out Stream to print to.
    * @param obj Object to print.
    */
   friend std::ostream& operator<<(std::ostream& out,
                                   const ossimGpkgTileRecord& obj);

   /**
    * @brief Get tile index.
    * @param size Initializes with tile_column and tile_row.
    */
   void getTileIndex( ossimIpt& index ) const;

   /** @return Tile type from signature block. */
   ossimGpkgTileType getTileType() const;

   /** @return Tile media type from signature block. e.g. "image/png" */
   std::string getTileMediaType() const;

   ossim_int32 m_id;
   ossim_int32 m_zoom_level;
   ossim_int32 m_tile_column;
   ossim_int32 m_tile_row;
   std::vector<ossim_uint8> m_tile_data;
   bool m_copy_tile_flag;
};

#endif /* #ifndef ossimGpkgTileRecord_HEADER */
