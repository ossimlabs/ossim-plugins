cmake_minimum_required (VERSION 2.8)

# Get the library suffix for lib or lib64.
get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
if(LIB64)
   set(LIBSUFFIX 64)
else()
   set(LIBSUFFIX "")
endif()

# OSSIM - Required for the ossim_int32 / ossim_uint32 typedefs:
find_package(ossim)
if(OSSIM_FOUND)
   include_directories( ${OSSIM_INCLUDE_DIR} )
else(OSSIM_FOUND)
   message(FATAL_ERROR "Could not find ossim")
endif(OSSIM_FOUND)

#---
# ossim-kakadu-shift-test exercises the header-only sample shift helpers in
# ../src/ossimKakaduShift.h. It deliberately links nothing: the helpers are
# inline and Kakadu-free, so the arithmetic is testable without a Kakadu
# installation.
#---
add_executable(ossim-kakadu-shift-test ossim-kakadu-shift-test.cpp)
set_target_properties(ossim-kakadu-shift-test
   PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

#---
# enable_testing() must be called in this scope for add_test() to do anything.
# Nothing in the super-build calls it at top level -- ossim-maxar-plugin's
# test/CMakeLists.txt calls it itself for the same reason. Without this,
# add_test() below is silently a no-op: the binary builds into bin/ but no
# CTestTestfile.cmake is generated and ctest never sees the test.
#---
enable_testing()

add_test(NAME ossim-kakadu-shift-test
         COMMAND ossim-kakadu-shift-test)
