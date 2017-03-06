set(LIB_NAME ossimcsm3_plugin)
MESSAGE( "************** LIBRARY SETUP FOR ossimcsm_plugin******************")

# Include ossim cmake stuff:
include(OssimVersion)
include(OssimCommonVariables)
include(OssimUtilities)

set(requiredLibs)

# OSSIM - Required: 
find_package(ossim)
if(OSSIM_FOUND)
   include_directories( ${OSSIM_INCLUDE_DIR} )
   set( requiredLibs ${requiredLibs} ${OSSIM_LIBRARIES} )
else(OSSIM_FOUND)
   message(FATAL_ERROR "Could not find ossim")
endif(OSSIM_FOUND)

# OpenThreads - Required:
find_package( OpenThreads )
if( OPENTHREADS_FOUND )
   include_directories( ${OPENTHREADS_INCLUDE_DIR} )
   set( requiredLibs ${requiredLibs} ${OPENTHREADS_LIBRARY} )
else( OPENTHREADS_FOUND )
   message( FATAL_ERROR "Could not find required OpenThreads package!" )
endif( OPENTHREADS_FOUND )

# csmApi - Required:

find_package(csmApi)
if (CSMAPI_FOUND)
   include_directories(${CSMAPI_INCLUDES})
   set( requiredLibs ${requiredLibs} ${CSMAPI_LIBRARIES} )
else(CSMAPI_FOUND)
   message(FATAL_ERROR "Could not find csmApi")
endif(CSMAPI_FOUND)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

FILE(GLOB OSSIMPLUGIN_SRCS *.cpp)
FILE(GLOB OSSIMPLUGIN_HEADERS *.h)

#---
# OSSIMPLUGINSMAKINGDLL controls dll linkage on windows.  
# Adding this sets OSSIM_PLUGINS_DLL #define TO "__declspec(dllexport)".
#---
add_definitions("-DOSSIMPLUGINSMAKINGDLL")

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
