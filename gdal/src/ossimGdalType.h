//------------------------------------------------------------------------
// License:  See top level LICENSE.txt file.
//
// Author:  Garrett Potts
//
// $Id: ossimGdalType.h 11440 2007-07-30 12:35:26Z dburken $
//------------------------------------------------------------------------
#ifndef ossimGdalType_HEADER
#define ossimGdalType_HEADER
#include <ossim/base/ossimConstants.h>
#include <gdal.h>

class ossimGdalType
{
public:
   ossimScalarType toOssim(GDALDataType gdalType)const;
   GDALDataType    toGdal(ossimScalarType)const;
};

#endif
