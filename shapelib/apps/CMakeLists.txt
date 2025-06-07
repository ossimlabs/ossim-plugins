message( "************** Begin: Cmake setup for shapelib plugin apps ******************" )

# Include ossim cmake stuff:
include(OssimVersion)
include(OssimCommonVariables)
include(OssimUtilities)

set(requiredLibs)

# Get the library suffix for lib or lib64.
get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)       
if(LIB64)
   set(LIBSUFFIX 64)
else()
   set(LIBSUFFIX "")
endif()

# OSSIM - Required: 
find_package(ossim)
if(OSSIM_FOUND)
   include_directories( ${OSSIM_INCLUDE_DIR} )
   set( requiredLibs ${requiredLibs} ${OSSIM_LIBRARY} )
else(OSSIM_FOUND)
   message(FATAL_ERROR "Could not find ossim")
endif(OSSIM_FOUND)

# Requires shapelib:
find_package(Shapelib)
if (SHAPELIB_FOUND)
   include_directories( ${SHAPELIB_INCLUDE_DIR} )
   set(requiredLibs ${requiredLibs} ${SHAPELIB_LIBRARY} )
else(SHAPELIB_FOUND)
   message(FATAL_ERROR "Could not find SHAPELIB!")
endif(SHAPELIB_FOUND)

# Add our include...
include_directories(".")

message( STATUS "SHAPELIB_INCLUDE_DIR = ${SHAPELIB_INCLUDE_DIR}" )
message( STATUS "SHAPELIB_LIBRARY     = ${SHAPELIB_LIBRARY}" )
message( STATUS "OSSIM_INCLUDE_DIR    = ${OSSIM_INCLUDE_DIR}" )
message( STATUS "OSSIM_LIBRARY        = ${OSSIM_LIBRARY}" )
message( STATUS "Required libs        = ${requiredLibs}" )

# ---
# ossim-img2shp app:
# ---

# Add the executable:
add_executable( ossim-img2shp ossim-img2shp.cpp )

# Set the output dir:
set_target_properties( ossim-img2shp
                       PROPERTIES 
                       RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin" )

# ossim-web-chipper application links with required libs.
target_link_libraries( ossim-img2shp ${requiredLibs} )

# Install app:
install( TARGETS ossim-img2shp DESTINATION bin )

# Add uninstall target:

message( "************** End: Cmake setup for shapelib plugin apps ******************" )
