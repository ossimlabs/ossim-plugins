//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author: David Burken
//
// Description: GeoPackage Info class.
// 
//----------------------------------------------------------------------------
// $Id$


#include "ossimGpkgInfo.h"
#include "ossimGpkgDatabaseRecordBase.h"
#include "ossimGpkgUtil.h"

#include <ossim/base/ossimCommon.h>
#include <ossim/base/ossimException.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimString.h>
#include <ossim/base/ossimTrace.h>

#include <sqlite3.h>

static ossimTrace traceDebug("ossimGpkgInfo:debug");
static ossimTrace traceTiles("ossimGpkgInfo:tiles");

ossimGpkgInfo::ossimGpkgInfo()
   : ossimInfoBase(),
     m_file()
{
}

ossimGpkgInfo::~ossimGpkgInfo()
{
}

bool ossimGpkgInfo::open(const ossimFilename& file)
{
   bool result = false;
   
   if ( file.size() ) // Check for empty filename.
   {
      std::ifstream str;
      str.open( file.c_str(), std::ios_base::in | std::ios_base::binary);

      if ( str.good() ) // Open good...
      {
         if ( ossim_gpkg::checkSignature( str ) ) // Has "SQLite format 3" signature.
         {
            m_file = file;
            result = true;
         }
      }
   }

   return result;
}

std::ostream& ossimGpkgInfo::print(std::ostream& out) const
{
   static const char MODULE[] = "ossimGpkgInfo::open";
   
   if ( traceDebug() )
   {
      ossimNotify(ossimNotifyLevel_DEBUG)
         << MODULE << " entered..."
         << "File:  " << m_file.c_str()
         << std::endl;
   }

   // Check for empty filename.
   if (m_file.size())
   {
      sqlite3* db;
      
      int rc = sqlite3_open_v2( m_file.c_str(), &db, SQLITE_OPEN_READONLY, 0);
      if ( rc == SQLITE_OK )
      {
         ossimKeywordlist kwl;

         // Get the gpkg_contents records:
         std::string tableName = "gpkg_contents";
         std::vector< ossimRefPtr<ossimGpkgDatabaseRecordBase> > records;
         if ( ossim_gpkg::getTableRows( db, tableName, records ) )
         {
            std::vector< ossimRefPtr<ossimGpkgDatabaseRecordBase> >::const_iterator i =
               records.begin();
            ossim_int32 index = 0;
            while ( i != records.end() )
            {
               std::string prefix = "gpkg.";
               prefix += tableName;
               prefix += ossimString::toString(index++).string();
               prefix += ".";
               (*i)->saveState( kwl, prefix );
               ++i;
            } 
         }

         records.clear();

         // Get the gpkg_spatial_ref_sys record:
         tableName = "gpkg_spatial_ref_sys";
         if ( ossim_gpkg::getTableRows( db, tableName, records ) )
         {
            std::vector< ossimRefPtr<ossimGpkgDatabaseRecordBase> >::const_iterator i =
               records.begin();
            ossim_int32 index = 0;
            while ( i != records.end() )
            {
               std::string prefix = "gpkg.";
               prefix += tableName;
               prefix += ossimString::toString(index++).string();
               prefix += ".";
               (*i)->saveState( kwl, prefix );
               ++i;
            }
         }

         records.clear();

         // Get the gpkg_tile_matrix_set records:
         tableName = "gpkg_tile_matrix_set";
         if ( ossim_gpkg::getTableRows( db, tableName, records ) )
         {
            std::vector< ossimRefPtr<ossimGpkgDatabaseRecordBase> >::const_iterator i =
               records.begin();
            ossim_int32 index = 0;
            while ( i != records.end() )
            {
               std::string prefix = "gpkg.";
               prefix += tableName;
               prefix += ossimString::toString(index++).string();
               prefix += ".";
               (*i)->saveState( kwl, prefix );
               ++i;

               if ( traceTiles() )
               {
                  std::string key = "table_name";
                  std::string tileTableName = kwl.findKey( prefix, key );
                  if ( tileTableName.size() )
                  {
                     ossim_gpkg::printTiles( db, tileTableName,
                                             ossimNotify(ossimNotifyLevel_DEBUG) );
                  }
               }
            } 
         }

         records.clear();

         // Get the gpkg_tile_matrix_set records:
         tableName = "gpkg_tile_matrix";
         if ( ossim_gpkg::getTableRows( db, tableName, records ) )
         {
            std::vector< ossimRefPtr<ossimGpkgDatabaseRecordBase> >::const_iterator i =
               records.begin();
            ossim_int32 index = 0;
            while ( i != records.end() )
            {
               std::string prefix = "gpkg.";
               prefix += tableName;
               prefix += ossimString::toString(index++).string();
               prefix += ".";
               (*i)->saveState( kwl, prefix );
               ++i;
            } 
         }

         records.clear();
         
         // Get the nsg_tile_matrix_extent records:
         tableName = "nsg_tile_matrix_extent";
         if ( ossim_gpkg::getTableRows( db, tableName, records ) )
         {
            std::vector< ossimRefPtr<ossimGpkgDatabaseRecordBase> >::const_iterator i =
               records.begin();
            ossim_int32 index = 0;
            while ( i != records.end() )
            {
               std::string prefix = "gpkg.";
               prefix += tableName;
               prefix += ossimString::toString(index++).string();
               prefix += ".";
               (*i)->saveState( kwl, prefix );
               ++i;
            } 
         }



         if ( kwl.getSize() )
         {
            out << kwl << std::endl;
         }
      }

      sqlite3_close(db);
      db = 0;
   }

   return out;
}
