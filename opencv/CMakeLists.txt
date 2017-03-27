find_package( OpenCV )
if( OPENCV_FOUND )
    include_directories( ${OPENCV_INCLUDE_DIR} )
    set( requiredLib ${requiredLib} ${OPENCV_LIBRARIES} )
else( OPENCV_FOUND )
    message( FATAL_ERROR "Could not find OpenCV package!" )
endif( OPENCV_FOUND )

MESSAGE( STATUS "OPENCV_LIBRARY = ${OPENCV_LIBRARIES}" )
MESSAGE( STATUS "OPENCV_INCLUDE = ${OPENCV_INCLUDE_DIR}" )

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/src)

#IF(BUILD_OSSIM_TESTS)
#   add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test ${CMAKE_CURRENT_BINARY_DIR}/test)
#ENDIF()


