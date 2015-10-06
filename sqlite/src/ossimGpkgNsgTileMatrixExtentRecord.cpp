//----------------------------------------------------------------------------
//
// File: ossimGpkgNsgTileMatrixExtentRecord.cpp
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: Container class for nsg_tile_matrix_extent table.
//
//----------------------------------------------------------------------------
// $Id$

#include "ossimGpkgNsgTileMatrixExtentRecord.h"
#include "ossimSqliteUtil.h"
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimDrect.h>
#include <ossim/base/ossimIrect.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimNotify.h>
#include <sqlite3.h>
#include <iomanip>

static const std::string TABLE_NAME = "nsg_tile_matrix_extent";

ossimGpkgNsgTileMatrixExtentRecord::ossimGpkgNsgTileMatrixExtentRecord()
   :
   ossimGpkgDatabaseRecordBase(),   
   m_table_name(),
   m_zoom_level(0),
   m_extent_type(),
   m_min_column(0),
   m_min_row(0),
   m_max_column(0),
   m_max_row(0),
   m_min_x(ossim::nan()),
   m_min_y(ossim::nan()),
   m_max_x(ossim::nan()),
   m_max_y(ossim::nan())
{
}

ossimGpkgNsgTileMatrixExtentRecord::ossimGpkgNsgTileMatrixExtentRecord(
   const ossimGpkgNsgTileMatrixExtentRecord& obj)
   :
   ossimGpkgDatabaseRecordBase(),
   m_table_name(obj.m_table_name),
   m_zoom_level(obj.m_zoom_level),
   m_extent_type(obj.m_extent_type),
   m_min_column(obj.m_min_column),
   m_min_row(obj.m_min_row),
   m_max_column(obj.m_max_column),
   m_max_row(obj.m_max_row),   
   m_min_x(obj.m_min_x),
   m_min_y(obj.m_min_y),
   m_max_x(obj.m_max_x),
   m_max_y(obj.m_max_y)
{
}

const ossimGpkgNsgTileMatrixExtentRecord& ossimGpkgNsgTileMatrixExtentRecord::operator=(
   const ossimGpkgNsgTileMatrixExtentRecord& obj)
{
   if ( this != &obj )
   {
      m_table_name  = obj.m_table_name;
      m_zoom_level  = obj.m_zoom_level;
      m_extent_type = obj.m_extent_type;
      m_min_column  = obj.m_min_column;
      m_min_row     = obj.m_min_row;      
      m_max_column  = obj.m_max_column;
      m_max_row     = obj.m_max_row;
      m_min_x       = obj.m_min_x;
      m_min_y       = obj.m_min_y;
      m_max_x       = obj.m_max_x;
      m_max_y       = obj.m_max_y;
   }
   return *this;
}

ossimGpkgNsgTileMatrixExtentRecord::~ossimGpkgNsgTileMatrixExtentRecord()
{
}

const std::string& ossimGpkgNsgTileMatrixExtentRecord::getTableName()
{
   return TABLE_NAME;
}

bool ossimGpkgNsgTileMatrixExtentRecord::init( sqlite3_stmt* pStmt )
{
   static const char M[] = "ossimGpkgNsgTileMatrixExtentRecord::init";
   
   bool status = false;

   if ( pStmt )
   {
      const ossim_int32 EXPECTED_COLUMNS = 11;
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
         ossim_int32 type;
         
         const char* c = 0; // To catch null so not to pass to string.
         
         for ( ossim_int32 i = 0; i < nCol; ++i )
         {
            colName = sqlite3_column_name(pStmt, i);
            type    = sqlite3_column_type(pStmt, i);
            if ( colName.size() )
            {
               if ( colName == "table_name" )
               {
                  c = (const char*)sqlite3_column_text(pStmt, i);
                  m_table_name = (c ? c : "");
                  ++columnsFound;
               }
               else if ( ( colName == "zoom_level" ) && ( type == SQLITE_INTEGER ) )
               {
                  m_zoom_level = sqlite3_column_int(pStmt, i);
                  ++columnsFound;
               }
               else if ( colName == "extent_type" )
               {
                  c = (const char*)sqlite3_column_text(pStmt, i);
                  m_extent_type = (c ? c : "");
                  ++columnsFound;
               }
               else if ( ( colName == "min_column" ) && ( type == SQLITE_INTEGER ) )
               {
                  m_min_column = sqlite3_column_int(pStmt, i);
                  ++columnsFound;
               }
               else if ( ( colName == "min_row" ) && ( type == SQLITE_INTEGER ) )
               {
                  m_min_row = sqlite3_column_int(pStmt, i);
                  ++columnsFound;
               }
               else if ( ( colName == "max_column" ) && ( type == SQLITE_INTEGER ) )
               {
                  m_max_column = sqlite3_column_int(pStmt, i);
                  ++columnsFound;
               }
               else if ( ( colName == "max_row" ) && ( type == SQLITE_INTEGER ) )
               {
                  m_max_row = sqlite3_column_int(pStmt, i);
                  ++columnsFound;
               }
               else if ( ( colName == "min_x" ) && ( type == SQLITE_FLOAT ) )
               {
                  m_min_x = sqlite3_column_double(pStmt, i);
                  ++columnsFound;
               }
               else if ( ( colName == "min_y" ) && ( type == SQLITE_FLOAT ) )
               {
                  m_min_y = sqlite3_column_double(pStmt, i);
                  ++columnsFound;
               }
               else if ( ( colName == "max_x" ) && ( type == SQLITE_FLOAT ) ) 
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
   
} // End:  ossimGpkgNsgTileMatrixExtentRecord::init( pStmt )

bool ossimGpkgNsgTileMatrixExtentRecord::init( const std::string& tableName,
                                               ossim_int32 zoom_level,
                                               const ossimIrect& imageRect,
                                               const ossimDrect& projectionRect )
{
   bool status = false;
   if ( (imageRect.hasNans() == false) && (projectionRect.hasNans() == false) )
   {
      m_table_name  = tableName;
      m_zoom_level  = zoom_level;
      m_extent_type = "complete";
      m_min_column  = imageRect.ul().x;
      m_min_row     = imageRect.ul().y;
      m_max_column  = imageRect.lr().x;
      m_max_row     = imageRect.lr().y;
      m_min_x       = projectionRect.ll().x;
      m_min_y       = projectionRect.ll().y;
      m_max_x       = projectionRect.ur().x;
      m_max_y       = projectionRect.ur().y;

      status = true;
   }
   return status;
}

bool ossimGpkgNsgTileMatrixExtentRecord::createTable( sqlite3* db )
{
   bool status = false;
   if ( db )
   {
      status = ossim_sqlite::tableExists( db, TABLE_NAME );
      if ( !status )
      {
         std::ostringstream sql;
         sql << "CREATE TABLE " << TABLE_NAME << " ( "
             << "table_name TEXT NOT NULL, "
             << "zoom_level INTEGER NOT NULL, "
             << "extent_type TEXT NOT NULL, "
             << "min_column INTEGER NOT NULL, "
             << "min_row INTEGER NOT NULL, "         
             << "max_column INTEGER NOT NULL, "
             << "max_row INTEGER NOT NULL, "         
             << "min_x DOUBLE NOT NULL, "
             << "min_y DOUBLE NOT NULL, "
             << "max_x DOUBLE NOT NULL, "
             << "max_y DOUBLE NOT NULL, "
             << "CONSTRAINT pk_ntme PRIMARY KEY (table_name, zoom_level, extent_type, min_column, min_row, max_column, max_row), "
             << "CONSTRAINT fk_ntme FOREIGN KEY (table_name, zoom_level) "
             << "REFERENCES gpkg_tile_matrix(table_name, zoom_level)"
             << ")";
         
         if ( ossim_sqlite::exec( db, sql.str() ) == SQLITE_DONE )
         {
            status = true;
         }
      }
   }
   return status;
}

bool ossimGpkgNsgTileMatrixExtentRecord::insert( sqlite3* db )
{
   bool status = false;
   if ( db )
   {
      std::ostringstream sql;
      sql << "INSERT INTO nsg_tile_matrix_extent VALUES ( "
          <<  "'" << m_table_name  << "', "
          << m_zoom_level  << ", "
          <<  "'" << m_extent_type << "', "
          << m_min_column  << ", "
          << m_min_row     << ", "
          << m_max_column  << ", "
          << m_max_row     << ", "
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

void ossimGpkgNsgTileMatrixExtentRecord::saveState( ossimKeywordlist& kwl,
                                                    const std::string& prefix ) const
{
   std::string myPref = prefix.size() ? prefix : std::string("nsg_tile_matrix_extent.");
   std::string value;
   
   std::string key = "table_name";
   kwl.addPair(myPref, key, m_table_name, true);

   key = "zoom_level";
   value = ossimString::toString(m_zoom_level).string();
   kwl.addPair(myPref, key, value, true);

   key = "extent_type";
   kwl.addPair(myPref, key, m_extent_type, true);
   
   key = "min_column";
   value = ossimString::toString(m_min_column).string();
   kwl.addPair(myPref, key, value, true);

   key = "min_row";
   value = ossimString::toString(m_min_row).string();
   kwl.addPair(myPref, key, value, true);

   key = "max_column";
   value = ossimString::toString(m_max_column).string();
   kwl.addPair(myPref, key, value, true);

   key = "max_row";
   value = ossimString::toString(m_max_row).string();
   kwl.addPair(myPref, key, value, true);

   key = "min_x";
   value = ossimString::toString(m_min_x, 20).string();
   kwl.addPair(myPref, key, value, true);

   key = "min_y";
   value = ossimString::toString(m_min_y, 20).string();
   kwl.addPair(myPref, key, value, true);

   key = "max_x";
   value = ossimString::toString(m_max_x, 20).string();
   kwl.addPair(myPref, key, value, true);

   key = "max_y";
   value = ossimString::toString(m_max_y, 20).string();
   kwl.addPair(myPref, key, value, true);
}
