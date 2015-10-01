//**************************************************************************************************
//
// OSSIM (http://trac.osgeo.org/ossim/)
//
// License:  LGPL -- See LICENSE.txt file in the top level directory for more details.
//
//**************************************************************************************************
// $Id: ossimRialtoReader.h 23401 2015-06-25 15:00:31Z okramer $

#ifndef ossimRialtoReader_HEADER
#define ossimRialtoReader_HEADER 1

#include "ossimPdalReader.h"
#include <ossim/plugin/ossimPluginConstants.h>
#include <rialto/RialtoReader.hpp>
#include <pdal/Options.hpp>

class ossimPointRecord;
#define USE_FULL_POINT_CLOUD_BUFFERING

class OSSIM_PLUGINS_DLL ossimRialtoReader : public ossimPdalReader
{
public:
   /** default constructor */
   ossimRialtoReader();

   /** virtual destructor */
   virtual ~ossimRialtoReader();

   /**
    * Accepts filename of Rialto database file.  Returns TRUE if successful.
    */
   virtual bool open(const ossimFilename& fname);

   /**
    * Riaalto implementation does not support direct file reads, so this method is stubbed out with
    * a warining message.
    */
   virtual void getFileBlock(ossim_uint32 offset, ossimPointBlock& block, ossim_uint32 np=0) const;

   /**
    * Fetches the block of points inside the block bounds. If the height components of the bounds
    * are NaN, then only the horizontal bounds are considered. Thread-safe version accepts data
    * block object from caller. The block object is cleared before points are pushed on the vector.
    * The block size will be non-zero if points were found.
    */
   virtual void getBlock(const ossimGrect& bounds, ossimPointBlock& block) const;

private:
   virtual void establishMinMax();

   virtual void establishAvailableFields() { establishMinMax(); }

TYPE_DATA
};

#endif /* #ifndef ossimPdalTileDbReader_HEADER */
