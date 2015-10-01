PROJECT(ossim-kakadu-plugin)

# Specify location of Kakadu SDK. 
# It is assumed here that a symlink exists called KAKADU_SDK_DIR in 
# this directory and points to the SDK directory. You can alternatively 
# hardcode the path here. See ./README.md file. 
IF(NOT DEFINED KAKADU_ROOT_SRC)
  SET(KAKADU_ROOT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/KAKADU_ROOT_SRC")
ENDIF()
IF(NOT DEFINED KAKADU_AUX_LIBRARY)
  SET(KAKADU_AUX_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/KAKADU_AUX_LIB")
ENDIF()
IF(NOT DEFINED KAKADU_LIBRARY)
  SET(KAKADU_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/KAKADU_LIB")
ENDIF()

SET(BUILD_OSSIM_FRAMEWORKS "OFF")
SET(OSSIM_PLUGIN_LINK_TYPE "SHARED")

subdirs(src)
subdirs(test)

