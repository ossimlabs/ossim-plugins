# ossim-potrace-plugin
Contains C++ library code for vectorizing bitmap data using the POTRACE library. The plugin provides an ossimUtility-derived class for performing the raster-to-vector conversion. Currently only GeoJSON format is supported for the vector output.

# How to Build
1. Make sure the environment is set up as described in the OSSIM repo [README](https://github.com/ossimlabs/ossim/blob/master/README.md). 
2. You will need to either define an environment variable `BUILD_POTRACE_PLUGIN=ON` or edit the [cmake script](https://github.com/ossimlabs/ossim/blob/master/cmake/scripts/) to default that variable to on.
3. Build OSSIM as you normally do. The plugin library should be built and located in the build/lib directory as ossim_potrace_plugin.so (for linux).
 
# How to Use
There is a dedicated application, `ossim-potrace`, that is built and installed when the plugin is generated. It requires the plugin library at runtime only. The command line is simply:

`ossim-potrace [options] <input-raster-file> [<output-vector-file>]`

Or, alternatively using the ossim command-line interface:

`ossim-cli potrace [options] <input-raster-file> [<output-vector-file>]`

The program treats all non-null pixels as foreground, and null pixels as background. Polygons are generated that encircle sets of contiguous foreground pixels.

The only option presently supported is `--mode polygon|linestring` that specifies whether to represent foreground-background boundary as polygons or line-strings. Polygons are closed regions surrounding either null or non-null pixels. Most viewers will represent polygons as solid blobs. Line-strings only outline the boundary but do not maintain sense of "insideness".

Presently only GeoJSON format is supported for the output vector file. Other formats shall be added as needed.

Special acknowledgement is given to Peter Selinger for making Potrace available to the open source community. Much of the code in this plugin was shamelessly lifted from his [source code repository](http://potrace.sourceforge.net) and modified to work in the OSSIM environment. 

