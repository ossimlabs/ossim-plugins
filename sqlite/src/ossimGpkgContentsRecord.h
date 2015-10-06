//----------------------------------------------------------------------------
//
// File: ossimGpkgContentsRecord.h
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: Container class for gpkg_contents table.
//
//----------------------------------------------------------------------------
// $Id$

#ifndef ossimGpkgContentsRecord_HEADER
#define ossimonGpkgContentsRecord_HEADER 1

#include "ossimGpkgDatabaseRecordBase.h"
#include <ossim/base/ossimConstants.h>

class ossimDpt;
class ossimMapProjection;
struct sqlite3;

class ossimGpkgContentsRecord : public ossimGpkgDatabaseRecordBase
{
public:

   /** @brief default constructor */
   ossimGpkgContentsRecord();

   /* @brief copy constructor */
   ossimGpkgContentsRecord(const ossimGpkgContentsRecord& obj);

   /* @brief assignment operator= */
   const ossimGpkgContentsRecord& operator=(const ossimGpkgContentsRecord& copy_this);

   /** @brief destructor */
   virtual ~ossimGpkgContentsRecord();

   /**
    * @brief Get the table name "gpkg_contents".
    * @return table name
    */
   static const std::string& getTableName();
   
   /**
    * @brief Initialize from database.
    * @param pStmt SQL statement, i.e. result of sqlite3_prepare_v2(...) call.
    * @return true on success, false on error.
    */
   virtual bool init( sqlite3_stmt* pStmt );

   /**
    * @brief Initialize from projection.
    * @param tableName e.g. "tiles"
    * @param srs_id ID of gpkg_spatial_ref_sys record our projection is
    * relative to.
    * @param minPt Minimum bounds in either Easting Northin or lat lon.
    * @param maxPt Maximum bounds in either Easting Northin or lat lon.    * 
    * @return true on success, false on error.
    */
   bool init( const std::string& tableName,
              ossim_int32 srs_id,
              const ossimDpt& minPt,
              const ossimDpt& maxPt );
   
   /**
    * @brief Creates  table in database.
    * @param db
    * @return true on success, false on error.
    */   
   static bool createTable( sqlite3* db );

   /**
    * @brief Inserst this record into gpkg_spatial_ref_sys table.
    * @param db
    * @return true on success, false on error.
    */   
   bool insert( sqlite3* db );
   
   /**
    * @brief Saves the state of object to keyword list.
    * @param kwl Initialized by this.
    * @param prefix e.g. "image0.". Can be empty.
    */
   virtual void saveState( ossimKeywordlist& kwl, const std::string& prefix ) const;

   std::string   m_table_name;
   std::string   m_data_type;
   std::string   m_identifier;
   std::string   m_description;
   std::string   m_last_change;
   ossim_float64 m_min_x;
   ossim_float64 m_min_y;
   ossim_float64 m_max_x;
   ossim_float64 m_max_y;
   ossim_int32   m_srs_id;
};

#endif /* #ifndef ossimGpkgContentsRecord_HEADER */
