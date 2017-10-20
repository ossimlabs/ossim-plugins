set(LIB_NAME ossim_csm3_plugin)
MESSAGE( "************** LIBRARY SETUP FOR ossim_csm3_plugin******************")

# Include ossim cmake stuff:
include(OssimVersion)
include(OssimCommonVariables)
include(OssimUtilities)

set(requiredLibs)

message("MSP_HOME = <${MSP_HOME}>")
find_package(MSP)
if (MSP_FOUND)
   include_directories(${MSP_INCLUDE_DIRS})
   set( requiredLibs ${requiredLibs} ${MSP_LIBRARIES} )
   set( OSSIM_HAS_MSP 1 )
   message( "CSM3 is using MSP" )
   
else(MSP_FOUND)
   message( "Could not find MSP package. Building CSM3 plugin without MSP support.")
   set( OSSIM_HAS_MSP 0 )
endif(MSP_FOUND)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/src/ossimCsm3Config.h.in
               ${CMAKE_CURRENT_SOURCE_DIR}/src/ossimCsm3Config.h @ONLY)

add_subdirectory( src )

#IF(BUILD_OSSIM_TESTS)
#   add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test ${CMAKE_CURRENT_BINARY_DIR}/test)
#ENDIF()


