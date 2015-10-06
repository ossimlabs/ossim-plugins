//----------------------------------------------------------------------------
// File: ossimGpkgDatabaseRecordInferface.h
// 
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Description: Base class definition for GeoPackage database record.
// 
//----------------------------------------------------------------------------
// $Id$

#include "ossimGpkgDatabaseRecordBase.h"
#include <ossim/base/ossimKeywordlist.h>
#include <ostream>

ossimGpkgDatabaseRecordBase::ossimGpkgDatabaseRecordBase()
   : ossimReferenced()
{
}

ossimGpkgDatabaseRecordBase::~ossimGpkgDatabaseRecordBase()
{
}

std::ostream& ossimGpkgDatabaseRecordBase::print(std::ostream& out) const
{
   ossimKeywordlist kwl;
   saveState(kwl, std::string(""));
   out << kwl;
   return out;
}
   
std::ostream& operator<<(std::ostream& out,
                         const ossimGpkgDatabaseRecordBase& obj)
{
   return obj.print( out );
}

   
