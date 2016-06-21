# ossim-jpeg12-plugin

OSSIM Plugin for reading nitf files with 12 bit jpeg compressed blocks.

## Build Instructions for plugin:

This plugin is built as part of the OSSIM suite of repositories. See the build instructions in [ossim/README.md](http://github.com/ossimlabs/ossim/blob/master/README.md). As long as this repository is located in the same parent directory as [OSSIM](http://github.com/ossimlabs/ossim), CMake will detect it and create the Makefiles in `<build-dir>/<build-type>/ossim-jpeg12-plugin`. You can `cd` into that directory and type `make` if you only want to build the plugin.

## Runtime and OSSIM Preferences File
There is an ossim preferences file template/example in the repository at [`ossim/share/ossim/templates/ossim_preferences_template`](http://github.com/ossimlabs/ossim/blob/master/share/ossim/templates/ossim_preferences_template). Copy it to some known location. All OSSIM applications will look for this file to configure the runtime. Set the environment variable `OSSIM_PREFS_FILE` to point to your working prefs file prior to running any OSSIM application.

In your preferences file, add the following line in the section for plugins:
```
plugin.fileX: /path/to/your/ossim-jpeg12-plugin.so
```
The extension will of course depend on your OS. Replace X with a consecutive index (starting at 0 or 1). The plugins are loaded in the order of these indexes. The path should be your OSSIM install path, or you can use your output build's `lib` directory during development.
