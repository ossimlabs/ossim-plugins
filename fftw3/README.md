# ossim-fftw-plugin
Contains C++ library code for accessing FFTW3 library as an ossimImageSourceFilter class.

Requires the third-party open source library [FFTW3](https://www.fftw.org "FFTW3's Homepage").

Note: It may be necessary to build the FFTW3 library with the compile option "-fPIC" to avoid link-time errors while building this plugin. The easiest solution is to:

1. checkout the [FFTW3 git repository](https://github.com/FFTW/fftw3 "FFTW3 on GitHub"), 
2. in the shell you'll be building the library, do `export CPPFLAGS="-fPIC"`
3. then follow the directions for building on your system. On linux, run ./configure; make; make install.

The make install should place the FFTW3 library where OSSIM's cmake will find it. 

It is important that you make sure you do the following -- this applies to all plugins:

1. Enable the plugin build by either exporting an env var `BUILD_FFTW3_PLUGIN="ON"`, or editing ossim/cmake/scripts/ossim-cmake-config.sh and default that variable to "ON".
2. Verify that libossim-fftw3-plugin.so (or equivalent) is in your build/lib folder.
3. Add the plugin library to your ossim preferences file _before_ the GDAL plugin (the latter must always be last).
