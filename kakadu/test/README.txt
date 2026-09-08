Tests for the ossim kakadu plugin.

ossim-kakadu-shift-test
   Unit test for the sample shift helpers in ../src/ossimKakaduShift.h.
   Links nothing and needs no Kakadu installation or imagery. Run it under
   UBSan (-fsanitize=undefined,signed-integer-overflow) to demonstrate that
   the shift arithmetic is free of the undefined behaviour it replaced.
