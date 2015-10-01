PROJECT(ossim-mrsid-plugin)

# Specify location of MrSID SDK containing Raster and LIDAR subdirectories. 
# It is assumed here that a symlink exists called MRSID_SDK_DIR in 
# this directory and points to the SDK directory. You can alternatively 
# hardcode the path here. See ./README.md file. 
IF(NOT DEFINED MRSID_DIR)
  SET(MRSID_DIR "${CMAKE_CURRENT_SOURCE_DIR}/MRSID_SDK_DIR")
ENDIF()

SET(BUILD_OSSIM_FRAMEWORKS "OFF")
SET(OSSIM_PLUGIN_LINK_TYPE "SHARED")

subdirs(src)
subdirs(test)

