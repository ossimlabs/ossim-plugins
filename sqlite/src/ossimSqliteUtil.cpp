//----------------------------------------------------------------------------
// File: ossimGpkgUtil.cpp
// 
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: OSSIM SQLite utility class.
//----------------------------------------------------------------------------
// $Id$

#include "ossimSqliteUtil.h"
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimTrace.h>
#include <sqlite3.h>
#include <OpenThreads/Mutex>
#include <ctime>

static OpenThreads::Mutex timeMutex;
static ossimTrace traceDebug("ossimSqliteUtil:debug");

int ossim_sqlite::exec( sqlite3* db, const std::string& sql )
{
   int rc = SQLITE_ERROR;
   if ( db && sql.size() )
   {
      if (traceDebug())
      {
         ossimNotify(ossimNotifyLevel_DEBUG)
            << "sql:\n" << sql << "\n";
      }

      sqlite3_stmt* pStmt = 0; // The current SQL statement

      rc = sqlite3_prepare_v2(db,          // Database handle
                              sql.c_str(), // SQL statement, UTF-8 encoded
                              -1,          // Maximum length of zSql in bytes.
                              &pStmt,      // OUT: Statement handle
                              NULL);
      if ( rc == SQLITE_OK )
      {
         rc = sqlite3_step(pStmt);
      }
      else
      {
         ossimNotify(ossimNotifyLevel_WARN)
            << "ossim_sqlite::exec error: " << sqlite3_errmsg(db) << std::endl;
      }
      
      sqlite3_finalize(pStmt);
   }
   return rc;
}

bool ossim_sqlite::tableExists( sqlite3* db, const std::string& tableName )
{
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
         int nCol = sqlite3_column_count( pStmt );
         if ( nCol )
         {
            status = true;
         }
      }
      sqlite3_finalize(pStmt);
   }
   
   return status;
   
} // End: ossim_sqlite::tableExists(...)

void ossim_sqlite::warn( const std::string& module,
                         const std::string& columnName,
                         ossim_int32 columnIndex,
                         ossim_int32 type )
{
   ossimNotify(ossimNotifyLevel_WARN)
      << module << " Unexpected column name or type[" << columnIndex << "]: "
      << "name: " << columnName << " type: " << type << std::endl;
}

void ossim_sqlite::getTime( std::string& result )
{
   timeMutex.lock();
   
   time_t rawTime;
   time(&rawTime);
   
   struct tm* timeInfo = gmtime(&rawTime);

   size_t size = 0;
   if ( timeInfo )
   {
      const size_t STRING_SIZE = 25;
      char outStr[STRING_SIZE];

      size = strftime(outStr, STRING_SIZE, "%Y-%m-%dT%H:%M:%S.000Z", timeInfo );
      if ( size )
      {
         // Per strftime spec not needed but null terminating anyway.
         outStr[STRING_SIZE-1] = '\0';
         result = outStr;
      }
   }
   if ( !size )
   {
      result.clear();
   }

   timeMutex.unlock();
}
