# ossim-plugins
Contains plugin libraries for extending OSSIM core functionality.

## Description
Plugins extend OSSIM's core functionality with additional readers, writers, sensor models, high-level "tool" utilities, filters, and codecs. The OSSIM applications (ossim-cli, ossim-info, ossim-chipper, etc.) read a configuration file that contains a list of plugin libraries to load at run-time.

The preferences configuration file name is typically set via the environment variable OSSIM_PREFS_FILE. Alternatively, it can be specified with the `-P <filename>` option on most command lines. 
  
The preferences for the plugin library specifications in the configuration file follow the format:
```
plugin.file1: $(OSSIM_INSTALL_PREFIX)/lib/libossim_png_plugin.so
plugin.file2: $(OSSIM_INSTALL_PREFIX)/lib/libossim_kml_plugin.so
...
```
Where OSSIM_INSTALL_PREFIX is the location of the OSSIM/OMAR installation on the host.

## Plugin Development

To add functionality via the plugin scheme, it is necessary to perform the following steps. In this example, we're creating a plugin called `Xxx` that will supply a new reader, namely, `ossimXxxReader`
1. Copy an existing plugin directory. The "png" plugin is a good example to use as a template as it has multiple classes being registered.
2. Depending on the type of functionality you'll be adding, rename and edit the corresponding class files. For example, for our new reader, `ossimXxxReader`, is necessarily derived from `ossimImageHandler`. Refer to `ossimPngReader.h/cpp` as a starting point.
3. Every functional class (reader, writer, tool, etc.) needs an accompanying factory that will be registered with the application's registry system. In our example, rename `ossimPngReaderFactory.h/cpp` to `ossimXxxReaderFactory.h/cpp` and edit to reflect your reader class.
3. Rename `ossimPngPluginInit.cpp` to `ossimXxxPluginInit.cpp` and edit the file. This file contains the special symbols that will be referenced by the run-time to initialize the plugin and register the functionality with the application's object factory registries. Using `Png` as the starting point, this is just replacing "Png" with your plugin's name everywhere in the file. _It is important to keep the names that include "Png" unique, and to leave the other symbols alone._ The symbols that are expected by the run-time (exactly as is) are `ossimSharedLibraryInitialize` and `ossimSharedLibraryFinalize`.
4. Note the registration of the plugin-specific object factories with the run-time global registry in `ossimSharedLibraryInitialize()`. For example, the reader class is registered as
```     
ossimImageHandlerRegistry::instance()->registerFactory(ossimXxxReaderFactory::instance());
```
5. Though not necessary, it is useful to also register the class list (the list of all object classes being offered by the plugin) with the code adapted from `ossimPngPluginInit.cpp`:
```
ossimXxxReaderFactory::instance()->getTypeNameList(theXxxObjList);
```
This has the effect of making the plugin's object classes available to a master list. You can see this list by running via the command line:
```
ossim-info --plugins
```
6. Make sure to unregister all the factories in `ossimSharedLibraryFinalize()`.
7. Edit the `CmakeLists.txt` in the plugin's directory to reflect your files and dependencies.
8. Also edit the repo's top-level `ossim-plugins/CMakeLists.txt` to add your plugin. Note the use of cmake variables to easily enable/disable building of specific plugins from a script.

## Plug and Pray
The result of the build should be a library, in our example probably named `ossimXxx_plugin.so` (or dll), and located in the build directory's `lib` subdirectory. Edit you preferences/config file to mention your plugin and run `ossim-info --plugins` to verify it is being loaded properly. You should see your plugin listed along with the classes it represents.
