set(LIB_NAME ossim_kakadu_plugin)

message( "************** LIBRARY SETUP FOR ossim_kakadu_plugin ******************")

# Include ossim cmake stuff:
include(OssimVersion)
include(OssimCommonVariables)
include(OssimUtilities)

set(requiredLibs)

# CURL - Optional:
find_package( CURL )
if( CURL_FOUND )
   include_directories( ${CURL_INCLUDE_DIR} )
   set( requiredLibs ${requiredLibs} ${CURL_LIBRARY} )
   add_definitions("-DKAKADU_USE_CURL=1")
else( CURL_FOUND )
   message( STATUS "*** curl support is disabled in kakadu plugin.  No Curl library found ***")
   add_definitions("-DKAKADU_USE_CURL=0")   
endif( CURL_FOUND )

# OpenThreads - Required:
find_package( OpenThreads )
if( OPENTHREADS_FOUND )
   include_directories( ${OPENTHREADS_INCLUDE_DIR} )
   set( requiredLibs ${requiredLibs} ${OPENTHREADS_LIBRARY} )
else( OPENTHREADS_FOUND )
   message( FATAL_ERROR "Could not find required OpenThreads package!" )
endif( OPENTHREADS_FOUND )

# TIFF - Required:
find_package( TIFF )
if( TIFF_FOUND )
   include_directories( ${TIFF_INCLUDE_DIR} )
   set( requiredLibs ${requiredLibs} ${TIFF_LIBRARY} )
else( TIFF_FOUND )
   message( FATAL_ERROR "Could not find required tiff package!" )
endif( TIFF_FOUND )

# OSSIM - Required: 
find_package(ossim)
if(OSSIM_FOUND)
   include_directories( ${OSSIM_INCLUDE_DIR} )
   set( requiredLibs ${requiredLibs} ${OSSIM_LIBRARIES} )
else(OSSIM_FOUND)
   message(FATAL_ERROR "Could not find ossim")
endif(OSSIM_FOUND)

if(KAKADU_ROOT_SRC)
   include_directories(${KAKADU_ROOT_SRC}/coresys
                       ${KAKADU_ROOT_SRC}/managed/all_includes)
else(KAKADU_ROOT_SRC)
   message(SEND_ERROR "Please pass in the KAKADU_ROOT_SRC and point it to the root source tree")
   set(KAKADU_ROOT_SRC "" CACHE STRING "Specify the root location where the kakadu source tree is located")
endif(KAKADU_ROOT_SRC)

if(KAKADU_AUX_LIBRARY)
   set(requiredLibs ${requiredLibs} ${KAKADU_AUX_LIBRARY})
else(KAKADU_AUX_LIBRARY)
   message(SEND_ERROR "Please pass in the KAKADU_AUX_LIBRARY and point it to the library")
   set(KAKADU_AUX_LIBRARY "" CACHE STRING "Specify the full path to the library to link to")
endif(KAKADU_AUX_LIBRARY)

if(KAKADU_LIBRARY)
   set(requiredLibs ${requiredLibs} ${KAKADU_LIBRARY})
else(KAKADU_LIBRARY)
   message(SEND_ERROR "Please pass in the KAKADU_LIBRARY and point it to the library")
   set(KAKADU_LIBRARY "" CACHE STRING "Specify the full path to the library to link to")
endif(KAKADU_LIBRARY)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB OSSIMPLUGIN_SRCS *.cpp)
file(GLOB OSSIMPLUGIN_HEADERS *.h)

message( STATUS "KAKADU_ROOT_SRC    = ${KAKADU_ROOT_SRC}")
message( STATUS "KAKADU_LIBRARY     = ${KAKADU_LIBRARY}")
message( STATUS "KAKADU_AUX_LIBRARY = ${KAKADU_LIBRARY}")
message( STATUS "Kakadu plugin required libs = ${requiredLibs}")

#---
# OSSIMPLUGINSMAKINGDLL controls dll linkage on windows.  
# Adding this sets OSSIM_PLUGINS_DLL #define TO "__declspec(dllexport)".
#---
add_definitions("-DOSSIMPLUGINSMAKINGDLL")

#---
# Kakadu library compatibilty define:
# This must match whatever kakadu library was compiled with.
# Look in Kakadu_library_root/coresys/make/Makefile-xxxx.
#---
add_definitions("-DKDU_X86_INTRINSICS")
add_definitions("-DKDU_NO_AVX2")

# Adjust the install library path:
if(OSSIM_INSTALL_PLUGINS_WITH_VERSION)
   set(INSTALL_LIBRARY_DIR   "${INSTALL_LIBRARY_DIR}/ossim-${OSSIM_VERSION}/plugins")
else(OSSIM_INSTALL_PLUGINS_WITH_VERSION)
   set(INSTALL_LIBRARY_DIR   "${INSTALL_LIBRARY_DIR}/ossim/plugins")
endif(OSSIM_INSTALL_PLUGINS_WITH_VERSION)

OSSIM_LINK_LIBRARY(${LIB_NAME}
                   COMPONENT_NAME ossim TYPE "${OSSIM_PLUGIN_LINK_TYPE}"
		   LIBRARIES ${requiredLibs}
                   HEADERS "${OSSIMPLUGIN_HEADERS}" 
		   SOURCE_FILES "${OSSIMPLUGIN_SRCS}"
                   INSTALL_LIB)
