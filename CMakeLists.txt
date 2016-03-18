project(ossim-plugins)
MESSAGE("**********************Setting up ossimplugins********************")

if(NOT APPLE)
  cmake_minimum_required(VERSION 2.6)
else(NOT APPLE)
  cmake_minimum_required(VERSION 2.8)
endif(NOT APPLE)

include(OssimVersion)
include(OssimCommonVariables)
include(OssimUtilities)

############################### OVERRIDE THE DEFAULTS TO APPend THE PLUGIN INFORMATION###################


OPTION(OSSIM_INSTALL_PLUGINS_WITH_VERSION "Set to ON to build OSSIM's plugins to install with a version prefix like ossim-<version>/plugins.  Use OFF to not add a version and make it ossim/plugins." OFF)
OPTION(BUILD_CNES_PLUGIN "Set to ON to build OSSIM's CNES that depend only on ossim.  Use OFF to skip this plugin." ON)
OPTION(BUILD_GDAL_PLUGIN "Set to ON to build OSSIM's gdal plugin.  Use OFF to skip this plugin." OFF)
OPTION(BUILD_GEOPDF_PLUGIN "Set to ON to build OSSIM's geopdf plugin.  Use OFF to skip this plugin." OFF)
OPTION(BUILD_HDF5_PLUGIN "Set to ON to build OSSIM's hdf5 plugin.  Use OFF to skip this plugin." OFF)
OPTION(BUILD_KAKADU_PLUGIN "Set to ON to build OSSIM's kakadu plugin.  Use OFF to skip this plugin." OFF)
OPTION(BUILD_KML_PLUGIN "Set to ON to build OSSIM's kml super overvlay plugin.  Use OFF to skip this plugin." OFF)
OPTION(BUILD_MRSID_PLUGIN "Set to ON to build OSSIM's mrsid plugin.  Use OFF to skip this plugin." OFF)
OPTION(BUILD_OPENCV_PLUGIN "Set to ON to build OSSIM's opencv plugin.  Use OFF to skip this plugin." OFF)
OPTION(BUILD_OPENJPEG_PLUGIN "Set to ON to build OSSIM's openjpeg plugin.  Use OFF to skip this plugin." OFF)
OPTION(BUILD_PDAL_PLUGIN "Set to ON to build OSSIM's pdal plugin.  Use OFF to skip this plugin." OFF)
OPTION(BUILD_PNG_PLUGIN "Set to ON to build OSSIM's png plugin.  Use OFF to skip this plugin." OFF)
OPTION(BUILD_POTRACE_PLUGIN "Set to ON to build OSSIM's potrace plugin.  Use OFF to skip this plugin." OFF)
OPTION(BUILD_SQLITE_PLUGIN "Set to ON to build OSSIM's sqlite plugin.  Use OFF to skip this plugin." OFF)
OPTION(BUILD_WEB_PLUGIN "Set to ON to build OSSIM's web plugin.  Use OFF to skip this plugin." OFF)

find_package(OpenThreads)
find_package(ossim)

if(OSSIM_INSTALL_PLUGINS_WITH_VERSION)
  SET(INSTALL_RUNTIME_DIR   "${INSTALL_LIBRARY_DIR}/ossim-${OSSIM_VERSION}/plugins")
else()
  SET(INSTALL_RUNTIME_DIR   "${INSTALL_LIBRARY_DIR}/ossim/plugins")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${OSSIM_INCLUDE_DIR})
include_directories(${OPENTHREADS_INCLUDE_DIR})

SET(BUILD_OSSIM_FRAMEWORKS "OFF")
SET(OSSIM_PLUGIN_LINK_TYPE "SHARED")

if(BUILD_CNES_PLUGIN)
   add_subdirectory(cnes)
endif(BUILD_CNES_PLUGIN)

if(BUILD_CSM_PLUGIN)
   add_subdirectory(csm)
endiF(BUILD_CSM_PLUGIN)

if(BUILD_GDAL_PLUGIN)
   add_subdirectory(gdal)
endif(BUILD_GDAL_PLUGIN)

if(BUILD_GEOPDF_PLUGIN)
   add_subdirectory(geopdf)
endif(BUILD_GEOPDF_PLUGIN)

if(BUILD_HDF5_PLUGIN)
   add_subdirectory(hdf5)
endif(BUILD_HDF5_PLUGIN)

if(BUILD_KAKADU_PLUGIN)
   add_subdirectory(kakadu)
endif(BUILD_KAKADU_PLUGIN)

if(BUILD_KML_PLUGIN)
   add_subdirectory(kml)
endif(BUILD_KML_PLUGIN)

if(BUILD_MRSID_PLUGIN)
   add_subdirectory(mrsid)  
endif(BUILD_MRSID_PLUGIN)

if(BUILD_NDF_PLUGIN)
   add_subdirectory(ndf) 
endif(BUILD_NDF_PLUGIN)

if(BUILD_OPENCV_PLUGIN)
   add_subdirectory(opencv)
endif(BUILD_OPENCV_PLUGIN)

if(BUILD_OPENJPEG_PLUGIN)
   add_subdirectory(openjpeg) 
endif(BUILD_OPENJPEG_PLUGIN)

if(BUILD_PDAL_PLUGIN)
   add_subdirectory(pdal)
endif(BUILD_PDAL_PLUGIN)

if(BUILD_PNG_PLUGIN)
   add_subdirectory(png)
endif(BUILD_PNG_PLUGIN)

if(BUILD_POTRACE_PLUGIN)
   add_subdirectory(potrace)
endif(BUILD_POTRACE_PLUGIN)

if(BUILD_SQLITE_PLUGIN)
   add_subdirectory(sqlite)
endif(BUILD_SQLITE_PLUGIN)

if(BUILD_WEB_PLUGIN)
   add_subdirectory(web)
endif(BUILD_WEB_PLUGIN)

if(OSSIM_BUILD_ADDITIONAL_PLUGIN_DIRECTORIES)
    FOREACH(it IN LISTS OSSIM_BUILD_ADDITIONAL_PLUGIN_DIRECTORIES)
        add_subdirectory(${it})
    endFOREACH(it)
else(OSSIM_BUILD_ADDITIONAL_PLUGIN_DIRECTORIES)
    SET(OSSIM_BUILD_ADDITIONAL_PLUGIN_DIRECTORIES "" CACHE PATH "Specify full paths separated by ; to additional plugins you wish to include into the ossim build framework" FORCE)
endif()

MESSAGE( STATUS "BUILD_CNES_PLUGIN             = ${BUILD_CNES_PLUGIN}" )
MESSAGE( STATUS "BUILD_GDAL_PLUGIN             = ${BUILD_GDAL_PLUGIN}" )
MESSAGE( STATUS "BUILD_GEOPDF_PLUGIN           = ${BUILD_GEOPDF_PLUGIN}" )
MESSAGE( STATUS "BUILD_HDF5_PLUGIN             = ${BUILD_HDF5_PLUGIN}" )
MESSAGE( STATUS "BUILD_KAKADU_PLUGIN           = ${BUILD_KAKADU_PLUGIN}" )
MESSAGE( STATUS "BUILD_KML_PLUGIN              = ${BUILD_KML_PLUGIN}" )
MESSAGE( STATUS "BUILD_MRSID_PLUGIN            = ${BUILD_MRSID_PLUGIN}" )
MESSAGE( STATUS "BUILD_OPENCV_PLUGIN           = ${BUILD_OPENCV_PLUGIN}" )
MESSAGE( STATUS "BUILD_OPENJPEG_PLUGIN         = ${BUILD_OPENJPEG_PLUGIN}" )
MESSAGE( STATUS "BUILD_PDAL_PLUGIN             = ${BUILD_PDAL_PLUGIN}" )
MESSAGE( STATUS "BUILD_PNG_PLUGIN              = ${BUILD_PNG_PLUGIN}" )
MESSAGE( STATUS "BUILD_POTRACE_PLUGIN          = ${BUILD_POTRACE_PLUGIN}" )
MESSAGE( STATUS "BUILD_SQLITE_PLUGIN           = ${BUILD_SQLITE_PLUGIN}" )
MESSAGE( STATUS "BUILD_WEB_PLUGIN              = ${BUILD_WEB_PLUGIN}" )
MESSAGE( STATUS "OPENTHREADS_LIBRARY           = ${OPENTHREADS_LIBRARY}" )
MESSAGE( STATUS "OPENTHREADS_INCLUDE           = ${OPENTHREADS_INCLUDE_DIR}" )
MESSAGE( STATUS "OSSIM_LIBRARY                 = ${OSSIM_LIBRARY}" )
MESSAGE( STATUS "OSSIM_INCLUDE                 = ${OSSIM_INCLUDE_DIR}" )
MESSAGE( STATUS "OSSIM_INSTALL_PLUGINS_WITH_VERSION = ${OSSIM_INSTALL_PLUGINS_WITH_VERSION}" )

OSSIM_ADD_COMMON_MAKE_UNINSTALL()
