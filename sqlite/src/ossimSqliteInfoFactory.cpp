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

#include "ossimSqliteInfoFactory.h"
#include <ossim/support_data/ossimInfoFactory.h>
#include <ossim/support_data/ossimInfoBase.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimRefPtr.h>
#include "ossimGpkgInfo.h"

ossimSqliteInfoFactory::~ossimSqliteInfoFactory()
{}

ossimSqliteInfoFactory* ossimSqliteInfoFactory::instance()
{
   static ossimSqliteInfoFactory sharedInstance;

   return &sharedInstance;
}

std::shared_ptr<ossimInfoBase> ossimSqliteInfoFactory::create(const ossimFilename& file) const
{
   std::shared_ptr<ossimInfoBase> result = 0;

   std::string ext = file.ext().downcase().string();
   if ( ext == "gpkg" )
   {
      result = std::make_shared<ossimGpkgInfo>();
      if(result->open(file) == false)
      {
         result.reset();
      }
   }

   return result;
}

std::shared_ptr<ossimInfoBase> ossimSqliteInfoFactory::create(std::shared_ptr<ossim::istream>& str,
                                                              const std::string& connectionString)const
{
   std::shared_ptr<ossimInfoBase> result;



   return result;
}

ossimSqliteInfoFactory::ossimSqliteInfoFactory()
{}

ossimSqliteInfoFactory::ossimSqliteInfoFactory(const ossimSqliteInfoFactory& /* obj */ )
{}

const ossimSqliteInfoFactory& ossimSqliteInfoFactory::operator=(
   const ossimSqliteInfoFactory& /* rhs */)
{
   return *this;
}

