//----------------------------------------------------------------------------
// File: ossimGpkgUtil.cpp
// 
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: OSSIM GeoPackage utility class.
//----------------------------------------------------------------------------
// $Id$

#include "ossimGpkgUtil.h"
#include "ossimGpkgContentsRecord.h"
#include "ossimGpkgDatabaseRecordBase.h"
#include "ossimGpkgSpatialRefSysRecord.h"
#include "ossimGpkgTileEntry.h"
#include "ossimGpkgTileRecord.h"
#include "ossimGpkgTileMatrixRecord.h"
#include "ossimGpkgTileMatrixSetRecord.h"
#include "ossimGpkgNsgTileMatrixExtentRecord.h"

#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimNotify.h>

#include <sqlite3.h>
#include <fstream>
#include <ostream>
#include <sstream>

bool ossim_gpkg::checkSignature(std::istream& in)
{
   //---
   // First 15 bytes should be: "SQLite format 3"
   // Note the spec says 16 bytes but it doesn't add up.
   // Samples have '.' at end.
   //---
   bool result = false;
   char SIG[15];
   in.read(SIG, 15);
   if ( (SIG[0] == 'S') && (SIG[1] == 'Q') && (SIG[2] == 'L') && (SIG[3] == 'i') &&
        (SIG[4] == 't') && (SIG[5] == 'e') && (SIG[6] == ' ') && (SIG[7] == 'f') &&
        (SIG[8] == 'o') && (SIG[9] == 'r') && (SIG[10] == 'm') && (SIG[11] == 'a') &&
        (SIG[12] == 't') && (SIG[13] == ' ') && (SIG[14] == '3') )   
   {
      result = true;
   }
   return result;
}

bool ossim_gpkg::checkApplicationId(std::istream& in)
{
   //---
   // Check the application_id:
   // Requirement 2: Every GeoPackage must contain 0x47503130 ("GP10" in ACII)
   // in the application id field of the SQLite database header to indicate a
   // GeoPackage version 1.0 file.
   //---
   bool result = false;
   char APP_ID[4];
   in.seekg( 68, std::ios_base::beg );
   in.read(APP_ID, 4);
   if ( (APP_ID[0] == 'G') && (APP_ID[1] == 'P') && (APP_ID[2] == '1') && (APP_ID[3] == '0') )
   {
      result = true;
   }
   return result;
}

void ossim_gpkg::getTileEntries( sqlite3* db, std::vector<ossimGpkgTileEntry>& entries )
{
   if ( db )
   {
      // Get all the tile matrix sets.  Each set can be concidered an entry.
      
      std::vector<ossimGpkgTileMatrixSetRecord> sets;
      ossim_gpkg::getGpkgRecords(
         db, sets, ossimGpkgTileMatrixSetRecord::getTableName() );

      if ( sets.size() )
      {
         // Get all the tile matrix rows.
         std::vector<ossimGpkgTileMatrixRecord> levels;
         ossim_gpkg::getGpkgRecords(
            db, levels, ossimGpkgTileMatrixRecord::getTableName() );

          // Get all the nsg tile matrix extent rows.
         std::vector<ossimGpkgNsgTileMatrixExtentRecord> extents;
         ossim_gpkg::getGpkgRecords(
            db, extents, ossimGpkgNsgTileMatrixExtentRecord::getTableName() );

         // Get all the srs rows.
         std::vector<ossimGpkgSpatialRefSysRecord> srs;
         ossim_gpkg::getGpkgRecords(
            db, srs, ossimGpkgSpatialRefSysRecord::getTableName() );

         // For each entry captue the tile matrix and srs that belong to entry.
         std::vector<ossimGpkgTileMatrixSetRecord>::const_iterator setIdx = sets.begin();
         while ( setIdx != sets.end() )
         {
            ossimGpkgTileEntry entry;
            entry.setTileMatrixSet(*setIdx);

            // Add tile matrix objects to entry if table_name matches.
            std::vector<ossimGpkgTileMatrixRecord>::const_iterator mIdx = levels.begin();
            while ( mIdx != levels.end() )
            {
               if ( entry.getTileMatrixSet().m_table_name == (*mIdx).m_table_name )
               {
                  // table_name matches...
                  entry.addTileMatrix( (*mIdx) );
               }
               ++mIdx;
            }

            // Add tile matrix extent objects to entry if table_name matches.
            std::vector<ossimGpkgNsgTileMatrixExtentRecord>::const_iterator extIdx = extents.begin();
            while ( extIdx != extents.end() )
            {
               if ( entry.getTileMatrixSet().m_table_name == (*extIdx).m_table_name )
               {
                  // table_name matches...
                  entry.addTileMatrixExtent( (*extIdx) );
               }
               ++extIdx;
            }

            std::vector<ossimGpkgSpatialRefSysRecord>::const_iterator srsIdx = srs.begin();
            while ( srsIdx != srs.end() )
            {
               if (entry.getTileMatrixSet().m_srs_id == (*srsIdx).m_srs_id)
               {
                  // srs id matches...
                  entry.setSrs( (*srsIdx) );
                  break;
               }
               ++srsIdx;
            }

            if ( entry.getTileMatrix().size() )
            {
               // The sort call puts the tile matrix entries in highest zoom to lowest order.
               entry.sortTileMatrix();
               entry.sortTileMatrixExtents();
               
               // Add the entry
               entries.push_back( entry );
            }
            else
            {
               ossimNotify(ossimNotifyLevel_WARN)
                  << "ossim_gpkg::getTileEntries WARNING No levels found for entry!"
                  << std::endl;
            }

            ++setIdx; // Next entry.
         }
      } 
      
   } // Matches: if ( sqlite3* db )
   
} // End: ossim_gpkg::getTileEntries( ... )

bool ossim_gpkg::getTileEntry( sqlite3* db,
                               const std::string& tileTableName,
                               ossimGpkgTileEntry& entry )
{
   bool status = false;
   
   if ( db )
   {
      // Get all the tile matrix set for the tile table name.
      ossimGpkgTileMatrixSetRecord set;
      if ( ossim_gpkg::getGpkgRecord(
              db, set, ossimGpkgTileMatrixSetRecord::getTableName(), tileTableName ) )
      {
         entry.setTileMatrixSet( set );

         // Get all the tile matrix rows.  There's one for each level.
         std::vector<ossimGpkgTileMatrixRecord> levels;
         ossim_gpkg::getGpkgRecords(
            db, levels, ossimGpkgTileMatrixRecord::getTableName(), tileTableName );

         if ( levels.size() )
         {
            // Add tile matrix objects to entry if table_name matches.
            std::vector<ossimGpkgTileMatrixRecord>::const_iterator mIdx = levels.begin();
            while ( mIdx != levels.end() )
            {
               entry.addTileMatrix( (*mIdx) );
               ++mIdx;
            }
            
            // Get the GpkgSpatialRefSys.  Required or we don't go on.
            ossimGpkgSpatialRefSysRecord srs;
            if ( ossim_gpkg::getSrsRecord(
                    db,
                    set.m_srs_id,
                    srs ) )
            {
               entry.setSrs( srs );

               //---
               // At this point we can set the status to true.  The below nsg
               // tile matrix extent is not required.
               //---
               status = true;

               // Add tile matrix extent objects to entry if table_name matches.

               // Get all the nsg tile matrix extent rows.
               std::vector<ossimGpkgNsgTileMatrixExtentRecord> extents;
               ossim_gpkg::getGpkgRecords(
                  db,
                  extents,
                  ossimGpkgNsgTileMatrixExtentRecord::getTableName(),
                  tileTableName );
               std::vector<ossimGpkgNsgTileMatrixExtentRecord>::const_iterator extIdx =
                  extents.begin();
               while ( extIdx != extents.end() )
               {
                  if ( entry.getTileMatrixSet().m_table_name == (*extIdx).m_table_name )
                  {
                     // table_name matches...
                     entry.addTileMatrixExtent( (*extIdx) );
                  }
                  ++extIdx;
               }

               if ( entry.getTileMatrix().size() )
               {
                  // The sort call puts the tile matrix entries in highest zoom to lowest order.
                  entry.sortTileMatrix();
               }
               if ( entry.getTileMatrixExtent().size() )
               {
                  // The sort call puts the tile matrix entries in highest zoom to lowest order.
                  entry.sortTileMatrixExtents();

                  // If we have extents, the size should match the tile matrix size.
                  if ( entry.getTileMatrixExtent().size() != entry.getTileMatrix().size() )
                  {
                     ossimNotify(ossimNotifyLevel_WARN)
                        << "ossim_gpkg::getTileEntry WARNING size mismatch between tile matrix"
                        << " and tile matrix extents!\n";
                  }
               }
            }
            else
            {
               ossimNotify(ossimNotifyLevel_WARN)
                  << "ossim_gpkg::getTileEntry WARNING No gpkg_spatial_ref_sys record found for"
                  << " entry!\n";
            }
         }
         else
         {
            ossimNotify(ossimNotifyLevel_WARN)
               << "ossim_gpkg::getTileEntry WARNING No gpkg_tile_matrix records found for entry!"
               << std::endl;
         }
      } 
      
   } // Matches: if ( sqlite3* db )

   return status;
   
} // End: ossim_gpkg::getTileEntry( ... )

template <class T> void ossim_gpkg::getGpkgRecords( sqlite3* db,
                                                    std::vector<T>& result,
                                                    const std::string& dbTableName )
{
   if ( db && dbTableName.size() )
   {
      const char *zLeftover;      /* Tail of unprocessed SQL */
      sqlite3_stmt *pStmt = 0;    /* The current SQL statement */
      std::string sql = "SELECT * from ";
      sql += dbTableName;
      
      int rc = sqlite3_prepare_v2(db,           // Database handle
                                  sql.c_str(),  // SQL statement, UTF-8 encoded
                                  -1,           // Maximum length of zSql in bytes.
                                  &pStmt,       // OUT: Statement handle
                                  &zLeftover);  // OUT: Pointer to unused portion of zSql
      if ( rc == SQLITE_OK )
      {
         while( 1 )
         {
            // Read the row:
            rc = sqlite3_step(pStmt);
            if ( rc == SQLITE_ROW )
            {
               T record;
               ossimGpkgDatabaseRecordBase* gpkgRecPtr =
                  dynamic_cast<ossimGpkgDatabaseRecordBase*>( &record );
               if ( gpkgRecPtr )
               {
                  if ( gpkgRecPtr->init( pStmt ) )
                  {
                     result.push_back(record);
                  }
                  else
                  {
                     ossimNotify(ossimNotifyLevel_WARN)
                        << "ossim_gpkg::getGpkgRecords init failed!"
                        << std::endl;
                     break;
                  }
               }
               else
               {
                  break;
               }
            }
            else
            {
               break;
            }
         }
      }
      sqlite3_finalize(pStmt);
   }
   
} // End: ossim_gpkg::getGpkgRecords( db, result, dbTableName )

template <class T> void ossim_gpkg::getGpkgRecords( sqlite3* db,
                                                    std::vector<T>& result,
                                                    const std::string& dbTableName,
                                                    const std::string& table_name )
{
   if ( db && dbTableName.size() && table_name.size() )
   {
      const char *zLeftover;      /* Tail of unprocessed SQL */
      sqlite3_stmt *pStmt = 0;    /* The current SQL statement */
      std::ostringstream sql;
      sql << "SELECT * from " << dbTableName
          << " WHERE table_name == '" << table_name << "'";
      
      int rc = sqlite3_prepare_v2(db,                 // Database handle
                                  sql.str().c_str(),  // SQL statement, UTF-8 encoded
                                  -1,                 // Maximum length of zSql in bytes.
                                  &pStmt,             // OUT: Statement handle
                                  &zLeftover);        // OUT: Pointer to unused portion of zSql
      if ( rc == SQLITE_OK )
      {
         while( 1 )
         {
            // Read the row:
            rc = sqlite3_step(pStmt);
            if ( rc == SQLITE_ROW )
            {
               T record;
               ossimGpkgDatabaseRecordBase* gpkgRecPtr =
                  dynamic_cast<ossimGpkgDatabaseRecordBase*>( &record );
               if ( gpkgRecPtr )
               {
                  if ( gpkgRecPtr->init( pStmt ) )
                  {
                     result.push_back(record);
                  }
                  else
                  {
                     ossimNotify(ossimNotifyLevel_WARN)
                        << "ossim_gpkg::getGpkgRecords init failed!"
                        << std::endl;
                     break;
                  }
               }
               else
               {
                  break;
               }
            }
            else
            {
               break;
            }
         }
      }
      sqlite3_finalize(pStmt);
   }
   
} // End: ossim_gpkg::getGpkgRecords( db, result, dbTableName, table_name )

template <class T> bool ossim_gpkg::getGpkgRecord( sqlite3* db,
                                                   T& result,
                                                   const std::string& dbTableName,
                                                   const std::string& table_name )
{
   bool status = false;
   
   if ( db && dbTableName.size() && table_name.size() )
   {
      const char *zLeftover;      /* Tail of unprocessed SQL */
      sqlite3_stmt *pStmt = 0;    /* The current SQL statement */
      std::ostringstream sql;
      sql << "SELECT * from " << dbTableName
          << " WHERE table_name == '" << table_name << "'";

      int rc = sqlite3_prepare_v2(db,                // Database handle
                                  sql.str().c_str(), // SQL statement, UTF-8 encoded
                                  -1,                // Maximum length of zSql in bytes.
                                  &pStmt,            // OUT: Statement handle
                                  &zLeftover);       // OUT: Pointer to unused portion of zSql
      if ( rc == SQLITE_OK )
      {
         // Read the row:
         rc = sqlite3_step(pStmt);
         
         if ( rc == SQLITE_ROW )
         {
            ossimGpkgDatabaseRecordBase* gpkgRecPtr =
               dynamic_cast<ossimGpkgDatabaseRecordBase*>( &result );
            if ( gpkgRecPtr )
            {
               status = gpkgRecPtr->init( pStmt );
               if ( !status )
               {
                  ossimNotify(ossimNotifyLevel_WARN)
                     << "ossim_gpkg::getGpkgRecord init failed!"
                     << std::endl;
               }
            }
         }
      }
      sqlite3_finalize(pStmt);
   }

   return status;
   
} // End: ossim_gpkg::getGpkgRecord( ... )


bool ossim_gpkg::getSrsRecord( sqlite3* db,
                               ossim_int32 srs_id,
                               ossimGpkgSpatialRefSysRecord& srs )
{
   bool status = false;
   
   if ( db )
   {
      const char *zLeftover;      /* Tail of unprocessed SQL */
      sqlite3_stmt *pStmt = 0;    /* The current SQL statement */
      std::ostringstream sql;
      sql << "SELECT * from " << srs.getTableName()
          << " WHERE srs_id == " << srs_id;
      
      int rc = sqlite3_prepare_v2(db,                // Database handle
                                  sql.str().c_str(), // SQL statement, UTF-8 encoded
                                  -1,                // Maximum length of zSql in bytes.
                                  &pStmt,            // OUT: Statement handle
                                  &zLeftover);       // OUT: Pointer to unused portion of zSql
      if ( rc == SQLITE_OK )
      {
         // Read the row:
         rc = sqlite3_step(pStmt);
         if ( rc == SQLITE_ROW )
         {
            ossimGpkgDatabaseRecordBase* gpkgRecPtr =
               dynamic_cast<ossimGpkgDatabaseRecordBase*>( &srs );
            if ( gpkgRecPtr )
            {
               status = gpkgRecPtr->init( pStmt );
               if ( !status )
               {
                  ossimNotify(ossimNotifyLevel_WARN)
                     << "ossim_gpkg::getSrsRecord init failed!"
                     << std::endl;
               }
            }
         }
      }
      sqlite3_finalize(pStmt);
   }

   return status;
   
} // End: ossim_gpkg::getSrsRecord( ... )

bool ossim_gpkg::getTableRows(
   sqlite3* db,
   const std::string& tableName,
   std::vector< ossimRefPtr<ossimGpkgDatabaseRecordBase> >& result )
{
   static const char M[] = "ossim_gpkg::getTableRows";

   bool status = false;
   
   if ( db && tableName.size() )
   {
      const char *zLeftover;      /* Tail of unprocessed SQL */
      sqlite3_stmt *pStmt = 0;    /* The current SQL statement */
      std::string sql = "SELECT * from ";
      sql += tableName;

      int rc = sqlite3_prepare_v2(db,           // Database handle
                                  sql.c_str(),  // SQL statement, UTF-8 encoded
                                  -1,           // Maximum length of zSql in bytes.
                                  &pStmt,       // OUT: Statement handle
                                  &zLeftover);  // OUT: Pointer to unused portion of zSql
      if ( rc == SQLITE_OK )
      {
         bool initStatus = true;
         
         int nCol = sqlite3_column_count( pStmt );
         if ( nCol )
         {
            while( 1 )
            {
               // Read the row:
               rc = sqlite3_step(pStmt);
               if ( rc == SQLITE_ROW )
               {
                  ossimRefPtr<ossimGpkgDatabaseRecordBase> row = getNewTableRecord( tableName );
                  if ( row.valid() )
                  {
                     if ( row->init( pStmt ) )
                     {
                        result.push_back(row);
                     }
                     else
                     {
                        ossimNotify(ossimNotifyLevel_WARN)
                           << M << " init failed!" << std::endl;
                        initStatus = false;
                        break;
                     }
                  }
                  else
                  {
                     ossimNotify(ossimNotifyLevel_WARN)
                        << M << " could not make object for table name: " << tableName
                        << std::endl;
                     initStatus = false;
                     break; 
                  }
               }
               else
               {
                  break;
               }
            }
         }
         if ( initStatus && result.size() )
         {
            status = true;
         }  
      }
      sqlite3_finalize(pStmt);
   }
   
   return status;
   
} // End: ossim_gpks::getTableRows(...)

ossimRefPtr<ossimGpkgDatabaseRecordBase> ossim_gpkg::getNewTableRecord(
   const std::string& tableName )
{
   ossimRefPtr<ossimGpkgDatabaseRecordBase> result = 0;
   if ( tableName == ossimGpkgTileMatrixRecord::getTableName() )
   {
      result = new ossimGpkgTileMatrixRecord();
   }
   else if ( tableName == ossimGpkgTileMatrixSetRecord::getTableName() )
   {
      result = new ossimGpkgTileMatrixSetRecord();
   }
   else if ( tableName == ossimGpkgSpatialRefSysRecord::getTableName() )
   {
      result = new ossimGpkgSpatialRefSysRecord();
   }
   else if ( tableName == ossimGpkgContentsRecord::getTableName() )
   {
      result = new ossimGpkgContentsRecord();
   }
   else if ( tableName == ossimGpkgNsgTileMatrixExtentRecord::getTableName() )
   {
      result = new ossimGpkgNsgTileMatrixExtentRecord();
   }

   return result;
}

std::ostream& ossim_gpkg::printTiles(sqlite3* db, const std::string& tileTableName, std::ostream& out)
{
   if ( db )
   {
      const char *zLeftover;      /* Tail of unprocessed SQL */
      sqlite3_stmt *pStmt = 0;    /* The current SQL statement */
      std::string sql = "SELECT * from ";
      sql += tileTableName;
      
      int rc = sqlite3_prepare_v2(db,           // Database handle
                                  sql.c_str(),  // SQL statement, UTF-8 encoded
                                  -1,           // Maximum length of zSql in bytes.
                                  &pStmt,       // OUT: Statement handle
                                  &zLeftover);  // OUT: Pointer to unused portion of zSql
      if ( rc == SQLITE_OK )
      {
         int nCol = sqlite3_column_count( pStmt );
         if ( nCol )
         {
            ossimGpkgTileRecord tile;
            tile.setCopyTileFlag(false);
            while( 1 )
            {
               // Read the row:
               rc = sqlite3_step(pStmt);
               if ( rc == SQLITE_ROW )
               {
                  if (tile.init( pStmt ) )
                  {
                     out << tile << std::endl;
                  }     
               }
               else
               {
                  break;
               }
            }
         }
      }
      sqlite3_finalize(pStmt);
   }
   return out;
}
