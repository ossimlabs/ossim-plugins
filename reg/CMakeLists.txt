MESSAGE( "************** LIBRARY SETUP FOR ossim_reg_plugin******************")

# Include ossim cmake stuff:
include(OssimVersion)
include(OssimCommonVariables)
include(OssimUtilities)

set(requiredLibs)

find_package( OpenCV )
if( OPENCV_FOUND )
    include_directories( ${OPENCV_INCLUDE_DIR} )
    set( requiredLibs ${requiredLib} ${OPENCV_LIBRARIES} )
else( OPENCV_FOUND )
    message( FATAL_ERROR "Could not find OpenCV package!" )
endif( OPENCV_FOUND )

# Check for optional OpenCV libraries:
if( OPENCV_GPU_FOUND )
    set( requiredLib ${requiredLib} ${OPENCV_GPU_LIBRARY} )
endif( OPENCV_GPU_FOUND )

if( OPENCV_IMGCODECS_FOUND )
    set( requiredLib ${requiredLib} ${OPENCV_IMGCODECS_LIBRARY} )
endif( OPENCV_IMGCODECS_FOUND )

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_BINARY_DIR}/src)

IF(BUILD_OSSIM_TESTS)
   add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test ${CMAKE_CURRENT_BINARY_DIR}/test)
ENDIF()


