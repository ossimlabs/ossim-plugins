set(LIB_NAME ossim_mrsid_plugin)
MESSAGE( "************** LIBRARY SETUP FOR ossim_mrsid_plugin ******************")

# Include ossim cmake stuff:
include(OssimVersion)
include(OssimCommonVariables)
include(OssimUtilities)

set(requiredLibs)

find_package(MrSid)
if(MRSID_FOUND)
   include_directories(${CMAKE_CURRENT_SOURCE_DIR})
   include_directories(${MRSID_INCLUDE_DIR})
   set( requiredLibs ${requiredLibs} ${MRSID_LIBRARIES} )
else(MRSID_FOUND)
   message(FATAL_ERROR "Could not find mrsid")
endif(MRSID_FOUND)

MESSAGE( STATUS "MRSID_LIBRARIES     = ${MRSID_LIBRARIES}" )
MESSAGE( STATUS "MRSID_INCLUDE       = ${MRSID_INCLUDE_DIR}" )
MESSAGE( STATUS "MRSID_WRITE_FOUND   = ${MRSID_WRITE_FOUND}" )

find_package(GDAL)
if(GDAL_FOUND)
   include_directories(${GDAL_INCLUDE_DIR})
   set( requiredLibs ${requiredLibs} ${GDAL_LIBRARY} )
else(GDAL_FOUND)
    message( FATAL_ERROR "Could not find gdal" )
endif(GDAL_FOUND)

# OSSIM - Required: 
find_package(ossim)
if(OSSIM_FOUND)
   include_directories( ${OSSIM_INCLUDE_DIR} )
   set( requiredLibs ${requiredLibs} ${OSSIM_LIBRARIES} )
else(OSSIM_FOUND)
   message(FATAL_ERROR "Could not find ossim")
endif(OSSIM_FOUND)

####
# OSSIMPLUGINSMAKINGDLL controls dll linkage on windows.  
# Adding this sets OSSIM_PLUGINS_DLL #define TO "__declspec(dllexport)".
###
add_definitions("-DOSSIMPLUGINSMAKINGDLL")

FILE(GLOB_RECURSE OSSIMPLUGIN_SRCS *.cpp)
FILE(GLOB_RECURSE OSSIMPLUGIN_HEADERS *.h)

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

