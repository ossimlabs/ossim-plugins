Tests for the ossim kakadu plugin.

ossim-kakadu-shift-test
   Unit test for the sample shift helpers in ../src/ossimKakaduShift.h.
   Links nothing and needs no Kakadu installation or imagery. Run it under
   UBSan (-fsanitize=undefined,signed-integer-overflow) to demonstrate that
   the shift arithmetic is free of the undefined behaviour it replaced.

   Registered with ctest. Note that enable_testing() has to be called in
   test/CMakeLists.txt itself: nothing in the surrounding build calls it at
   top level, and without it add_test() is silently a no-op -- the binary
   still builds into bin/, but no CTestTestfile.cmake is generated and ctest
   never runs it.


Signed vs unsigned coverage
---------------------------

Asked during review of the two shift fixes: were both signed and unsigned
data tested?

At the helper level, yes, and exhaustively -- 132416 checks, 0 failures. The
test compares both helpers against a reference computed in 64-bit arithmetic
by multiplication, so the reference itself never shifts a negative value. It
covers:

  * the signed extremes;
  * 32768, the boundary where an unsigned 16-bit sample starts reading
    negative through the kdu_int16 alias;
  * full-scale 65535;
  * every upshift 0 through 31 (the only values the transfer routines can
    produce);
  * the full 0..65535 DC level shift round trip;
  * the signed identity at upshift 0.

End to end, through real imagery, the coverage is unsigned only -- and this
is a property of available imagery, not an oversight. The defect these fixes
address requires a NEGATIVE operand reaching the shift, and there are only
two routes to one:

  * OSSIM_SINT16 carrying genuinely negative samples; or
  * OSSIM_UINT16 carrying samples above 32767, which read negative through
    the kdu_int16 alias.

Every product available to us is 11-bit data in a 16-bit container, so
samples never exceed 2047 and are never negative. Converting such a product
with --output-radiometry S16 does exercise the signed BRANCH, but with
non-negative operands only, where the shift is well defined and the bug
cannot appear. Reaching either route needs synthetic data.

That is why the coverage lives at the helper level rather than in an
end-to-end product test: the unit test can construct the operands that real
imagery cannot, and it does, including the 32768 alias boundary named above.

Neither fix was prompted by an observed miscompile. 43e7cae was -- there the
optimizer turned an arithmetic right shift into a logical one at -O3, and
every 16-bit J2K product read back solid white. 6beec92 is the sibling
construct: nothing is known to miscompile today, but the same licence the
optimizer already took once is still granted, and left-shifting a negative
signed value is undefined in C++17 (defined only from C++20; ossim pins
CMAKE_CXX_STANDARD 17).
