//----------------------------------------------------------------------------
//
// File: ossimGpkgTileRecord.cpp
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: Container class GeoPackage tile record.
//
//----------------------------------------------------------------------------
// $Id$

#include "ossimGpkgTileRecord.h"
#include "ossimSqliteUtil.h"
#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimDpt.h>
#include <ossim/base/ossimIpt.h>
#include <ossim/base/ossimNotify.h>
#include <sqlite3.h>
#include <cstring> /* for memcpy */

ossimGpkgTileRecord::ossimGpkgTileRecord()
   :
   m_id(0),
   m_zoom_level(0),
   m_tile_column(0),
   m_tile_row(0),
   m_tile_data(0),
   m_copy_tile_flag(true)
{
}

ossimGpkgTileRecord::ossimGpkgTileRecord(const ossimGpkgTileRecord& obj)
   :
   m_id(obj.m_id),
   m_zoom_level(obj.m_zoom_level),
   m_tile_column(obj.m_tile_column),
   m_tile_row(obj.m_tile_row),
   m_tile_data(obj.m_tile_data),
   m_copy_tile_flag(obj.m_copy_tile_flag)
{
}

const ossimGpkgTileRecord& ossimGpkgTileRecord::operator=(
   const ossimGpkgTileRecord& obj)
{
   if ( this != &obj )
   {
      m_id = obj.m_id;
      m_zoom_level = obj.m_zoom_level;
      m_tile_column = obj.m_tile_column;
      m_tile_row = obj.m_tile_row;
      m_tile_data = obj.m_tile_data;
      m_copy_tile_flag = obj.m_copy_tile_flag;
   }
   return *this;
}

ossimGpkgTileRecord::~ossimGpkgTileRecord()
{
}

bool ossimGpkgTileRecord::init( sqlite3_stmt* pStmt )
{
   static const char M[] = "ossimGpkgTileRecord::init";
   
   bool status = false;

   if ( pStmt )
   {
      const ossim_int32 EXPECTED_COLUMNS = 5;
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
         ossim_int32 type = 0;
         std::string colName;
         
         for ( ossim_int32 i = 0; i < nCol; ++i )
         {
            colName = sqlite3_column_name(pStmt, i);
            type = sqlite3_column_type(pStmt, i);

            if ( colName.size() )
            {
               if ( ( colName == "id" ) && ( type == SQLITE_INTEGER ) )
               {
                  m_id = sqlite3_column_int(pStmt, i);
                  ++columnsFound;
               }
               else if ( ( colName == "zoom_level" ) && ( type == SQLITE_INTEGER ) )
               {
                  m_zoom_level = sqlite3_column_int(pStmt, i);
                  ++columnsFound;
               }
               else if ( ( colName == "tile_column" ) && ( type == SQLITE_INTEGER ) )
               {
                  m_tile_column = sqlite3_column_int(pStmt, i);
                  ++columnsFound;
               }
               else if ( ( colName == "tile_row" ) && ( type == SQLITE_INTEGER ) )
               {
                  m_tile_row = sqlite3_column_int(pStmt, i);
                  ++columnsFound;
               }
               else if ( ( colName == "tile_data" ) && ( type == SQLITE_BLOB ) )
               {
                  ++columnsFound;

                  if ( m_copy_tile_flag )
                  {
                     ossim_int32 bytes = sqlite3_column_bytes( pStmt, i );
                     if ( bytes )
                     {
                        //---
                        // Copy the tile data as it will go away on the next:
                        // sqlite3_step(), sqlite3_reset() or sqlite3_finalize()
                        //---
                        m_tile_data.resize( bytes );
                        std::memcpy( (void*)&m_tile_data.front(),
                                     sqlite3_column_blob( pStmt, i ), bytes );
                     }
                  }
               }
               else
               {
                  ossimNotify(ossimNotifyLevel_WARN)
                     << M << " Unexpected column type[" << i << "]: " << type << std::endl;
                  break;
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

#if 0 /* Please leave for debug. (drb) */
   static bool tracedTile = false;
   if ( status && !tracedTile )
   {
      tracedTile = true;
      std::ofstream os;
      std::string file = "debug-tile.";
      swith( getTileType() )
      {
         case OSSIM_GPKG_PNG:
         {
            file += "png";
            break;
         }
         case OSSIM_GPKG_JPEG:
         {
            file += "jpg";
            break;
         }
         default:
            break;
      }
      
      os.open( file.c_str(), ios::out | ios::binary);
      if ( os.good() )
      {
         os.write( (char*)&m_tile_data.front(), m_tile_data.size() );
      }
      os.close();
   }
#endif
   
   return status;
   
} // End: ossimGpkgTileRecord::init( pStmt )

bool ossimGpkgTileRecord::createTable( sqlite3* db, const std::string& tableName )
{
   bool status = false;
   if ( db )
   {
      status = ossim_sqlite::tableExists( db, tableName );
      if ( !status )
      {
         std::ostringstream sql;
         sql << "CREATE TABLE " << tableName << " ( "
             << "id INTEGER PRIMARY KEY AUTOINCREMENT, "
             << "zoom_level INTEGER NOT NULL, "
             << "tile_column INTEGER NOT NULL, "
             << "tile_row INTEGER NOT NULL, "
             << "tile_data BLOB NOT NULL, "
             << "UNIQUE (zoom_level, tile_column, tile_row) "
             << ")";
         
         if ( ossim_sqlite::exec( db, sql.str() ) == SQLITE_DONE )
         {
            status = true;
         }
      }
   }
   return status;
}

void ossimGpkgTileRecord::setCopyTileFlag( bool flag )
{
   m_copy_tile_flag = flag;
}

std::ostream& ossimGpkgTileRecord::print(std::ostream& out) const
{
   out << "id: " << m_id
       << "\nzoom_level: " << m_zoom_level
       << "\nm_tile_column: " << m_tile_column
       << "\nm_tile_row: " << m_tile_row
       << "\nsignature_block: ";
   
   // Signature block:
   if ( m_tile_data.size() > 7 )
   {
      for ( int i = 0; i < 8; ++i )
      {
         out << std::hex << int(m_tile_data[i]) << " ";
      }
      out << std::dec;
   }
   else
   {
      out << "null";
   }
   out << "\nmedia_type: " << getTileMediaType() << std::endl;
   
   return out;
}

std::ostream& operator<<(std::ostream& out,
                         const ossimGpkgTileRecord& obj)
{
   return obj.print( out );
}

void ossimGpkgTileRecord::getTileIndex( ossimIpt& index ) const
{
   index.x = m_tile_column;
   index.y = m_tile_row;
}

ossimGpkgTileRecord::ossimGpkgTileType ossimGpkgTileRecord::getTileType() const
{
   ossimGpkgTileType result = ossimGpkgTileRecord::OSSIM_GPKG_UNKNOWN;

   if ( m_tile_data.size() > 7 )
   {
      if ( (m_tile_data[0] == 0xff) &&
           (m_tile_data[1] == 0xd8) &&
           (m_tile_data[2] == 0xff) )
      {
         if ( (m_tile_data[3] == 0xe0) || (m_tile_data[3] == 0xdb) )
         {
            result = ossimGpkgTileRecord::OSSIM_GPKG_JPEG;
         }
      }
      else if ( ( m_tile_data[0] == 0x89 ) &&
                ( m_tile_data[1] == 0x50 ) &&
                ( m_tile_data[2] == 0x4e ) &&
                ( m_tile_data[3] == 0x47 ) &&
                ( m_tile_data[4] == 0x0d ) &&
                ( m_tile_data[5] == 0x0a ) &&
                ( m_tile_data[6] == 0x1a ) &&
                ( m_tile_data[7] == 0x0a ) )
      {
         result = ossimGpkgTileRecord::OSSIM_GPKG_PNG;
      }

#if 0 /* Please keep for debug. (drb) */
      if ( result == ossimGpkgTileRecord::OSSIM_GPKG_UNKNOWN )
      {
         static bool traced = false;
         if ( !traced )
         {
            if ( result == ossimGpkgTileRecord::OSSIM_GPKG_UNKNOWN )
            {
               for ( int i = 0; i < 8; ++i )
               {
                  std::cout << std::hex << int(m_tile_data[i]) << " ";
               }
               std::cout << std::dec << "\n";
               traced = true;
            }
         }
      }
#endif
   }

   return result;   
}

std::string ossimGpkgTileRecord::getTileMediaType() const
{
   std::string result;
   switch ( getTileType() )
   {
      case ossimGpkgTileRecord::OSSIM_GPKG_JPEG:
      {
         result = "image/jpeg";
         break;
      }
      case ossimGpkgTileRecord::OSSIM_GPKG_PNG:
      {
         result = "image/png";
         break;
      }
      default:
      {
         result = "unknown";
         break;
      }
   }
   return result;
}
