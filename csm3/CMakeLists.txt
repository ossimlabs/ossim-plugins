set(LIB_NAME ossim_csm3_plugin)
MESSAGE( "************** LIBRARY SETUP FOR ossim_csm3_plugin******************")

# Include ossim cmake stuff:
include(OssimVersion)
include(OssimCommonVariables)
include(OssimUtilities)

set(requiredLibs)

# csmApi - Required:
find_package(CsmApi)
if (CSMAPI_FOUND)
   include_directories(${CSMAPI_INCLUDE_DIRS})
   set( requiredLibs ${requiredLibs} ${CSMAPI_LIBRARIES} )
else(CSMAPI_FOUND)
   message(FATAL_ERROR "Could not find csmApi")
endif(CSMAPI_FOUND)

add_subdirectory( src )

IF(BUILD_OSSIM_TESTS)
   add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test ${CMAKE_CURRENT_BINARY_DIR}/test)
ENDIF()


