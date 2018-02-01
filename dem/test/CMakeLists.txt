

cmake_minimum_required (VERSION 2.8)

# Get the library suffix for lib or lib64.
get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)       
if(LIB64)
   set(LIBSUFFIX 64)
else()
   set(LIBSUFFIX "")
endif()

set(requiredLibs ${requiredLibs} ossim_reg_plugin )

# Add the executable:
add_executable(match-test match-test.cpp )
add_executable(detect-test detect-test.cpp )

# Set the output dir:
set_target_properties(match-test detect-test
                      PROPERTIES 
                      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

target_link_libraries( match-test ${requiredLibs} )
target_link_libraries( detect-test ${requiredLibs} )

