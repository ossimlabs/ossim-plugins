//----------------------------------------------------------------------------
// File: ossimSqliteUtil.h
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: OSSIM SQLite utility class.
//
//----------------------------------------------------------------------------
// $Id

#ifndef ossimSqliteUtil_HEADER
#define ossimSqliteUtil_HEADER 1

#include <ossim/base/ossimConstants.h>
#include <string>

struct sqlite3;

namespace ossim_sqlite
{
   /**
    * @brief Preforms sqlite3_prepare_v2, sqlite3_step and sqlite3_finalize.
    * @param db
    * @param sql 
    * @return return code of last command executed.  Typically this is the
    * return of sqlite3_step but could be return of sqlite3_prepare_v2 if
    * it had an error, i.e. return is not SQLITE_OK.
    */   
   int exec( sqlite3* db, const std::string& sql );

   /**
    * @brief Checks for existance of table.
    * @param db An open database.
    * @param tableName e.g. "gpkg_contents"
    * @return true on success, false on error.
    */
   bool tableExists( sqlite3* db, const std::string& tableName );

   /**
    * @brief Outputs formated warning message.
    * @param module e.g. "ossimGpkgNsgTileMatrixSetRecord::init"
    * @param columnName e.g. "zoom_level", from sqlite3_column_name(...)
    * @param columnIndex zero based column index.
    * @param type Type from sqlite3_column_type(...)
    */
   void warn( const std::string& module,
              const std::string& columnName,
              ossim_int32 columnIndex,
              ossim_int32 type );

   /**
    * @brief Gets time in the form of "%Y-%m-%dT%H:%M:%S.000Z".
    * e.g. 2015-02-10T19:32:15.000Z
    * @param result Initialized by this.
    */
   void getTime( std::string& result );
   
} // End: namespace ossim_sqlite

#endif /* #ifndef ossimSqliteUtil_HEADER */
