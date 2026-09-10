//---
//
// License: MIT
//
// Description: Unit test for the Kakadu compressor's sample shift helpers.
//
// These helpers replaced expressions that were undefined behaviour:
//
//   (value << upshift) - (1<<31)  overflowed a signed int for every
//                                non-negative operand, which at -O3 the
//                                optimizer used to turn the following
//                                arithmetic right shift into a logical one
//   value << upshift             is undefined for a negative value in C++17
//
// Both are meant to compute their result on the two's complement
// representation. This test checks them against an independent reference
// computed in 64-bit arithmetic with multiplication rather than shifting, so
// the reference itself invokes no shift of a negative value.
//
// Build and run under UBSan to make the original defects' absence
// demonstrable. The pre-fix expressions raise, at -O3 and C++17:
//   "signed integer overflow: 65536000 - -2147483648 cannot be represented
//    in type 'int'"  and  "left shift of negative value -1"
// while this test reports 132416 checks, 0 failures with:
//   -std=c++17 -O3 -fsanitize=undefined,signed-integer-overflow
//
//---
// $Id$

#include "../src/ossimKakaduShift.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace
{
   int failures = 0;
   int checks   = 0;

   //---
   // Independent reference. Computes value * 2^upshift (and optionally the
   // -2^31 DC level shift) in 64-bit arithmetic, then reduces modulo 2^32 and
   // reinterprets as a signed 32-bit value -- i.e. exactly the two's
   // complement result the helpers are specified to produce.
   //---
   ossim_int32 reference( ossim_int64 value, int upshift, bool dcLevelShift )
   {
      ossim_int64 wide = value * ( static_cast<ossim_int64>(1) << upshift );
      if ( dcLevelShift )
      {
         wide -= static_cast<ossim_int64>(2147483648LL);
      }

      // Reduce modulo 2^32 without shifting a negative value:
      ossim_uint64 mod = static_cast<ossim_uint64>(wide) & 0xFFFFFFFFULL;
      ossim_uint32 bits = static_cast<ossim_uint32>(mod);
      return static_cast<ossim_int32>(bits);
   }

   void check( const char* what, ossim_int32 got, ossim_int32 want,
               ossim_int32 value, int upshift )
   {
      ++checks;
      if ( got != want )
      {
         ++failures;
         std::printf( "FAIL %s: value=%d upshift=%d got=%d want=%d\n",
                      what, value, upshift, got, want );
      }
   }
}

int main( int /*argc*/, char* /*argv*/[] )
{
   //---
   // Values that matter: the signed extremes, the boundary where an unsigned
   // 16-bit sample starts reading negative through a kdu_int16 alias (32768),
   // and the full-scale 16-bit sample (65535 -> -1).
   //---
   const ossim_int32 values[] =
   {
      0, 1, -1, 2, -2,
      127, 128, 255, 256,
      511, 512,
      32766, 32767, -32767, -32768,
      -32769, 65534, 65535, 65536,
      2147483647, (-2147483647 - 1)
   };
   const int numValues = static_cast<int>( sizeof(values)/sizeof(values[0]) );

   // upshift ranges over 32-src_bits and 16-src_bits in the transfer_*
   // routines, so 0 through 31 covers every case they can produce.
   for ( int i = 0; i < numValues; ++i )
   {
      for ( int upshift = 0; upshift <= 31; ++upshift )
      {
         const ossim_int32 v = values[i];

         check( "ossimKakaduSignedUpshift",
                ossimKakaduSignedUpshift( v, upshift ),
                reference( v, upshift, false ), v, upshift );

         check( "ossimKakaduDcLevelShift",
                ossimKakaduDcLevelShift( v, upshift ),
                reference( v, upshift, true ), v, upshift );
      }
   }

   //---
   // Property the compressor actually depends on: for an unsigned 16-bit
   // sample, the DC level shift must land the value at v - 2^15 once the
   // 32-bit result is shifted back down by 16. This is the property the
   // original defect inverted -- it produced v + 2^15, so every image read
   // back saturated white.
   //---
   for ( ossim_int64 v = 0; v <= 65535; ++v )
   {
      // As the compressor sees it: an unsigned sample aliased through a
      // signed 16-bit pointer.
      const ossim_int32 aliased =
         static_cast<ossim_int32>( static_cast<ossim_int16>( v ) );

      const ossim_int32 shifted = ossimKakaduDcLevelShift( aliased, 16 );
      const ossim_int32 recovered = shifted >> 16;
      const ossim_int32 want = static_cast<ossim_int32>( v ) - 32768;

      ++checks;
      if ( recovered != want )
      {
         ++failures;
         std::printf( "FAIL dc round trip: sample=%lld got=%d want=%d\n",
                      static_cast<long long>(v), recovered, want );
      }
   }

   //---
   // A signed sample takes no level shift, and upshift is 0 for 16-bit data
   // in the 16-bit buffer path, so the helper must be the identity there.
   //---
   for ( ossim_int32 v = -32768; v <= 32767; ++v )
   {
      ++checks;
      if ( ossimKakaduSignedUpshift( v, 0 ) != v )
      {
         ++failures;
         std::printf( "FAIL signed identity: value=%d got=%d\n",
                      v, ossimKakaduSignedUpshift( v, 0 ) );
      }
   }

   std::printf( "%d checks, %d failures\n", checks, failures );
   std::printf( "%s\n", failures ? "FAILED" : "PASSED" );

   return failures ? 1 : 0;
}
