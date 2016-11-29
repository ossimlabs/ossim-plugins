//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Mingjie Su
//
// Description: Factory for Ogr info objects.
// 
//----------------------------------------------------------------------------
// $Id: ossimGdalInfoFactory.cpp 2645 2011-05-26 15:21:34Z oscar.kramer $

#include <ossimGdalInfoFactory.h>
#include <ossimOgrInfo.h>
#include <ossimHdfInfo.h>

#include <ossim/support_data/ossimInfoFactory.h>
#include <ossim/support_data/ossimInfoBase.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimRefPtr.h>

ossimGdalInfoFactory::~ossimGdalInfoFactory()
{}

ossimGdalInfoFactory* ossimGdalInfoFactory::instance()
{
   static ossimGdalInfoFactory sharedInstance;

   return &sharedInstance;
}

std::shared_ptr<ossimInfoBase> ossimGdalInfoFactory::create(const ossimFilename& file) const
{
   // Test hdf...
   std::shared_ptr<ossimInfoBase> result;

   std::shared_ptr<ossimHdfInfo> hdfInfo = std::make_shared<ossimHdfInfo>();
   if ( hdfInfo->open(file) )
   {
      return hdfInfo;
   }
   
   std::shared_ptr<ossimOgrInfo> ogrInfo = std::make_shared<ossimOgrInfo>();
   if ( ogrInfo->open(file) )
   {
      return ogrInfo;
   }

   return result;
}

std::shared_ptr<ossimInfoBase> ossimGdalInfoFactory::create(std::shared_ptr<ossim::istream>& str,
                                                            const std::string& connectionString)const
{
   std::shared_ptr<ossimInfoBase> result;

   // generic stream open/adaptors not implemented yet

   return result;
}

ossimGdalInfoFactory::ossimGdalInfoFactory()
{}

ossimGdalInfoFactory::ossimGdalInfoFactory(const ossimGdalInfoFactory& /* obj */ )
{}

const ossimGdalInfoFactory& ossimGdalInfoFactory::operator=(
   const ossimGdalInfoFactory& /* rhs */)
{
   return *this;
}
