cmake_minimum_required (VERSION 2.8)

# Get the library suffix for lib or lib64.
get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)       
if(LIB64)
   set(LIBSUFFIX 64)
else()
   set(LIBSUFFIX "")
endif()


set(requiredLibs ${requiredLibs} ossim ossim_msp_plugin )

# Add the executable:
add_executable(msp-plugin-test msp-plugin-test.cpp )

# Set the output dir:
set_target_properties(msp-plugin-test 
                      PROPERTIES 
                      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

target_link_libraries( msp-plugin-test ${requiredLibs} )

