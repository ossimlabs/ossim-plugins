//----------------------------------------------------------------------------
//
// License:  LGPL
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author: David Burken
//
// Copied from Mingjie Su's ossimHdfInfoFactory
//
// Description: Factory for info objects.
// 
//----------------------------------------------------------------------------
// $Id$

#include "ossimH5InfoFactory.h"
#include <ossim/support_data/ossimInfoFactory.h>
#include <ossim/support_data/ossimInfoBase.h>
#include <ossim/base/ossimFilename.h>
#include <ossim/base/ossimRefPtr.h>
#include "ossimH5Info.h"

ossimH5InfoFactory::~ossimH5InfoFactory()
{}

ossimH5InfoFactory* ossimH5InfoFactory::instance()
{
   static ossimH5InfoFactory sharedInstance;

   return &sharedInstance;
}

ossimInfoBase* ossimH5InfoFactory::create(const ossimFilename& file) const
{
   ossimRefPtr<ossimInfoBase> result = 0;

   // cout << "Calling ossimH5Info ***********************" << endl;
   result = new ossimH5Info();
   if ( result->open(file) )
   {
      return result.release();
   }

   return 0;
}

ossimH5InfoFactory::ossimH5InfoFactory()
{}

ossimH5InfoFactory::ossimH5InfoFactory(const ossimH5InfoFactory& /* obj */ )
{}

const ossimH5InfoFactory& ossimH5InfoFactory::operator=(
   const ossimH5InfoFactory& /* rhs */)
{
   return *this;
}

