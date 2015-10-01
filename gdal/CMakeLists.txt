PROJECT(ossim-gdal-plugin)

SET(BUILD_OSSIM_FRAMEWORKS "OFF")
SET(OSSIM_PLUGIN_LINK_TYPE "SHARED")

subdirs(src)
subdirs(test)

