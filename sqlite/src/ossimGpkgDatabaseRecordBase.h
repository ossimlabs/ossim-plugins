//----------------------------------------------------------------------------
// File: ossimGpkgDatabaseRecordInferface.h
// 
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: Base class declaration for GeoPackage database record.
// 
//----------------------------------------------------------------------------
// $Id$

#ifndef ossimGpkgDatabaseRecordBase_HEADER
#define ossimGpkgDatabaseRecordBase_HEADER 1

#include <ossim/base/ossimReferenced.h>
#include <iosfwd>
#include <string>

class ossimKeywordlist;
struct sqlite3_stmt;

/** @class ossimGpkgDatabaseRecordBase */
class ossimGpkgDatabaseRecordBase : public ossimReferenced
{
public:
   
   /** @brief default constructor */
   ossimGpkgDatabaseRecordBase();

   /** @brief virtual destructor. */
   virtual ~ossimGpkgDatabaseRecordBase();

   /**
    * @brief Initialize from database.
    * Pure virtual, derived classes must implement.
    * @param pStmt SQL statement, i.e. result of sqlite3_prepare_v2(...) call.
    */
   virtual bool init( sqlite3_stmt* pStmt ) = 0;

   /**
    * @brief Saves the state of object to keyword list.
    * Pure virtual, derived classes must implement. 
    * @param kwl Initialized by this.
    * @param prefix e.g. "image0.". Can be empty.
    */
   virtual void saveState( ossimKeywordlist& kwl,
                           const std::string& prefix ) const = 0;

   /**
    * @brief Print method.
    * @param out Stream to print to.
    * @return Stream reference.
    */
   virtual std::ostream& print(std::ostream& out) const;
   
   /**
    * @brief Convenience operator << function.
    * @param out Stream to print to.
    * @param obj Object to print.
    */
   friend std::ostream& operator<<(std::ostream& out,
                                   const ossimGpkgDatabaseRecordBase& obj);
};

#endif /* #ifndef ossimGpkgDatabaseRecordBase_HEADER */
