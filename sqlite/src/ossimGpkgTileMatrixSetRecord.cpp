//----------------------------------------------------------------------------
//
// File: ossimGpkgTileMatrixSetRecord.cpp
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: Container class for gpkg_tile_matrix_set table.
//
//----------------------------------------------------------------------------
// $Id$

#include "ossimGpkgTileMatrixSetRecord.h"
#include "ossimSqliteUtil.h"
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimDrect.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimNotify.h>
#include <sqlite3.h>
#include <iomanip>

static const std::string TABLE_NAME = "gpkg_tile_matrix_set";

ossimGpkgTileMatrixSetRecord::ossimGpkgTileMatrixSetRecord()
   :
   ossimGpkgDatabaseRecordBase(),   
   m_table_name(),
   m_srs_id(0),
   m_min_x(ossim::nan()),
   m_min_y(ossim::nan()),
   m_max_x(ossim::nan()),
   m_max_y(ossim::nan())
{
}

ossimGpkgTileMatrixSetRecord::ossimGpkgTileMatrixSetRecord(const ossimGpkgTileMatrixSetRecord& obj)
   :
   ossimGpkgDatabaseRecordBase(),
   m_table_name(obj.m_table_name),
   m_srs_id(obj.m_srs_id),
   m_min_x(obj.m_min_x),
   m_min_y(obj.m_min_y),
   m_max_x(obj.m_max_x),
   m_max_y(obj.m_max_y)
{
}

const ossimGpkgTileMatrixSetRecord& ossimGpkgTileMatrixSetRecord::operator=(
   const ossimGpkgTileMatrixSetRecord& obj)
{
   if ( this != &obj )
   {
      m_table_name = obj.m_table_name;
      m_srs_id = obj.m_srs_id;
      m_min_x = obj.m_min_x;
      m_min_y = obj.m_min_y;
      m_max_x = obj.m_max_x;
      m_max_y = obj.m_max_y;
   }
   return *this;
}

ossimGpkgTileMatrixSetRecord::~ossimGpkgTileMatrixSetRecord()
{
}

const std::string& ossimGpkgTileMatrixSetRecord::getTableName()
{
   return TABLE_NAME;
}

bool ossimGpkgTileMatrixSetRecord::init( sqlite3_stmt* pStmt )
{
   static const char M[] = "ossimGpkgTileMatrixSetRecord::init";
   
   bool status = false;

   if ( pStmt )
   {
      const ossim_int32 EXPECTED_COLUMNS = 6;
      ossim_int32 nCol = sqlite3_column_count( pStmt );

      if ( nCol != EXPECTED_COLUMNS )
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << M << " WARNING:\nUnexpected number of columns: " << nCol
            << "Expected column count: " << EXPECTED_COLUMNS
            << std::endl;
      }
      
      if ( nCol >= EXPECTED_COLUMNS )
      {
         ossim_int32 columnsFound = 0;
         std::string colName;
         const char* c = 0; // To catch null so not to pass to string.

         for ( ossim_int32 i = 0; i < nCol; ++i )
         {
            colName = sqlite3_column_name(pStmt, i);
            if ( colName.size() )
            {
               if ( colName == "table_name" )
               {
                  c = (const char*)sqlite3_column_text(pStmt, i);
                  m_table_name = (c ? c : "");
                  ++columnsFound;
               }
               else if ( colName == "srs_id" )
               {
                  m_srs_id = sqlite3_column_int(pStmt, i);
                  ++columnsFound;
               }
               else if ( colName == "min_x" )
               {
                  m_min_x = sqlite3_column_double(pStmt, i);
                  ++columnsFound;
               }
               else if ( colName == "min_y" )
               {
                  m_min_y = sqlite3_column_double(pStmt, i);
                  ++columnsFound;
               }
               else if ( colName == "max_x" )
               {
                  m_max_x = sqlite3_column_double(pStmt, i);
                  ++columnsFound;
               }
               else if ( colName == "max_y" )
               {
                  m_max_y = sqlite3_column_double(pStmt, i);
                  ++columnsFound;
               }
               else
               {
                  ossimNotify(ossimNotifyLevel_WARN)
                     << M << " Unhandled column name["
                     << i << "]: " << colName << std::endl;
               }
               
            } // Matches: if ( colName.size() )
            
            if ( columnsFound == EXPECTED_COLUMNS )
            {
               status = true;
               break;
            }
            
         } // Matches: for ( int i = 0; i < nCol; ++i )  
      }
      
   } // Matches: if ( pStmt )
   
   return status;
   
} // End:  ossimGpkgTileMatrixSetRecord::init( pStmt )

bool ossimGpkgTileMatrixSetRecord::init(
   const std::string& tableName,ossim_int32 srs_id,
   const ossimDpt& minPt, const ossimDpt& maxPt )
{
   bool status = false;
   if ( (minPt.hasNans() == false) && (maxPt.hasNans() == false) )
   {
      m_table_name = tableName;
      m_srs_id = srs_id;
      m_min_x  = minPt.x;
      m_min_y  = minPt.y;
      m_max_x  = maxPt.x;
      m_max_y  = maxPt.y;
      status   = true;
   }
   return status;
}

bool ossimGpkgTileMatrixSetRecord::createTable( sqlite3* db )
{
   bool status = false;
   if ( db )
   {
      status = ossim_sqlite::tableExists( db, TABLE_NAME );
      if ( !status )
      {
         std::ostringstream sql;
         sql << "CREATE TABLE " << TABLE_NAME << " ( "  
             << "table_name TEXT NOT NULL PRIMARY KEY, "
             << "srs_id INTEGER NOT NULL, "
             << "min_x DOUBLE NOT NULL, "
             << "min_y DOUBLE NOT NULL, "
             << "max_x DOUBLE NOT NULL, "
             << "max_y DOUBLE NOT NULL, "
             << "CONSTRAINT fk_gtms_table_name FOREIGN KEY (table_name) REFERENCES gpkg_contents(table_name), "
             << "CONSTRAINT fk_gtms_srs FOREIGN KEY (srs_id) REFERENCES gpkg_spatial_ref_sys (srs_id) "
             << ")";
         
         if ( ossim_sqlite::exec( db, sql.str() ) == SQLITE_DONE )
         {
            status = true;
         }
      }
   }
   return status;
}

bool ossimGpkgTileMatrixSetRecord::insert( sqlite3* db )
{
   bool status = false;
   if ( db )
   {
      std::ostringstream sql;
      sql << "INSERT INTO gpkg_tile_matrix_set VALUES ( "
          <<  "'" << m_table_name << "', "
          << m_srs_id << ", "
          << std::setprecision(16)
          << m_min_x << ", "
          << m_min_y << ", " 
          << m_max_x << ", "
          << m_max_y
          << " )";

      if ( ossim_sqlite::exec( db, sql.str() ) == SQLITE_DONE )
      {
         status = true;
      }    
   }
   return status;
}

void ossimGpkgTileMatrixSetRecord::getRect( ossimDrect& rect ) const
{
   rect = ossimDrect( m_min_x, m_max_y, m_max_x, m_min_y, OSSIM_RIGHT_HANDED );
}

ossim_float64 ossimGpkgTileMatrixSetRecord::getWidth() const
{
   return m_max_x - m_min_x;
}

ossim_float64 ossimGpkgTileMatrixSetRecord::getHeight() const
{
   return m_max_y - m_min_y;
}

void ossimGpkgTileMatrixSetRecord::saveState( ossimKeywordlist& kwl,
                                              const std::string& prefix ) const
{
   std::string myPref = prefix.size() ? prefix : std::string("gpkg_tile_matrix_set.");
   std::string value;
   
   std::string key = "table_name";
   kwl.addPair(myPref, key, m_table_name, true);

   key = "srs_id";
   value = ossimString::toString(m_srs_id).string();
   kwl.addPair(myPref, key, value, true);

   key = "min_x";
   value = ossimString::toString(m_min_x, 15).string();
   kwl.addPair(myPref, key, value, true);

   key = "min_y";
   value = ossimString::toString(m_min_y, 15).string();
   kwl.addPair(myPref, key, value, true);

   key = "max_x";
   value = ossimString::toString(m_max_x, 15).string();
   kwl.addPair(myPref, key, value, true);

   key = "max_y";
   value = ossimString::toString(m_max_y, 15).string();
   kwl.addPair(myPref, key, value, true);
}
