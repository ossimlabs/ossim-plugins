MESSAGE( "************** LIBRARY SETUP FOR ossim_dem_plugin******************")

# Include ossim cmake stuff:
include(OssimVersion)
include(OssimCommonVariables)
include(OssimUtilities)

find_package( JsonCpp )
if( JSONCPP_FOUND )
   include_directories( ${JSONCPP_INCLUDE_DIR} )
   set( requiredLibs ${requiredLibs} ${JSONCPP_LIBRARY} )
   message("Found JsonCpp" )
else( JSONCPP_FOUND )
   message( FATAL_ERROR "Could not find required JsonCpp package!" )
endif( JSONCPP_FOUND )

message(" OPENCV_HOME = <${OPENCV_HOME}>")
find_package(OpenCV)
if( OPENCV_FOUND )
   include_directories( ${OPENCV_INCLUDE_DIR} )
   set( requiredLibs ${requiredLibs} ${OPENCV_LIBRARIES} )
   message( "Found OpenCV" )
else( OPENCV_FOUND )
   message( FATAL "Could not find OpenCV package. " )
endif( OPENCV_FOUND )

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/src)

#IF(BUILD_OSSIM_TESTS)
#   add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test ${CMAKE_CURRENT_BINARY_DIR}/test)
#ENDIF()


