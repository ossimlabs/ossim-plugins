set(LIB_NAME ossim_jpeg12_plugin)

message( "************** LIBRARY SETUP FOR ossim_jpeg12_plugin ******************")

# Include ossim cmake stuff:
include(OssimVersion)
include(OssimCommonVariables)
include(OssimUtilities)

set(requiredLibs)

# JPEG12 support this is required for this plugin:
find_package( JPEG12 )
if( JPEG12_FOUND )
   include_directories( BEFORE ${JPEG12_INCLUDE_DIR} )
   include_directories( BEFORE ${CMAKE_CURRENT_SOURCE_DIR} )
   set( requiredLibs ${requiredLibs} ${JPEG12_LIBRARY} )
   # This will turn on compiling of ossimNitfTileSource_12 class:
else( JPEG12_FOUND )
   message( FATAL_ERROR "Could not find required jpeg12 package!" )
endif( JPEG12_FOUND )

# OSSIM - Required: 
find_package(ossim)
if(OSSIM_FOUND)
   include_directories( ${OSSIM_INCLUDE_DIR} )
   set( requiredLibs ${requiredLibs} ${OSSIM_LIBRARIES} )
else(OSSIM_FOUND)
   message(FATAL_ERROR "Could not find ossim")
endif(OSSIM_FOUND)

file(GLOB OSSIMPLUGIN_SRCS *.cpp)
file(GLOB OSSIMPLUGIN_HEADERS *.h)

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
if(WIN32)
 set(INSTALL_RUNTIME_DIR "${INSTALL_RUNTIME_DIR}/ossim/plugins")
endif()

OSSIM_LINK_LIBRARY(${LIB_NAME}
                   COMPONENT_NAME ossim TYPE "${OSSIM_PLUGIN_LINK_TYPE}"
		   LIBRARIES ${requiredLibs}
                   HEADERS "${OSSIMPLUGIN_HEADERS}" 
		   SOURCE_FILES "${OSSIMPLUGIN_SRCS}"
                   INSTALL_LIB)
