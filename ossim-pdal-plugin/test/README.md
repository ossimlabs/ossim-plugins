# PDAL Plugin Tests

A single executable provides testing for both the ossimPdalFileReader and ossimRialtoReader classes. After building, the executable is available in $OSSIM_BUILD_DIR/bin/ossim-pdal-plugin-test. The executable should be run out of this test directory if the default autzen data is to be used.

The default test data is autzen.las, a small point cloud of ~10,000 points. Alternatively, a filename can be provided on the command line to use a different dataset.

## PDAL LAS File Generator

This is not really a test, as it does not make use of any OSSIM class of consequence. It is used to generate a sample ramp point cloud LAS file with 11 points. The output is the file "ramp.las" which can serve as input to the PDAL test below.


## PDAL File Reader Test

To run the ossimPdalFileReader test, do:

    ossim-pdal-plugin-test pdal [<alt_input.las>]

The output will be the raster file "pdal-OUTPUT.tif". This file can be compared against the expected result found in pdal-EXPECTED.ti. If the optional LAS file is specified, it is used in place of the default autzen.las.


## RIALTO_TRANSLATE and RIALTO_INFO

This is not a test, but a step that must be run before the rialto test can be executed. This utility generates a rialto geopackage file repesenting the input filename specified:

    rialto_translate -m 0 -v -i <myfile>.las -o <myfile>.gpkg

It will generate a geopackage by the name <myfile>.gpkg, which can be used as input to the rialto test. 


## Rialto Reader Test

To run the ossimRialtoReader test, do:

    ossim-pdal-plugin-test rialto [<alt_input.las>]

The output will be the raster file "rialto-OUTPUT.tif". Compare against rialto-EXPECTED.tif. An alternate gpkg file can be specified for generating corresponding TIF image.



