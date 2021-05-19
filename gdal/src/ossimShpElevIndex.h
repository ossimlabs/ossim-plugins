//----------------------------------------------------------------------------
//
// File: ossimGdalImageElevationDatabase.h
// 
// License:  MIT
// 
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Scott Bortman
//
// Description: See description for class below.
//
//----------------------------------------------------------------------------
// $Id$

#ifndef ossimShpElevIndex_HEADER
#define ossimShpElevIndex_HEADER 1
#include "ogrsf_frmts.h"
#include <ossim/base/ossimGrect.h>
#include <atomic>
#include <iostream>
#include <future>
#include <mutex>
#include <thread>

class ossimShpElevIndex
{
public:
  static ossimShpElevIndex* getInstance( std::string shpFileName )
  {
    ossimShpElevIndex* shpElevIndex = instance.load( std::memory_order_acquire );

    if ( ! shpElevIndex )
    {
      std::lock_guard<std::mutex> myLock( myMutex );

      shpElevIndex = instance.load( std::memory_order_relaxed );

      if( ! shpElevIndex )
      {
        shpElevIndex = new ossimShpElevIndex( shpFileName );
        instance.store( shpElevIndex, std::memory_order_release );
      }
    }   

    return shpElevIndex;
  }

  std::string searchShapefile( double lon, double lat ) const;
  void getBoundingRect(ossimGrect& rect) const;


private:
  ossimShpElevIndex( std::string shpFileName );
  ~ossimShpElevIndex();
  ossimShpElevIndex( const ossimShpElevIndex& )= delete;
  ossimShpElevIndex& operator=( const ossimShpElevIndex& )= delete;

  static std::atomic<ossimShpElevIndex*> instance;
  static std::mutex myMutex;

  GDALDataset *poMemDS;
};
#endif

