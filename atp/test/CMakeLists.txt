cmake_minimum_required (VERSION 2.8)

# Get the library suffix for lib or lib64.
get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)       
if(LIB64)
   set(LIBSUFFIX 64)
else()
   set(LIBSUFFIX "")
endif()

set(requiredLibs ${requiredLibs} ossim_atp_plugin )

message("requiredLibs = ${requiredLibs}")
add_executable(atp-config-test atp-config-test.cpp )
set_target_properties(atp-config-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
target_link_libraries( atp-config-test ${requiredLibs} )

add_executable(atp-test atp-test.cpp )
set_target_properties(atp-test PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
target_link_libraries( atp-test ${requiredLibs} )

add_executable(atp-residuals atp-residuals.cpp)
set_target_properties(atp-residuals PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
target_link_libraries(atp-residuals ${requiredLibs} )

