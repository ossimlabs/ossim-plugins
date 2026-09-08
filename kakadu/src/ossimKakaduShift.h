//---
//
// License: MIT
//
// Description: Sample shift helpers for the Kakadu compressor's transfer_*
// routines.
//
// Both helpers perform their shift on the two's complement representation, in
// unsigned arithmetic, where the wrap is defined. The signed spellings they
// replace are undefined behaviour:
//
//   (value << upshift) - (1<<31)   overflows a signed int for every
//                                 non-negative operand
//   value << upshift              is undefined for a negative value in
//                                 C++17; it became well defined only in
//                                 C++20, and this tree builds as C++17
//
// Deliberately free of Kakadu headers so the arithmetic can be unit tested
// without a Kakadu installation. kdu_int32 and kdu_uint32 are int and
// unsigned int, the same types as ossim_int32 and ossim_uint32.
//
//---
// $Id$

#ifndef ossimKakaduShift_HEADER
#define ossimKakaduShift_HEADER 1

#include <ossim/base/ossimConstants.h>

//---
// Applies the JPEG2000 DC level shift for UNSIGNED samples: computes
// (value << upshift) - 2^31 without signed overflow.
//
// Doing that subtraction in signed arithmetic overflows for every
// non-negative operand. It is undefined behaviour, and at -O3 the optimizer
// used it to turn the arithmetic right shift that follows into a logical one,
// so each sample was encoded as v + 2^15 instead of v - 2^15. Decoders then
// applied their own level shift and clipped, producing an all-white image.
//---
inline ossim_int32 ossimKakaduDcLevelShift( ossim_int32 value, int upshift )
{
   const ossim_uint32 shifted = static_cast<ossim_uint32>(value) << upshift;
   return static_cast<ossim_int32>( shifted - 0x80000000u );
}

//---
// Left-shifts a SIGNED sample: computes value << upshift on the two's
// complement representation.
//
// Signed samples take no DC level shift -- JPEG2000 applies it to unsigned
// data only, since signed samples are already centred on zero -- so this is
// the signed counterpart to ossimKakaduDcLevelShift above, carrying no 2^31
// term.
//
// Left-shifting a negative signed value is undefined behaviour in C++17.
// Every real compiler shifts the two's complement representation, which is
// what this computes explicitly, so the emitted arithmetic is unchanged; what
// changes is that it is no longer the optimizer's licence to assume the
// operand is non-negative.
//---
inline ossim_int32 ossimKakaduSignedUpshift( ossim_int32 value, int upshift )
{
   return static_cast<ossim_int32>(
      static_cast<ossim_uint32>(value) << upshift );
}

#endif /* #ifndef ossimKakaduShift_HEADER */
