//----------------------------------------------------------------------------
//
// File: ossimGpkgContentsRecord.cpp
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: Container class for gpkg_contents table.
//
//----------------------------------------------------------------------------
// $Id$

#include "ossimGpkgContentsRecord.h"
#include "ossimSqliteUtil.h"
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimNotify.h>
#include <sqlite3.h>
#include <iomanip>

static const std::string TABLE_NAME = "gpkg_contents";

ossimGpkgContentsRecord::ossimGpkgContentsRecord()
   :
   ossimGpkgDatabaseRecordBase(),
   m_table_name(),
   m_data_type(),
   m_identifier(),
   m_description(),
   m_last_change(),
   m_min_x(ossim::nan()),
   m_min_y(ossim::nan()),
   m_max_x(ossim::nan()),
   m_max_y(ossim::nan()),
   m_srs_id(0)
{
}

ossimGpkgContentsRecord::ossimGpkgContentsRecord(const ossimGpkgContentsRecord& obj)
   :
   ossimGpkgDatabaseRecordBase(),
   m_table_name(obj.m_table_name),
   m_data_type(obj.m_data_type),
   m_identifier(obj.m_identifier),
   m_description(obj.m_description),
   m_last_change(obj.m_last_change),
   m_min_x(obj.m_min_x),
   m_min_y(obj.m_min_y),
   m_max_x(obj.m_max_x),
   m_max_y(obj.m_max_y),
   m_srs_id(obj.m_srs_id)
{
}

const ossimGpkgContentsRecord& ossimGpkgContentsRecord::operator=(
   const ossimGpkgContentsRecord& copy_this)
{
   if ( this != &copy_this )
   {
      m_table_name = copy_this.m_table_name;
      m_data_type = copy_this.m_data_type;
      m_identifier = copy_this.m_identifier;
      m_description = copy_this.m_description;
      m_last_change = copy_this.m_last_change;
      m_min_x = copy_this.m_min_x;
      m_min_y = copy_this.m_min_y;
      m_max_x = copy_this.m_max_x;
      m_max_y = copy_this.m_max_y;
      m_srs_id = copy_this.m_srs_id;
   }
   return *this;
}

ossimGpkgContentsRecord::~ossimGpkgContentsRecord()
{
}

const std::string& ossimGpkgContentsRecord::getTableName()
{
   return TABLE_NAME;
}

bool ossimGpkgContentsRecord::init( sqlite3_stmt* pStmt )
{
   static const char M[] = "ossimGpkgContentsRecord::init";
   
   bool status = false;

   if ( pStmt )
   {
      const ossim_int32 EXPECTED_COLUMNS = 10;
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
               else if ( colName == "data_type" )
               {
                  c = (const char*)sqlite3_column_text(pStmt, i);
                  m_data_type = (c ? c : "");
                  ++columnsFound;
               }
               else if ( colName == "identifier" )
               {
                  c = (const char*)sqlite3_column_text(pStmt, i);
                  m_identifier = (c ? c : "");
                  ++columnsFound;
               }
               else if ( colName == "description" )
               {
                  c = (const char*)sqlite3_column_text(pStmt, i);
                  m_description = (c ? c : "");
                  ++columnsFound;
               }
               else if ( colName == "last_change" )
               {
                  c = (const char*)sqlite3_column_text(pStmt, i);
                  m_last_change = (c ? c : "");
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
               else if ( colName == "srs_id" )
               {
                  m_srs_id = sqlite3_column_int(pStmt, i);
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
            
         } // Matches: for ( ossim_int32 i = 0; i < nCol; ++i )  
      }
      
   } // Matches: if ( pStmt )
   
   return status;
   
} // End: ossimGpkgContentsRecord::init( sqlite3_stmt* pStmt )

bool ossimGpkgContentsRecord::init(
   const std::string& tableName, ossim_int32 srs_id,
   const ossimDpt& minPt, const ossimDpt& maxPt )
{
   bool status = false;
   if ( (minPt.hasNans() == false) && (maxPt.hasNans() == false) )
   {
      m_table_name  = tableName;
      m_data_type   = "tiles";
      m_identifier  = "Raster Tiles for ";
      m_identifier  += tableName;
      m_description = "Create by ossim gpkg writer.";
      
      ossim_sqlite::getTime( m_last_change );

      m_min_x  = minPt.x;
      m_min_y  = minPt.y;
      m_max_x  = maxPt.x;
      m_max_y  = maxPt.y;
      m_srs_id = srs_id;
      status   = true;
   }
   return status;
}

bool ossimGpkgContentsRecord::createTable( sqlite3* db )
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
             << "data_type TEXT NOT NULL, "
             << "identifier TEXT UNIQUE, "
             << "description TEXT DEFAULT \"\", "

            //---
            // Requirement 13:
            // Note: Spec changed to %fZ.  No %f on linux but sqlite library handles.
            // << "last_change DATETIME NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now')), "
            //---
             << "last_change DATETIME NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')), "
             << "min_x DOUBLE, "
             << "min_y DOUBLE, "
             << "max_x DOUBLE, "
             << "max_y DOUBLE, "
             << "srs_id INTEGER, "
             << "CONSTRAINT fk_gc_r_srs_id FOREIGN KEY (srs_id) "
             << "REFERENCES gpkg_spatial_ref_sys(srs_id)"
             << ")";
         
         if ( ossim_sqlite::exec( db, sql.str() ) == SQLITE_DONE )
         {
            status = true;
         }
      }
   }
   return status;
}

bool ossimGpkgContentsRecord::insert( sqlite3* db )
{
   bool status = false;
   if ( db )
   {
      std::ostringstream sql;
      sql << "INSERT INTO gpkg_contents VALUES ( "
          <<  "'" << m_table_name << "', "
          <<  "'" << m_data_type << "', "
          <<  "'" << m_identifier << "', "
          <<  "'" << m_description << "', "
          <<  "'" << m_last_change << "', "
          << std::setprecision(16)
          << m_min_x << ", "
          << m_min_y << ", "
          << m_max_x << ", "
          << m_max_y << ", "
          << m_srs_id
          << " )";

      if ( ossim_sqlite::exec( db, sql.str() ) == SQLITE_DONE )
      {
         status = true;
      }    
   }
   return status;
}

void ossimGpkgContentsRecord::saveState( ossimKeywordlist& kwl,
                                         const std::string& prefix ) const
{
   std::string myPref = prefix.size() ? prefix : std::string("gpkg_contents.");
   std::string value;
   
   std::string key = "table_name";
   kwl.addPair(myPref, key, m_table_name, true);

   key = "data_type";
   kwl.addPair(myPref, key, m_data_type, true);
   
   key = "identifier";
   kwl.addPair(myPref, key, m_identifier, true);
   
   key = "description";
   kwl.addPair(myPref, key, m_description, true);

   key = "last_change";
   kwl.addPair(myPref, key, m_last_change, true);

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

   key = "srs_id";
   value = ossimString::toString(m_srs_id).string();
   kwl.addPair(myPref, key, value, true);
}
