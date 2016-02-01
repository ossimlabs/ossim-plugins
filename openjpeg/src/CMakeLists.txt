set(LIB_NAME ossim_openjpeg_plugin)

message( "************** LIBRARY SETUP FOR ossim_openjpeg_plugin ******************")

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

# OpenJPEG required:
find_package(OpenJPEG)
if(OPENJPEG_FOUND)
   include_directories( ${OPENJPEG_INCLUDE_DIR} )
   SET(requiredLibs ${requiredLibs} ${OPENJPEG_LIBRARIES} )
else(OPENJPEG_FOUND)
   message(FATAL_ERROR "Could not find OpenJPEG")
endif(OPENJPEG_FOUND)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB OSSIMPLUGIN_SRCS *.cpp)
file(GLOB OSSIMPLUGIN_HEADERS *.h)

message( STATUS "OPENJPEG_INCLUDE_DIR = ${OPENJPEG_INCLUDE_DIR}")
message( STATUS "OPENJPEG_LIBRARIES   = ${OPENJPEG_LIBRARIES}")
message( STATUS "OPENJPEG plugin required libs = ${requiredLibs}")

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
		   LIBRARIES ${OSSIM_LIBRARY} ${requiredLibs}
                   HEADERS "${OSSIMPLUGIN_HEADERS}"
		   SOURCE_FILES "${OSSIMPLUGIN_SRCS}"
                   INSTALL_LIB)
