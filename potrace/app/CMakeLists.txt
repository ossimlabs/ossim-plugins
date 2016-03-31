message( "   Begin: CMAKE SETUP FOR ossim-potrace app " )

cmake_minimum_required (VERSION 2.8)

# Get the library suffix for lib or lib64.
get_property(LIB64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)       
if(LIB64)
   set(LIBSUFFIX 64)
else()
   set(LIBSUFFIX "")
endif()

find_package(ossim)
if(OSSIM_FOUND)
   include_directories( ${OSSIM_INCLUDE_DIR} )
   set( requiredLibs ${requiredLibs} ${OSSIM_LIBRARIES} )
else(OSSIM_FOUND)
   message(FATAL_ERROR "Could not find ossim")
endif(OSSIM_FOUND)

message( STATUS "Required libs       = ${requiredLibs}" )

# ---
# ossim-potrace app:
# ---
OSSIM_SETUP_APPLICATION(ossim-potrace INSTALL COMMAND_LINE COMPONENT_NAME ossim SOURCE_FILES ossim-potrace.cpp)
