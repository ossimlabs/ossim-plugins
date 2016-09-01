if (NOT BUILD_OSSIM_HDF5_SUPPORT)
  return()
endif (NOT BUILD_OSSIM_HDF5_SUPPORT)

set(LIB_NAME ossim_hdf5_plugin)
MESSAGE( "************** LIBRARY SETUP FOR ossim_hdf5_plugin******************")

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
# Include ossim cmake stuff:
include(OssimVersion)
include(OssimCommonVariables)
include(OssimUtilities)

SET(requiredLibs)

set(HDF5_USE_STATIC_LIBRARIES FALSE)

find_package(HDF5A)
IF(HDF5_FOUND)
    INCLUDE_DIRECTORIES( ${HDF5_INCLUDE_DIR} )
    SET(requiredLibs ${requiredLibs} ${HDF5_LIBRARIES} )
ELSE(HDF5_FOUND)
    MESSAGE(FATAL_ERROR "Could not find hdf5")
ENDIF(HDF5_FOUND)

FILE(GLOB OSSIMPLUGIN_SRCS *.cpp)
FILE(GLOB OSSIMPLUGIN_HEADERS *.h)

####
# OSSIMPLUGINSMAKINGDLL controls dll linkage on windows.  
# Adding this sets OSSIM_PLUGINS_DLL #define TO "__declspec(dllexport)".
###
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

MESSAGE( STATUS "HDF5 Plugin required libs         = ${requiredLibs}" )
