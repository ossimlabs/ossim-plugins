//----------------------------------------------------------------------------
//
// File: ossimGpkgSpatialRefSysRecord.cpp
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: Container class for gpkg_spatial_ref_sys table.
//
//----------------------------------------------------------------------------
// $Id$

#include "ossimGpkgSpatialRefSysRecord.h"
#include "ossimSqliteUtil.h"

#include <ossim/base/ossimGpt.h>
#include <ossim/base/ossimKeywordlist.h>
#include <ossim/base/ossimNotify.h>
#include <ossim/base/ossimTrace.h>

#include <ossim/projection/ossimEquDistCylProjection.h>
#include <ossim/projection/ossimGoogleProjection.h>
#include <ossim/projection/ossimMapProjection.h>
#include <ossim/projection/ossimUtmProjection.h>

#include <sqlite3.h>

#include <sstream>

//---
// For trace debugging. To enable at runtime do:
// your_app -T "ossimGpkgSpatialRefSysRecord:debug" your_app_args
//---
static ossimTrace traceDebug("ossimGpkgSpatialRefSysRecord:debug");

static std::string TABLE_NAME = "gpkg_spatial_ref_sys";

ossimGpkgSpatialRefSysRecord::ossimGpkgSpatialRefSysRecord()
   :
   ossimGpkgDatabaseRecordBase(),
   m_srs_name(),
   m_srs_id(0),
   m_organization(),
   m_organization_coordsys_id(0),
   m_definition(),
   m_description()
{
}

ossimGpkgSpatialRefSysRecord::ossimGpkgSpatialRefSysRecord(const ossimGpkgSpatialRefSysRecord& obj)
   :
   ossimGpkgDatabaseRecordBase(), 
   m_srs_name(obj.m_srs_name),
   m_srs_id(obj.m_srs_id),
   m_organization(obj.m_organization),
   m_organization_coordsys_id(obj.m_organization_coordsys_id),
   m_definition(obj.m_definition),
   m_description(obj.m_description)
{
}

const ossimGpkgSpatialRefSysRecord& ossimGpkgSpatialRefSysRecord::operator=(
   const ossimGpkgSpatialRefSysRecord& obj)
{
   if ( this != &obj )
   {
      m_srs_name = obj.m_srs_name;
      m_srs_id = obj.m_srs_id;
      m_organization = obj.m_organization;
      m_organization_coordsys_id = obj.m_organization_coordsys_id;
      m_definition = obj.m_definition;
      m_description = obj.m_description;
   }
   return *this;
}

ossimGpkgSpatialRefSysRecord::~ossimGpkgSpatialRefSysRecord()
{
}

const std::string& ossimGpkgSpatialRefSysRecord::getTableName()
{
   return TABLE_NAME;
}

bool ossimGpkgSpatialRefSysRecord::init( sqlite3_stmt* pStmt )
{
   static const char M[] = "ossimGpkgSpatialRefSysRecord::init(pStmt)";
   
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
               if ( colName == "srs_name" )
               {
                  c = (const char*)sqlite3_column_text(pStmt, i);
                  m_srs_name = (c ? c : "");
                  ++columnsFound;
               }
               else if ( colName == "srs_id" )
               {
                  m_srs_id = sqlite3_column_int(pStmt, i);
                  ++columnsFound;
               }
               else if ( colName == "organization" )
               {
                  c = (const char*)sqlite3_column_text(pStmt, i);
                  m_organization = (c ? c : "");
                  ++columnsFound;
               }
               else if ( colName == "organization_coordsys_id" )
               {
                  m_organization_coordsys_id = sqlite3_column_int(pStmt, i);
                  ++columnsFound;
               }
               else if ( colName == "definition" )
               {
                  c = (const char*)sqlite3_column_text(pStmt, i);
                  m_definition = (c ? c : "");
                  ++columnsFound;
               }
               else if ( colName == "description" )
               {
                  c = (const char*)sqlite3_column_text(pStmt, i);
                  m_description = (c ? c : "" );
                  ++columnsFound;
               }               
               else
               {
                  ossimNotify(ossimNotifyLevel_WARN)
                     << M << " Unhandled column name[" << i << "]: "
                     << colName << std::endl;
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
   
} // End: ossimGpkgSpatialRefSysRecord::init( pStmt )

ossimGpkgSpatialRefSysRecord::InitCode
ossimGpkgSpatialRefSysRecord::init( sqlite3* db, const ossimMapProjection* proj )
{
   //---
   // Data taken from document:
   // National System for Geospatial-Intelligence (NSG) GeoPackage Encoding
   // Standard 1.0 Implementation Interoperability Standard (2014-10-29)
   //---
   ossimGpkgSpatialRefSysRecord::InitCode initCode = ossimGpkgSpatialRefSysRecord::ERROR;
   
   static const char M[] = "ossimGpkgSpatialRefSysRecord::init(proj)";

   if ( db && proj )
   {
      ossim_int32 pcsCode   = (ossim_int32)proj->getPcsCode();
      
      if ( pcsCode == 4326 )
      {
         m_srs_name = "WGS 84 Geographic 2D lat/lon";
         m_definition = "GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.01745329251994328,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4326\"]]";
         m_organization = "EPSG";
         m_organization_coordsys_id = 4326;
         m_description = "Horizontal component of 3D system. Used by the GPS satellite navigation system and for NATO military geodetic surveying.";

         initCode = ossimGpkgSpatialRefSysRecord::OK;
      }
      else if ( pcsCode == 3395 )
      {
         m_srs_name = "WGS 84 / World Mercator";
         m_definition = "PROJCS[\"WGS 84 / World Mercator\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.01745329251994328,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4326\"]],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],PROJECTION[\"Mercator_1SP\"],PARAMETER[\"central_meridian\",0],PARAMETER[\"scale_factor\",1],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],AUTHORITY[\"EPSG\",\"3395\"],AXIS[\"Easting\",EAST],AXIS[\"Northing\",NORTH]]";
         m_organization = "EPSG";
         m_organization_coordsys_id = 3395;
         m_description = "Mercator (1SP)";

         initCode = ossimGpkgSpatialRefSysRecord::OK;
      }
      else if ( pcsCode == 3857 )
      {
         m_srs_name = "WGS 84 / Pseudo-Mercator"; // ???
         m_definition = "PROJCS[\"WGS 84 / Pseudo-Mercator\",GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4326\"]],PROJECTION[\"Mercator_1SP\"],PARAMETER[\"central_meridian\",0],PARAMETER[\"scale_factor\",1],PARAMETER[\"false_easting\",0],PARAMETER[\"false_northing\",0],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"X\",EAST],AXIS[\"Y\",NORTH],EXTENSION[\"PROJ4\",\"+proj=merc +a=6378137 +b=6378137 +lat_ts=0.0 +lon_0=0.0 +x_0=0.0 +y_0=0 +k=1.0 +units=m +nadgrids=@null +wktext  +no_defs\"],AUTHORITY[\"EPSG\",\"3857\"]]";
         m_organization = "EPSG";
         m_organization_coordsys_id = 3857;
         m_description = "Google Projection"; // ?
         
         initCode = ossimGpkgSpatialRefSysRecord::OK;
      }
      else
      {
         ossimString projName = proj->getClassName();
         ossimString datumCode = proj->getDatum()->code();
         
         if ( projName == "ossimUtmProjection" )
         {
            if ( datumCode == "WGE" )
            {
               const ossimUtmProjection* utmProj =
                  dynamic_cast<const ossimUtmProjection*>( proj );
               if ( utmProj )
               {
                  ossimGpt origin = proj->origin();
                  ossimString centralMeridian = ossimString::toString( origin.lond() );
                  ossimString zone = ossimString::toString( utmProj->getZone() );
                  char hemisphere = utmProj->getHemisphere();
                  
                  m_srs_name = "WGS 84 / UTM zone ";
                  m_srs_name += zone.string();
                  if ( hemisphere == 'N' )
                  {
                     m_srs_name += "north";
                  }
                  else
                  {
                     m_srs_name += "south";
                  }
                  
                  m_definition = "PROJCS[\"WGS 84 / UTM zone ";
                  m_definition += zone.string();
                  m_definition += hemisphere;
                  m_definition += "\",GEOGCS[\"WGS 84\",DATUM[\"World Geodetic System_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.0174532925199433,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4326\"]],PROJECTION[\"Transverse Mercator\",AUTHORITY[\"EPSG\",\"9807\"]],PARAMETER[\"Latitude of natural origin\",0],PARAMETER[\"Longitude of natural origin\",";
                  m_definition += centralMeridian.string();
                  m_definition += "],PARAMETER[\"scale_factor\",0.9996],PARAMETER[\"false_easting\",500000],PARAMETER[\"false_northing\",";
                  if ( hemisphere == 'N' )
                  {
                     m_definition += "0";
                  }
                  else
                  {
                     m_definition += "10000000";
                  }
                  m_definition += "],UNIT[\"metre\",1,AUTHORITY[\"EPSG\",\"9001\"]],AXIS[\"Easting\",EAST],AXIS[\"Northing\",AUTHORITY[\"EPSG\",\"";
                  m_definition += ossimString::toString(pcsCode).string();
                  m_definition += "\"]]";
                  
                  m_organization = "EPSG";
                  m_organization_coordsys_id = pcsCode;
                  m_description = "Large and medium scale topographic mapping and engineering survey.";
                  initCode = ossimGpkgSpatialRefSysRecord::OK;
               }
               
            } // Matches: if ( datumCode == "WGE" )

            if ( initCode == ossimGpkgSpatialRefSysRecord::ERROR )
            {
               ossimNotify(ossimNotifyLevel_WARN)
                  << M
                  << "\nUnhandled projection: " << proj->getClassName()
                  << "\n datum: " << proj->getDatum()->code() << std::endl;
            } 
            
         } // Matches: if ( projName == "ossimUtmProjection" )
      }

      if ( initCode == ossimGpkgSpatialRefSysRecord::OK )
      {
         m_srs_id = getSrsId( db );
         if ( m_srs_id != -1 )
         {
            // Record was found in database.
            initCode = ossimGpkgSpatialRefSysRecord::OK_EXISTS;
         }
         else
         {
            m_srs_id = getNextSrsId( db );
         }
      }
      
   } // Matches: if ( proj )

   return initCode;
   
} // End: ossimGpkgSpatialRefSysRecord::init( proj )

bool ossimGpkgSpatialRefSysRecord::createTable( sqlite3* db )
{
   bool status = false;
   if ( db )
   {
      status = ossim_sqlite::tableExists( db, TABLE_NAME );

      if ( !status )
      {
         std::ostringstream sql;
         sql << "CREATE TABLE " << TABLE_NAME << " ( "
             << "srs_name TEXT NOT NULL, "
             << "srs_id INTEGER NOT NULL PRIMARY KEY, "
             << "organization TEXT NOT NULL, "
             << "organization_coordsys_id INTEGER NOT NULL, "
             << "definition  TEXT NOT NULL, "
             << "description TEXT )";
         
         if ( ossim_sqlite::exec( db, sql.str() ) == SQLITE_DONE )
         {
            // Requirement 11: Create the three required record entries.
            ossimGpkgSpatialRefSysRecord record;
            record.m_srs_id = -1;
            record.m_organization = "NONE";
            record.m_organization_coordsys_id = -1;
            record.m_definition = "undefined";
            
            if ( record.insert( db ) )
            {
               record.m_srs_id = 0;
               record.m_organization_coordsys_id = 0;
               
               if ( record.insert( db ) )
               {
                  record.m_srs_id = 1;
                  record.m_srs_name = "WGS 84 Geographic 2D lat/lon";
                  record.m_definition = "GEOGCS[\"WGS 84\",DATUM[\"WGS_1984\",SPHEROID[\"WGS 84\",6378137,298.257223563,AUTHORITY[\"EPSG\",\"7030\"]],AUTHORITY[\"EPSG\",\"6326\"]],PRIMEM[\"Greenwich\",0,AUTHORITY[\"EPSG\",\"8901\"]],UNIT[\"degree\",0.01745329251994328,AUTHORITY[\"EPSG\",\"9122\"]],AUTHORITY[\"EPSG\",\"4326\"]]";
                  record.m_organization = "EPSG";
                  record.m_organization_coordsys_id = 4326;
                  record.m_description = "Horizontal component of 3D system. Used by the GPS satellite navigation system and for NATO military geodetic surveying.";
                  
                  status = record.insert( db );
               }
            }
         }
      }
   }
   
   return status;
   
} // End: ossimGpkgSpatialRefSysRecord::createTable( sqlite3* db )

bool ossimGpkgSpatialRefSysRecord::insert( sqlite3* db )
{
   bool status = false;
   if ( db )
   {
      std::ostringstream sql;
      sql << "INSERT INTO gpkg_spatial_ref_sys VALUES ( "
          << "'" << m_srs_name << "', "
          << m_srs_id << ", "
          << "'" << m_organization << "', "
          << m_organization_coordsys_id << ", '"
          << m_definition << "', '"
          << m_description << "' )";

      if ( ossim_sqlite::exec( db, sql.str() ) == SQLITE_DONE )
      {
         status = true;
      }
   }
   return status;
}

void ossimGpkgSpatialRefSysRecord::saveState( ossimKeywordlist& kwl,
                                              const std::string& prefix ) const
{
   std::string myPref = prefix.size() ? prefix : std::string("gpkg_spatial_ref_sys.");
   std::string value;
   
   std::string key = "srs_name";
   kwl.addPair(myPref, key, m_srs_name, true);

   key = "srs_id";
   value = ossimString::toString(m_srs_id).string();
   kwl.addPair(myPref, key, value, true);

   key = "organization";
   kwl.addPair(myPref, key, m_organization, true);
   
   key = "organization_coordsys_id";
   value = ossimString::toString(m_organization_coordsys_id).string();
   kwl.addPair( myPref, key, value, true);

   key = "definition";
   kwl.addPair(myPref, key, m_definition, true);

   key = "description";
   kwl.addPair(myPref, key, m_description, true);
}

ossim_int32 ossimGpkgSpatialRefSysRecord::getSrsId( sqlite3* db )
{
   ossim_int32 result = -1;
   if ( db )
   {
      std::ostringstream sql;
      sql << "SELECT srs_id FROM gpkg_spatial_ref_sys WHERE organization == '"
          << m_organization
          << "' AND organization_coordsys_id == "
          << m_organization_coordsys_id;
      
      const char *zLeftover;                         // Tail of unprocessed SQL
      sqlite3_stmt *pStmt = 0;                       // The current SQL statement
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
            result = sqlite3_column_int(pStmt, 0);
         }
      }
      sqlite3_finalize(pStmt);
   }
   return result;
}

ossim_int32 ossimGpkgSpatialRefSysRecord::getNextSrsId( sqlite3* db )
{
   ossim_int32 result = 2; // -1, 0, and 1 are created by create table...
   if ( db )
   {
      std::string sql = "SELECT srs_id FROM gpkg_spatial_ref_sys ORDER BY srs_id DESC LIMIT 1";

      const char *zLeftover;                   // Tail of unprocessed SQL
      sqlite3_stmt *pStmt = 0;                 // The current SQL statement
      int rc = sqlite3_prepare_v2(db,          // Database handle
                                  sql.c_str(), // SQL statement, UTF-8 encoded
                                  -1,          // Maximum length of zSql in bytes.
                                  &pStmt,      // OUT: Statement handle
                                  &zLeftover); // OUT: Pointer to unused portion of zSql
      if ( rc == SQLITE_OK )
      {
         // Read the row:
         rc = sqlite3_step(pStmt);
         if ( rc == SQLITE_ROW )
         {
            result  = sqlite3_column_int(pStmt, 0);
            ++result;
         }
      }
      sqlite3_finalize(pStmt);
   }
   return result;
}
