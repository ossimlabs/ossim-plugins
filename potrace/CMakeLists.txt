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

find_package( JsonCpp )
if( JSONCPP_FOUND )
   include_directories( ${JSONCPP_INCLUDE_DIR} )
   set( requiredLibs ${requiredLibs} ${JSONCPP_LIBRARY} )
   message("Found JsonCpp" )
else( JSONCPP_FOUND )
   message( FATAL_ERROR "Could not find required JsonCpp package!" )
endif( JSONCPP_FOUND )

add_subdirectory( src )

message( "************** END SETUP FOR ossim_potrace_plugin ******************" )
