message( "************** Begin: CMAKE SETUP FOR pdal-plugin-test ******************" )

cmake_minimum_required (VERSION 2.8)

# Get the library suffix for lib or lib64.
get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)       
if(LIB64)
   set(LIBSUFFIX 64)
else()
   set(LIBSUFFIX "")
endif()


message( STATUS "Required libs       = ${requiredLibs}" )

# ---
# pdal-plugin-test app:
# ---

# Add the executable:
add_executable(ossim-pdal-plugin-test plugin-test.cpp )

# Set the output dir:
set_target_properties(ossim-pdal-plugin-test 
                      PROPERTIES 
                      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# pdal-plugin-test application links with required libs.
target_link_libraries( ossim-pdal-plugin-test ${requiredLibs} )

message( "************** End: CMAKE SETUP FOR pdal-plugin-test ******************" )
