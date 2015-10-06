set(LIB_NAME ossimgeopdf_plugin)
MESSAGE( "************** LIBRARY SETUP FOR ossimgeopdf_plugin******************")

# Include ossim cmake stuff:
include(OssimVersion)
include(OssimCommonVariables)
include(OssimUtilities)

find_package(ossim)
find_package(OpenThreads)
find_package(TIFF)
find_package(Podofo)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${OPENTHREADS_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${TIFF_INCLUDE_DIR})

IF(NOT PODOFO_INCLUDE_DIR)
        MESSAGE(SEND_ERROR "Please pass in the PODOFO_INCLUDE_DIR and point it to the includes")
	SET(PODOFO_INCLUDE_DIR "" CACHE STRING "Specify the full path to the includes")
ENDIF(NOT PODOFO_INCLUDE_DIR)

IF(NOT PODOFO_LIBRARY)
        MESSAGE(SEND_ERROR "Please pass in the PODOFO_LIBRARY and point it to the library")
	SET(PODOFO_LIBRARY "" CACHE STRING "Specify the full path to the library to link to")
ENDIF(NOT PODOFO_LIBRARY)

INCLUDE_DIRECTORIES( ${PODOFO_INCLUDE_DIR} )

MESSAGE( STATUS "PODOFO_INCLUDE_DIR = ${PODOFO_INCLUDE_DIR}")
MESSAGE( STATUS "PODOFO_LIBRARY = ${PODOFO_LIBRARY}")

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

OSSIM_LINK_LIBRARY(${LIB_NAME}
                   COMPONENT_NAME ossim TYPE "${OSSIM_PLUGIN_LINK_TYPE}"
		   LIBRARIES ${OSSIM_LIBRARIES} ${OPENTHREADS_LIBRARY} ${PODOFO_LIBRARY} ${TIFF_LIBRARY}
                   HEADERS "${OSSIMPLUGIN_HEADERS}"
		   SOURCE_FILES "${OSSIMPLUGIN_SRCS}"
                   INSTALL_LIB)
