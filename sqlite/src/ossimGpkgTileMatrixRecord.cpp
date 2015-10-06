//----------------------------------------------------------------------------
//
// File: ossimGpkgTileMatrixRecord.cpp
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: Container class for gpkg_tile_matrix table.
//
//----------------------------------------------------------------------------
// $Id$

#include "ossimGpkgTileMatrixRecord.h"
#include "ossimSqliteUtil.h"
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimNotify.h>
#include <sqlite3.h>
#include <iomanip>

static const std::string TABLE_NAME = "gpkg_tile_matrix";

ossimGpkgTileMatrixRecord::ossimGpkgTileMatrixRecord()
   :
   ossimGpkgDatabaseRecordBase(),   
   m_table_name(),
   m_zoom_level(0),
   m_matrix_width(0),
   m_matrix_height(0),   
   m_tile_width(0),
   m_tile_height(0),   
   m_pixel_x_size(ossim::nan()),
   m_pixel_y_size(ossim::nan())
{
}

ossimGpkgTileMatrixRecord::ossimGpkgTileMatrixRecord(const ossimGpkgTileMatrixRecord& obj)
   :
   ossimGpkgDatabaseRecordBase(),
   m_table_name(obj.m_table_name),
   m_zoom_level(obj.m_zoom_level),
   m_matrix_width(obj.m_matrix_width),
   m_matrix_height(obj.m_matrix_height),
   m_tile_width(obj.m_tile_width),
   m_tile_height(obj.m_tile_height),   
   m_pixel_x_size(obj.m_pixel_x_size),
   m_pixel_y_size(obj.m_pixel_y_size)
{
}

const ossimGpkgTileMatrixRecord& ossimGpkgTileMatrixRecord::operator=(
   const ossimGpkgTileMatrixRecord& obj)
{
   if ( this != &obj )
   {
      m_table_name = obj.m_table_name;
      m_zoom_level = obj.m_zoom_level;
      m_matrix_width = obj.m_matrix_width;
      m_matrix_height = obj.m_matrix_height;
      m_tile_width = obj.m_tile_width;
      m_tile_height = obj.m_tile_height;   
      m_pixel_x_size = obj.m_pixel_x_size;
      m_pixel_y_size = obj.m_pixel_y_size;
   }
   return *this;
}

bool ossimGpkgTileMatrixRecord::init( const std::string& tableName,
                                      ossim_int32 zoom_level,
                                      const ossimIpt& matrixSize,
                                      const ossimIpt& tileSize,
                                      const ossimDpt& gsd )
{
   bool status = false;

   if ( (matrixSize.hasNans() == false) && (tileSize.hasNans() == false) &&
        (gsd.hasNans() == false) )
   {
      m_table_name    = tableName;
      m_zoom_level    = zoom_level;
      m_matrix_width  = matrixSize.x;
      m_matrix_height = matrixSize.y;
      m_tile_width    = tileSize.x;
      m_tile_height   = tileSize.y;
      m_pixel_x_size  = gsd.x;
      m_pixel_y_size  = gsd.y;
      status = true;
   }
   
   return status;
   
} // End: ossimGpkgTileMatrixRecord::init( zoom_level, ... )

bool ossimGpkgTileMatrixRecord::createTable( sqlite3* db )
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
             << "matrix_width INTEGER NOT NULL, "
             << "matrix_height INTEGER NOT NULL, "
             << "tile_width INTEGER NOT NULL, "
             << "tile_height INTEGER NOT NULL, "
             << "pixel_x_size DOUBLE NOT NULL, "
             << "pixel_y_size DOUBLE NOT NULL, "
             << "CONSTRAINT pk_ttm PRIMARY KEY (table_name, zoom_level), "
             << "CONSTRAINT fk_tmm_table_name FOREIGN KEY (table_name) REFERENCES gpkg_contents(table_name) "
             << ")";
         
         if ( ossim_sqlite::exec( db, sql.str() ) == SQLITE_DONE )
         {
            status = true;
         }
      }
   }
   return status;
}

bool ossimGpkgTileMatrixRecord::insert( sqlite3* db )
{
   bool status = false;
   if ( db )
   {
      std::ostringstream sql;
      sql << "INSERT INTO gpkg_tile_matrix VALUES ( "
          <<  "'" << m_table_name << "', "
          << m_zoom_level << ", "
          << m_matrix_width <<  ", "
          << m_matrix_height << ", "
          << m_tile_width << ", "
          << m_tile_height << ", "
          << std::setiosflags(std::ios::fixed) << std::setprecision(16)
          << m_pixel_x_size << ", "
          << m_pixel_y_size
          << " )";

      if ( ossim_sqlite::exec( db, sql.str() ) == SQLITE_DONE )
      {
         status = true;
      }    
   }
   return status;
}

ossimGpkgTileMatrixRecord::~ossimGpkgTileMatrixRecord()
{
}

const std::string& ossimGpkgTileMatrixRecord::getTableName()
{
   return TABLE_NAME;
}

bool ossimGpkgTileMatrixRecord::init( sqlite3_stmt* pStmt )
{
   static const char M[] = "ossimGpkgTileMatrixRecord::init";
   
   bool status = false;
   
   if ( pStmt )
   {
      const ossim_int32 EXPECTED_COLUMNS = 8;
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
               else if ( colName == "zoom_level" )
               {
                  m_zoom_level = sqlite3_column_int(pStmt, i);
                  ++columnsFound;
               }
               else if ( colName == "matrix_width" )
               {
                  m_matrix_width = sqlite3_column_int(pStmt, i);
                  ++columnsFound;
               }
               else if ( colName == "matrix_height" )
               {
                  m_matrix_height = sqlite3_column_int(pStmt, i);
                  ++columnsFound;
               }
               else if ( colName == "tile_width" )
               {
                  m_tile_width = sqlite3_column_int(pStmt, i);
                  ++columnsFound;
               }
               else if ( colName == "tile_height" )
               {
                  m_tile_height = sqlite3_column_int(pStmt, i);
                  ++columnsFound;
               }
               else if ( colName == "pixel_x_size" )
               {
                  m_pixel_x_size = sqlite3_column_double(pStmt, i);
                  ++columnsFound;
               }
               else if ( colName == "pixel_y_size" )
               {
                  m_pixel_y_size = sqlite3_column_double(pStmt, i);
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
   
} // End: ossimGpkgTileMatrixRecord::init( sqlite3_stmt* pStmt )

void ossimGpkgTileMatrixRecord::saveState( ossimKeywordlist& kwl,
                                           const std::string& prefix ) const
{
   std::string myPref = prefix.size() ? prefix : std::string("gpkg_tile_matrix.");
   std::string value;
   
   std::string key = "table_name";
   kwl.addPair(myPref, key, m_table_name, true);

   key = "zoom_level";
   value = ossimString::toString(m_zoom_level).string();
   kwl.addPair(myPref, key, value, true);

   key = "matrix_width";
   value = ossimString::toString(m_matrix_width).string();
   kwl.addPair(myPref, key, value, true);

   key = "matrix_height";
   value = ossimString::toString(m_matrix_height).string();
   kwl.addPair(myPref, key, value, true);

   key = "tile_width";
   value = ossimString::toString(m_tile_width).string();
   kwl.addPair(myPref, key, value, true);

   key = "tile_height";
   value = ossimString::toString(m_tile_height).string();
   kwl.addPair(myPref, key, value, true);

   key = "pixel_x_size";
   value = ossimString::toString(m_pixel_x_size, 15, true).string();
   kwl.addPair(myPref, key, value, true);

   key = "pixel_y_size";
   value = ossimString::toString(m_pixel_y_size, 15, true).string();
   kwl.addPair(myPref, key, value, true);
}

void ossimGpkgTileMatrixRecord::getMatrixSize( ossimIpt& size ) const
{
   size.x = m_matrix_width;
   size.y = m_matrix_height;
}

void ossimGpkgTileMatrixRecord::getTileSize( ossimIpt& size ) const
{
   size.x = m_tile_width;
   size.y = m_tile_height;
}

void ossimGpkgTileMatrixRecord::getGsd( ossimDpt& gsd ) const
{
   gsd.x = m_pixel_x_size;
   gsd.y = m_pixel_y_size;
}
