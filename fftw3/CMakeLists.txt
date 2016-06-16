MESSAGE( "************** LIBRARY SETUP FOR ossim_fftw3_plugin ******************")
SET(BUILD_OSSIM_FRAMEWORKS "OFF")
SET(OSSIM_PLUGIN_LINK_TYPE "SHARED")

SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++11" )

# Add our include and library...
find_package(ossim)
if(OSSIM_FOUND)
   include_directories( ${OSSIM_INCLUDE_DIR} )
   set( requiredLibs ${requiredLibs} ${OSSIM_LIBRARIES} )
else(OSSIM_FOUND)
   message(FATAL_ERROR "Could not find ossim")
endif(OSSIM_FOUND)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

find_package(FFTW3)
if(FFTW3_FOUND)
   include_directories( ${FFTW3_INCLUDE_DIR} )
   set(requiredLibs ${requiredLibs} ${FFTW3_LIBRARIES} )
else(FFTW3_FOUND)
   message(FATAL_ERROR "Could not find FFTW3")
endif(FFTW3_FOUND)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory( src )

