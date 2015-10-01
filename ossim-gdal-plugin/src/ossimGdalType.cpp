//------------------------------------------------------------------------
// License:  LGPL
//
// See LICENSE.txt file in the top level directory for more details.
//
// Author:  Garrett Potts
//
// $Id: ossimGdalType.cpp 14786 2009-06-26 13:35:45Z dburken $
//------------------------------------------------------------------------

#include "ossimGdalType.h"
#include <ossim/base/ossimCommon.h>

ossimScalarType ossimGdalType::toOssim(GDALDataType gdalType)const
{
   switch(gdalType)
   {
      case GDT_Byte:
      {
         return OSSIM_UCHAR;
      }
      case GDT_UInt16:
      {
         return OSSIM_USHORT16;
      }
      case GDT_Int16:
      {
         return OSSIM_SSHORT16;
      }
      case GDT_Int32:
      {
         ossim_int32 sizeType = GDALGetDataTypeSize(gdalType)/8;
         if(sizeType == 2)
         {
            return OSSIM_SSHORT16;
         }
         break;
      }
      case GDT_Float32:
      {
         return OSSIM_FLOAT;
         break;
      }
      case GDT_Float64:
      {
         return OSSIM_DOUBLE;
         break;
      }
      default:
         break;
   }
   
   return OSSIM_SCALAR_UNKNOWN;
}

GDALDataType ossimGdalType::toGdal(ossimScalarType ossimType)const
{
   switch(ossimType)
   {
      case OSSIM_UCHAR:
      {
         return  GDT_Byte;
      }
      case OSSIM_USHORT11:
      case OSSIM_USHORT16:
      {
         return  GDT_UInt16;
      }
      case OSSIM_SSHORT16:
      {
         return GDT_Int16;
         break;
      }
      case OSSIM_FLOAT:
      case OSSIM_NORMALIZED_FLOAT:
      {
         return GDT_Float32;
      }
      case OSSIM_DOUBLE:
      case OSSIM_NORMALIZED_DOUBLE:
      {
         return GDT_Float64;
      }
      case OSSIM_SCALAR_UNKNOWN:
      {
         break;
      }
      default:
         break;
   }

   return GDT_Unknown;
}
