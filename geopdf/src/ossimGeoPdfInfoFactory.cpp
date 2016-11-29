//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Mingjie Su
//
// Description: Factory for info objects.
// 
//----------------------------------------------------------------------------
// $Id: ossimGeoPdfInfoFactory.cpp 19869 2011-07-23 13:25:34Z dburken $

#include <ossim/support_data/ossimInfoFactory.h>
#include <ossim/support_data/ossimInfoBase.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimRefPtr.h>

#include "ossimGeoPdfInfoFactory.h"
#include "ossimGeoPdfInfo.h"

ossimGeoPdfInfoFactory::~ossimGeoPdfInfoFactory()
{}

ossimGeoPdfInfoFactory* ossimGeoPdfInfoFactory::instance()
{
   static ossimGeoPdfInfoFactory sharedInstance;

   return &sharedInstance;
}

std::shared_ptr<ossimInfoBase> ossimGeoPdfInfoFactory::create(std::shared_ptr<ossim::istream>& str,
                                                              const std::string& connectionString) const
{
   std::shared_ptr<ossimInfoBase> result;


   return result;
}

std::shared_ptr<ossimInfoBase> ossimGeoPdfInfoFactory::create(const ossimFilename& file) const
{
   std::shared_ptr<ossimInfoBase> result;

   result = std::make_shared<ossimGeoPdfInfo>();
   if ( result->open(file) )
   {
      return result;
   }
   result.reset();
   
   return result;
}

ossimGeoPdfInfoFactory::ossimGeoPdfInfoFactory()
{}

ossimGeoPdfInfoFactory::ossimGeoPdfInfoFactory(const ossimGeoPdfInfoFactory& /* obj */ )
{}

const ossimGeoPdfInfoFactory& ossimGeoPdfInfoFactory::operator=(
   const ossimGeoPdfInfoFactory& /* rhs */)
{
   return *this;
}
