//----------------------------------------------------------------------------
//
// File: ossimGpkgSpatialRefSysRecord.h
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: Container class for gpkg_spatial_ref_sys table.
//
//----------------------------------------------------------------------------
// $Id$

#ifndef ossimGpkgSpatialRefSysRecord_HEADER
#define ossimGpkgSpatialRefSysRecord_HEADER 1

#include "ossimGpkgDatabaseRecordBase.h"
#include <ossim/base/ossimConstants.h>

class ossimMapProjection;
struct sqlite3;

class ossimGpkgSpatialRefSysRecord : public ossimGpkgDatabaseRecordBase
{
public:

   enum InitCode
   {
      ERROR     = -1, // Something bad...
      OK_EXISTS =  0, // Initialized and entry exist in database.
      OK        =  1  // Initialized but not in database yet.
   };

   /** @brief default constructor */
   ossimGpkgSpatialRefSysRecord();

   /* @brief copy constructor */
   ossimGpkgSpatialRefSysRecord(const ossimGpkgSpatialRefSysRecord& obj);

   /* @brief assignment operator= */
   const ossimGpkgSpatialRefSysRecord& operator=(const ossimGpkgSpatialRefSysRecord& obj);

   /** @brief destructor */
   virtual ~ossimGpkgSpatialRefSysRecord();

   /**
    * @brief Get the table name "gpkg_spatial_ref_sys".
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
    * @param db
    * @param proj Output map projection.
    * @return init code.
    * @see ossimGpkgSpatialRefSysRecordInitCode enumeration for more.
    */
   InitCode init( sqlite3* db, const ossimMapProjection* proj );

   /**
    * @brief Creates gpkg_contents table in database.
    * @param db
    * @return true on success, false on error.
    */   
   static bool createTable( sqlite3* db );

   /**
    * @brief Inserst this record into gpkg_contents table.
    * @param db
    * @return true on success, false on error.
    */   
   bool insert( sqlite3* db );   

   /**
    * @brief Saves the state of object.
    * @param kwl Initialized by this.
    * @param prefix e.g. "image0.". Can be empty.
    */
   virtual void saveState( ossimKeywordlist& kwl, const std::string& prefix ) const;

   /**
    * @brief Looks in database for matching record and returns the id if found.
    * @param db
    * @return srs_id if found; else, -1.
    */
   ossim_int32 getSrsId( sqlite3* db );

   /**
    * @brief Looks in database for matching record and returns the id if found.
    * @param db
    * @return srs_id if found; else, -1.
    */
   ossim_int32 getNextSrsId( sqlite3* db );

   std::string   m_srs_name;
   ossim_int32   m_srs_id;
   std::string   m_organization;
   ossim_int32   m_organization_coordsys_id;
   std::string   m_definition;
   std::string   m_description;
};

#endif /* #ifndef ossimGpkgSpatialRefSysRecord_HEADER */
