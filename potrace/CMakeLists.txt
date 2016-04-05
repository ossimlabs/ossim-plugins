MESSAGE( "************** CMAKE SETUP FOR ossim_potrace_plugin ******************")

SET(BUILD_OSSIM_FRAMEWORKS "OFF")
SET(OSSIM_PLUGIN_LINK_TYPE "SHARED")

# Add our include and library...
find_package(ossim)
if(OSSIM_FOUND)
   include_directories( ${OSSIM_INCLUDE_DIR} )
   set( requiredLibs ${requiredLibs} ${OSSIM_LIBRARIES} )
else(OSSIM_FOUND)
   message(FATAL_ERROR "Could not find ossim")
endif(OSSIM_FOUND)

add_subdirectory( src )

message( "************** END SETUP FOR ossim_potrace_plugin ******************" )
