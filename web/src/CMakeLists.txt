set(LIB_NAME ossim_web_plugin)
MESSAGE( "**************START  LIBRARY SETUP FOR ossim_web_plugin ******************")

# Include ossim cmake stuff:
include(OssimVersion)
include(OssimCommonVariables)
include(OssimUtilities)

find_package(ossim)
find_package(TIFF)
find_package(CURL)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${TIFF_INCLUDE_DIR})
INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIR})

FILE(GLOB OSSIMPLUGIN_SRCS *.cpp)
FILE(GLOB OSSIMPLUGIN_HEADERS *.h)

MESSAGE( STATUS "OSSIM_LIBRARIES = ${OSSIM_LIBRARIES}")

IF(NOT CURL_INCLUDE_DIR)
  MESSAGE( STATUS "************ OSSIM WEB PLUGIN CAN'T BE COMPILED, NO CURL SUPPORT FOUND*************")
  INCLUDE_DIRECTORIES(${CURL_INCLUDE_DIR})
ENDIF()

MESSAGE( STATUS "CURL_INCLUDE_DIR = ${CURL_INCLUDE_DIR}")
MESSAGE( STATUS "CURL_LIBRARIES   = ${CURL_LIBRARIES}")

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
		   LIBRARIES ${OSSIM_LIBRARIES} ${TIFF_LIBRARY} ${CURL_LIBRARIES}
                   HEADERS "${OSSIMPLUGIN_HEADERS}" 
		   SOURCE_FILES "${OSSIMPLUGIN_SRCS}"
                   INSTALL_LIB)

MESSAGE( "************** END LIBRARY SETUP FOR ossimweb_plugin******************")
