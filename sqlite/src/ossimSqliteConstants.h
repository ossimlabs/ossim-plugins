//---
//
// License: MIT
//
// Description: Sqlite constants.
// 
//---
// $Id$

#ifndef ossimSqliteConstants_HEADER
#define ossimSqliteConstants_HEADER 1

namespace ossim
{
   enum TileType
   {
      UNKNOWN_TILE = 0,
      JPEG_TILE    = 1,
      PNG_TILE     = 2
   };

   enum WriterMode
   {
      UNKNOWN  = 0,
      JPEG     = 1,  // RGB, 8-bit, JPEG compressed
      PNG      = 2,  // PNG,
      PNGA     = 3,  // PNG with alpha
      MIXED    = 4,  // full tiles=jpeg, partials=pnga
      JPEGPNGA = 4,  // full tiles=jpeg, partials=pnga(same as mixed)
      JPEGPNG  = 5   // full tiles=jpeg, partials=png
   };
   
} // End: namespace ossim

#endif /* End of "#ifndef ossimSqliteConstants_HEADER" */
