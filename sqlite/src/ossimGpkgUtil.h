//----------------------------------------------------------------------------
// File: ossimGpkgUtil.h
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: OSSIM GeoPackage utility class.
//
//----------------------------------------------------------------------------
// $Id

#ifndef ossimGpkgUtil_HEADER
#define ossimGpkgUtil_HEADER 1

#include <ossim/base/ossimRefPtr.h>
#include "ossimGpkgDatabaseRecordBase.h"
#include <iosfwd>
#include <vector>

class ossimGpkgContentsRecord;
class ossimGpkgSpatialRefSysRecord;
class ossimGpkgTileEntry;
class ossimGpkgTileMatrixRecord;
class ossimGpkgTileMatrixSetRecord;

struct sqlite3;

namespace ossim_gpkg
{
   /**
    * @brief Check signature method.
    * @return true if requirement 1 and 2 are met in the file; false, if not.
    */
   bool checkSignature(std::istream& in);

   /**
    * @brief Check application_id.
    * @return true if requirement 2 are met in the file; false, if not.
    */
   bool checkApplicationId(std::istream& in);

   /**
    * @brief Gets all the tile entries.
    * @param db An open database.
    * @param entries Initialized by this.
    */
   void getTileEntries( sqlite3* db,
                        std::vector<ossimGpkgTileEntry>& entries );
   /**
    * @brief Gets tile entry whos table_name field matches tileTableName.
    * @param db An open database.
    * @param table_name e.g. "tiles"
    * @param entries Initialized by this.
    */
   bool getTileEntry( sqlite3* db,
                      const std::string& tileTableName,
                      ossimGpkgTileEntry& entry );   

   /**
    * @brief Get gpkg records.
    *
    * class T must be derived from ossimGpkgDatabaseRecordBase.
    * @param db An open database.
    * @param result Initialized by this.
    * @param dbTableName E.g. "gpkg_contents".
    */
   template <class T> void getGpkgRecords( sqlite3* db,
                                           std::vector<T>& result,
                                           const std::string& dbTableName );

   /**
    * @brief Get gpkg records.
    *
    * class T must be derived from ossimGpkgDatabaseRecordBase.
    * @param db An open database.
    * @param result Initialized by this.
    * @param dbTableName E.g. "gpkg_contents".
    * @param table_name e.g. "tiles"
    */
   template <class T> void getGpkgRecords( sqlite3* db,
                                           std::vector<T>& result,
                                           const std::string& dbTableName,
                                           const std::string& table_name  );

   /**
    * @brief Get gpkg record.
    *
    * class T must be derived from ossimGpkgDatabaseRecordBase.
    * @param db An open database.
    * @param result Initialized by this.
    * @param dbTableName e.g. "gpkg_contents".
    * @param table_name e.g. "tiles"
    * @return true on success, false on error.
    */
   template <class T> bool getGpkgRecord( sqlite3* db,
                                          T& result,
                                          const std::string& dbTableName,
                                          const std::string& table_name );


   /**
    * @brief Get gpkg_spatial_ref_sys record for srs_it.
    *
    * @param db An open database.
    * @param srs_id ID to look for.
    * @param srs Initialized by this.
    * @return true on success, false on error.
    */

   bool getSrsRecord( sqlite3* db,
                      ossim_int32 srs_id,
                      ossimGpkgSpatialRefSysRecord& srs );   

   
   /**
    * @brief Parse table rows
    * @param db An open database.
    * @param tableName e.g. "gpkg_contents"
    * @param result Initialized by this with records.
    * @return true on success, false on error.
    */
   bool getTableRows(
      sqlite3* db,
      const std::string& tableName,
      std::vector< ossimRefPtr<ossimGpkgDatabaseRecordBase> >& result );

   /**
    * @brief Parse gpkg_spatial_ref_sys tables.
    * @param db An open database.
    * @param result Initialized by this with records.
    * @return true on success, false on error.
    */
   // bool parseGpkgSpatialRefSysTables(
   //    sqlite3* db, std::vector< ossimRefPtr<ossimGpkgSpatialRefSysRecord> >& result );

   /**
    * @brief Parse gpkg_tile_matrix_set tables.
    * @param db An open database.
    * @param result Initialized by this with records.
    * @return true on success, false on error.
    */
   // bool parseGpkgTileMatrixSetTables(
   //    sqlite3* db, std::vector< ossimRefPtr<ossimGpkgTileMatrixSetRecord> >& result );


   /**
    * @brief Parse gpkg_tile_matrix tables.
    * @param db An open database.
    * @param result Initialized by this with records.
    * @return true on success, false on error.
    */
   // bool parseGpkgTileMatrixTables(
   //    sqlite3* db, std::vector< ossimRefPtr<ossimGpkgTileMatrixRecord> >& result );

   /**
    * @brief Gets table record from table name.
    * @return Intance of record wrapped in ref ptr.  Contents can be null
    * if talbeName is not known.
    */
   ossimRefPtr<ossimGpkgDatabaseRecordBase> getNewTableRecord(
      const std::string& tableName );

   std::ostream& printTiles(sqlite3* db, const std::string& tileTableName, std::ostream& out);
}

#endif /* #ifndef ossimGpkgUtil_HEADER */
