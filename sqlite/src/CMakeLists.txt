set(LIB_NAME ossimsqlite_plugin)
MESSAGE( "************** LIBRARY SETUP FOR ossimsqlite_plugin******************")

# Include ossim cmake stuff:
include(OssimVersion)
include(OssimCommonVariables)
include(OssimUtilities)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

set(requiredLibs)

# SQLite - Required:
find_package(SQLITE)
if(SQLITE_FOUND)
   include_directories( ${SQLITE_INCLUDE_DIR} )
   SET(requiredLibs ${requiredLibs} ${SQLITE_LIBRARY})
else(SQLITE_FOUND)
   message(FATAL_ERROR "Could not find sqlite")
endif(SQLITE_FOUND)

# PNG - Required:
find_package(PNG)
IF(PNG_FOUND)
   include_directories( ${PNG_INCLUDE_DIR} )
   SET(requiredLibs ${requiredLibs} ${PNG_LIBRARIES} )
ELSE(PNG_FOUND)
   MESSAGE(FATAL_ERROR "Could not find png")
ENDIF(PNG_FOUND)

# JPEG - Required:
find_package( JPEG )
if( JPEG_FOUND )
   include_directories( ${JPEG_INCLUDE_DIR} )
   set( requiredLibs ${requiredLibs} ${JPEG_LIBRARY} )
else( JPEG_FOUND )
   message( FATAL_ERROR "Could not find jpeg" )
endif( JPEG_FOUND )

# OSSIM - Required: 
find_package(ossim)
if(OSSIM_FOUND)
   include_directories( ${OSSIM_INCLUDE_DIR} )
   set( requiredLibs ${requiredLibs} ${OSSIM_LIBRARIES} )
else(OSSIM_FOUND)
   message(FATAL_ERROR "Could not find ossim")
endif(OSSIM_FOUND)

message("requiredLibs: ${requiredLibs}")

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

OSSIM_LINK_LIBRARY(${LIB_NAME}
                   COMPONENT_NAME ossim TYPE "${OSSIM_PLUGIN_LINK_TYPE}"
		   LIBRARIES ${OSSIM_LIBRARY} ${requiredLibs}
                   HEADERS "${OSSIMPLUGIN_HEADERS}"
		   SOURCE_FILES "${OSSIMPLUGIN_SRCS}"
                   INSTALL_LIB)
