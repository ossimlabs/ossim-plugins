set(LIB_NAME ossim_csm3_plugin)
MESSAGE( "************** LIBRARY SETUP FOR ossim_csm3_plugin******************")

# Include ossim cmake stuff:
include(OssimVersion)
include(OssimCommonVariables)
include(OssimUtilities)

set(requiredLibs)

message(" MSP_HOME = <${MSP_HOME}>")
find_package(MSP)
if (MSP_FOUND)
   include_directories(${MSP_INCLUDE_DIRS})
   set( requiredLibs ${requiredLibs} ${MSP_LIBRARIES} )
   message( "Found MSP" )
else(MSP_FOUND)
   message(FATAL_ERROR, "Could not find MSP package. Cannot build CSM3 plugin.")
endif(MSP_FOUND)

add_subdirectory( src )

IF(BUILD_OSSIM_TESTS)
   add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test ${CMAKE_CURRENT_BINARY_DIR}/test)
ENDIF()


