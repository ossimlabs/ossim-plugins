set(LIB_NAME ossim_msp_plugin)
MESSAGE( "************** LIBRARY SETUP FOR ossim_msp_plugin******************")

# Include ossim cmake stuff:
include(OssimVersion)
include(OssimCommonVariables)
include(OssimUtilities)

set(requiredLibs ${dependentLibs})

find_package(MSP)
if (MSP_FOUND)
   include_directories(${MSP_INCLUDE_DIRS})
   set( requiredLibs ${requiredLibs} ${MSP_LIBRARIES} )
else(MSP_FOUND)
   message(FATAL_ERROR "Could not find MSP")
endif(MSP_FOUND)

add_subdirectory( src )

IF(BUILD_OSSIM_TESTS)
   add_subdirectory( test )
ENDIF()


