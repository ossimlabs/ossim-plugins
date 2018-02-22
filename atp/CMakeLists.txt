PROJECT(ossim-atp-plugin)
Message("################## Setting up ossim-atp-plugin  #########################")

cmake_minimum_required(VERSION 2.8)

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

set (CMAKE_CXX_STANDARD 11)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src )
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test )

# Share install targets (configuration JSON files):
FILE(GLOB ossim_atp_json ${${PROJECT_NAME}_SOURCE_DIR}/share/atp/*.json)
INSTALL(FILES ${ossim_atp_json} DESTINATION share/ossim/atp COMPONENT ossim)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/src/Config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/src/Config.h @ONLY)


message("################## end of ossim-atp-plugin setup #########################")
