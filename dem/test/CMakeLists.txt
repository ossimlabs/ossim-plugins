cmake_minimum_required (VERSION 2.8)

# Get the library suffix for lib or lib64.
get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)       
if(LIB64)
   set(LIBSUFFIX 64)
else()
   set(LIBSUFFIX "")
endif()

set(requiredLibs ${requiredLibs} ossim_dem_plugin )

message("requiredLibs = ${requiredLibs}")
add_executable(dem-test dem-test.cpp )
set_target_properties(dem-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
target_link_libraries( dem-test ${requiredLibs} )

