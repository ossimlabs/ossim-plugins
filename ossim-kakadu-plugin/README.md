# ossim-kakadu-plugin

OSSIM Plugin for reading and writing kakadu-formatted images. You must have a licensed kakadu SDK available.

## Build Instructions
This plugin is built as part of the OSSIM suite of repositories. See the build instructions in [ossim/README.md](http://github.com/ossimlabs/ossim/blob/master/README.md). As long as this repository is located in the same parent directory as [OSSIM](http://github.com/ossimlabs/ossim), CMake will detect it and create the Makefiles in `<build-dir>/<build-type>/ossim-kakadu-plugin`. You can `cd` into that directory and type `make` if you only want to build the plugin.

The kakadu libraries and SDK are required to build this plugin. In order for CMake to find the SDK, you need to do *one* of the following:

1. Create symbolic links: `KAKADU_ROOT_SRC`, `KAKADU_LIB`, and `KAKADU_AUX_LIB` in this directory (dummies are already in place) that point to the corresponding Kakadu directory and libraries, respectively. -OR-

2. Open the CMakeLists.txt in this folder and edit the first instructions, `set(KAKADU_ROOT_SRC...)` and so on, with the correct paths specified. You'll notice it currently uses the symbolic links.

It is possible to skip the building of this repo even if it is present in your local ossimlabs parent directory. Add the following to your cmake command line: `-DBUILD_KAKADU_PLUGIN=OFF`. You can also edit the build script in [ossim/cmake/scripts](http://github.com/ossimlabs/ossim/blob/master/cmake/scripts) and add the above `-D` parameter to the cmake command there.

## Runtime and OSSIM Preferences File
There is an ossim preferences file template/example in the repository at [`ossim/share/ossim/templates/ossim_preferences_template`](http://github.com/ossimlabs/ossim/blob/master/share/ossim/templates/ossim_preferences_template). Copy it to some known location. All OSSIM applications will look for this file to configure the runtime. Set the environment variable `OSSIM_PREFS_FILE` to point to your working prefs file prior to running any OSSIM application.

In your preferences file, add the following line in the section for plugins:
```
plugin.fileX: /path/to/your/ossim-kakadu-plugin.lib
```
The extension will of course depend on your OS. Replace X with a consecutive index (starting at 0 or 1). The plugins are loaded in the order of these indexes. The path should be your OSSIM install path, or you can use your output build's `lib` directory during development.



