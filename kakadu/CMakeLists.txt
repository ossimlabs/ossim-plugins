# Top level ossim kakadu plugin CMakeLists.txt file.

add_subdirectory(src)

IF(BUILD_OSSIM_TESTS)
   add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test ${CMAKE_CURRENT_BINARY_DIR}/test)
ENDIF()
