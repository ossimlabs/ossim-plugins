//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author: David Burken
//
// Description: Factory for SQLite info objects.
// 
//----------------------------------------------------------------------------
// $Id$

#ifndef ossimSqliteInfoFactory_HEADER
#define ossimSqliteInfoFactory_HEADER

#include <ossim/base/ossimConstants.h>
#include <ossim/support_data/ossimInfoFactoryInterface.h>

class ossimFilename;
class ossimInfoBase;

/**
 * @brief Info factory.
 */
class ossimSqliteInfoFactory : public ossimInfoFactoryInterface
{
public:

   /** virtual destructor */
   virtual ~ossimSqliteInfoFactory();

   static ossimSqliteInfoFactory* instance();

   /**
    * @brief create method.
    *
    * @param file Some file you want info for.
    *
    * @return ossimInfoBase* on success 0 on failure.  Caller is responsible
    * for memory.
    */
   virtual ossimInfoBase* create(const ossimFilename& file) const;
   
private:
   
   /** hidden from use default constructor */
   ossimSqliteInfoFactory();

   /** hidden from use copy constructor */
   ossimSqliteInfoFactory(const ossimSqliteInfoFactory& obj);

   /** hidden from use operator = */
   const ossimSqliteInfoFactory& operator=(const ossimSqliteInfoFactory& rhs);
};

#endif /* End of "#ifndef ossimSqliteInfoFactory_HEADER" */
