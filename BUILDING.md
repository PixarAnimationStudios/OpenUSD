Advanced Build Configuration
============================

## Table of Contents
- [Building With Build Script](#building-with-build-script)
- [Building With CMake](#building-with-cmake)
- [Optional Components](#optional-components)
- [Imaging Plugins](#imaging-plugins)
- [Third Party Plugins](#third-party-plugins)
- [Tests](#tests)
- [Other Build Options](#other-build-options)
- [USD Developer Options](#usd-developer-options)
- [Optimization Options](#optimization-options)
- [Linker Options](#linker-options)
- [Build Issues FAQ](#build-issues-faq)

## Building With Build Script

The simplest way to build USD is to run the supplied `build_usd.py`
script. This script will download required dependencies and build 
and install them along with USD in a given directory. 

See instructions and examples in [README.md](README.md#getting-and-building-the-code).

## Building With CMake

Users may specify libraries to build USD against and other build options by
passing arguments when running cmake. Documentation for these arguments
are below.

Some examples:

#### On Linux

```bash
cmake                                       \
-DTBB_ROOT_DIR=/path/to/tbb                 \
-DOPENSUBDIV_ROOT_DIR=/path/to/opensubdiv   \
-DBOOST_ROOT=/path/to/boost                 \
/path/to/USD/source

cmake --build . --target install -- -j <NUM_CORES>
```

#### On macOS

The following will generate an Xcode project that can be used to build USD.

```bash
cmake                                       \
-G "Xcode"                                  \
-DTBB_ROOT_DIR=/path/to/tbb                 \
-DOPENSUBDIV_ROOT_DIR=/path/to/opensubdiv   \
-DBOOST_ROOT=/path/to/boost                 \
/path/to/USD/source

cmake --build . --target install -- -j <NUM_CORES>
```

#### On Windows

The following will generate a Visual Studio 2017 solution that can be used to
build USD.

```cmd.exe
"C:\Program Files\CMake\bin\cmake.exe"      ^
-G "Visual Studio 15 2017 Win64"            ^
-DTBB_ROOT_DIR=C:\path\to\tbb               ^
-DOPENSUBDIV_ROOT_DIR=C:\path\to\opensubdiv ^
-DBOOST_ROOT=C:\path\to\boost               ^
\path\to\USD\source

cmake --build . --target install -- /m:%NUMBER_OF_PROCESSORS%
```

For other versions of Visual Studio, use the following cmake arguments:

- For VS2019: `-G "Visual Studio 16 2019" -A x64`
- For VS2022: `-G "Visual Studio 17 2022" -A x64`

For more information on Visual Studio generators for cmake, see 
[Visual Studio Generators](https://cmake.org/cmake/help/latest/manual/cmake-generators.7.html#visual-studio-generators).

## Optional Components

USD contains several optional components that are enabled by default
but may be disabled when invoking cmake. Disabling these components
removes the need for their dependencies when building USD.

##### Python

Some optional USD components use Python:
- [The USD Toolset](https://graphics.pixar.com/usd/docs/USD-Toolset.html)
- [Third Party Plugins](https://graphics.pixar.com/usd/docs/USD-3rd-Party-Plugins.html)
- Python language bindings for the USD C++ API
- Unit tests using Python

Please refer to [VERSIONS.md](VERSIONS.md) for supported Python versions.

Support for Python can optionally be disabled by specifying the cmake flag
`PXR_ENABLE_PYTHON_SUPPORT=FALSE`.

##### OpenGL

Support for OpenGL can optionally be disabled by specifying the cmake flag
`PXR_ENABLE_GL_SUPPORT=FALSE`.  This will skip components and libraries
that depend on GL, including:
- usdview
- Hydra GL imaging

##### Metal

Building USD with Metal enabled requires macOS Mojave (10.14) or newer.
Support for Metal can optionally be disabled by specifying the cmake flag
`PXR_ENABLE_METAL_SUPPORT=FALSE`.  This will skip components and libraries
that depend on Metal, including:
- Hydra imaging

##### Vulkan

Building USD with Vulkan enabled requires the Vulkan SDK and glslang to
be installed. The VULKAN_SDK environment variable must point to the
location of the SDK. The glslang compiler headers must be locatable during
the build process.

Support for Vulkan can optionally be enabled by specifying the cmake flag
`PXR_ENABLE_VULKAN_SUPPORT=TRUE`.

##### MaterialX

Enable [MaterialX](https://github.com/materialx/materialx) support in the 
build by specifying the cmake flag `PXR_ENABLE_MATERIALX_SUPPORT=TRUE` when
invoking cmake. Note that MaterialX with shared library support is required.

When building via build_usd.py, MaterialX support is enabled by default. The
default can be overriden using the --materialx and --no-materialx flags.

The additional dependencies that must be supplied when invoking cmake are:

| Dependency Name    | Description                                                 |
| ------------------ |-----------------------------------------------------------  |
| MaterialX_DIR      | Path to the CMake package config of a MaterialX SDK install.|

See [3rd Party Library and Application Versions](VERSIONS.md) for version information.

##### OSL (OpenShadingLanguage)

Support for OSL is disabled by default, and can optionally be enabled by
specifying the cmake flag `PXR_ENABLE_OSL_SUPPORT=TRUE`.  This will
enable components and libraries that depend on OSL.

Enabling OSL suport allows the Shader Definition Registry (sdr) to
parse metadata from OSL shaders.

##### Documentation

Doxygen documentation can optionally be generated by specifying the cmake flag
`PXR_BUILD_DOCUMENTATION=TRUE`.

The additional dependencies that must be supplied for enabling documentation 
generation are:

| Dependency Name    | Description                             |
| ------------------ |---------------------------------------  | 
| DOXYGEN_EXECUTABLE | The location of Doxygen                 |
| DOT_EXECUTABLE     | The location of Dot(from GraphViz).     |

See [3rd Party Library and Application Versions](VERSIONS.md) for version 
information, including supported Doxygen and GraphViz versions.

##### Python Documentation

Python docstrings for Python entities can be generated by specifying the cmake
flag `PXR_BUILD_PYTHON_DOCUMENTATION`. This process requires that Python support
(`PXR_ENABLE_PYTHON_SUPPORT`) and documentation (`PXR_BUILD_DOCUMENTATION`) are
enabled. 

This process uses the scripts in the docs/python subdirectory. Relevant
documentation from generated doxygen XML data is extracted and matched with
associated Python classes, functions, and properties in the built Python 
modules. A `__DOC.py` file is generated and installed in each of the directories 
of the installed Python modules. The `__DOC.py` file adds the docstrings to the 
Python entities when the module is loaded.

##### Imaging

This component contains Hydra, a high-performance graphics rendering engine.

Disable this component by specifying the cmake flag `PXR_BUILD_IMAGING=FALSE` when
invoking cmake. Disabling this component will also disable the [USD Imaging](#usd-imaging)
component and any [Imaging Plugins](#imaging-plugins).

##### USD Imaging

This component provides the USD imaging delegates for Hydra, as well as
usdview, a standalone native viewer for USD files.

Disable this component by specifying the cmake flag `PXR_BUILD_USD_IMAGING=FALSE` when
invoking cmake. usdview may also be disabled independently by specifying the cmake flag 
`PXR_BUILD_USDVIEW=FALSE`.

##### Command-line Tools

USD by default builds several helpful command-line tools for validating and 
manipulating USD files. For more information on the tools, see [USD Toolset](https://graphics.pixar.com/usd/release/toolset.html).

Disable building the command-line tools by specifying the cmake flag 
`PXR_BUILD_USD_TOOLS=FALSE` when invoking cmake. 

##### Examples

USD by default builds several example projects that demonstrate how to develop
various extensions and plugins.

Disable building the examples by specifying the cmake flag 
`PXR_BUILD_EXAMPLES=FALSE` when invoking cmake. 

##### Tutorials

USD by default builds USD and Python files used for the [USD Tutorials](https://graphics.pixar.com/usd/release/tut_usd_tutorials.html).

Disable building the tutorial support files by specifying the cmake flag 
`PXR_BUILD_TUTORIALS=FALSE` when invoking cmake. 

## Imaging Plugins

Hydra's rendering functionality can be extended with these optional plugins.

##### Ptex

Support for Ptex is disabled by default and can be enabled by specifying the 
cmake flag `PXR_ENABLE_PTEX_SUPPORT=TRUE`.

##### OpenImageIO 

This plugin can optionally be enabled by specifying the cmake flag
`PXR_BUILD_OPENIMAGEIO_PLUGIN=TRUE`. When enabled, OpenImageIO provides
broader support for reading and writing different image formats as textures. 
If OpenImageIO is disabled, imaging by default supports the image formats bmp, 
jpg, png, tga, and hdr. With OpenImageIO enabled, support extends to exr, tif, 
zfile, and tx file formats, which allows for the use of more advanced features
like subimages and mipmaps.

##### OpenColorIO 

This plugin can optionally be enabled by specifying the cmake flag
`PXR_BUILD_OPENCOLORIO_PLUGIN=TRUE`. When enabled, OpenColorIO provides
color management for Hydra viewports. 

##### Embree Rendering

This component contains an example rendering backend for Hydra and usdview, 
based on the embree raycasting library. Enable the plugin in the build by 
specifying the cmake flag `PXR_BUILD_EMBREE_PLUGIN=TRUE` when invoking
cmake.

The additional dependencies that must be supplied when invoking cmake are:

| Dependency Name       | Description                                 |
| --------------        | -----------------------------------         |
| EMBREE_LOCATION       | The root path to an embree library install. |

See [3rd Party Library and Application Versions](VERSIONS.md) for version information.

##### RenderMan Rendering

This plugin uses Pixar's RenderMan as a rendering backend for Hydra and 
usdview. Enable the plugin in the build by specifying the cmake flag 
`PXR_BUILD_PRMAN_PLUGIN=TRUE` when invoking cmake.

The additional dependencies that must be supplied when invoking cmake are:

| Dependency Name       | Description                                 |
| --------------        | -----------------------------------         |
| RENDERMAN_LOCATION    | The root path to an RenderMan install.      |

See [3rd Party Library and Application Versions](VERSIONS.md) for version information.

More documentation is available [here](http://openusd.org/docs/RenderMan-USD-Imaging-Plugin.html).

## Third Party Plugins

USD provides several plugins for integration with third-party software packages. 
There is additional documentation on each plugin
[here](http://openusd.org/docs/USD-3rd-Party-Plugins.html).
These plugins are not built by default and must be enabled via the instructions below.

The USD Maya plugins can be found in the Autodesk-supported repo available
[here](https://github.com/Autodesk/maya-usd).

The USD Katana plugins can be found in the Foundry-supported repo available
[here](https://github.com/TheFoundryVisionmongers/KatanaUsdPlugins).

##### Alembic Plugin

Enable the [Alembic](https://github.com/alembic/alembic) plugin in the build
by specifying the cmake flag `PXR_BUILD_ALEMBIC_PLUGIN=TRUE` when invoking cmake.

The additional dependencies that must be supplied when invoking cmake are:

| Dependency Name                   | Description                                       |
| ----------------------------------|-------------------------------------------------- |
| ALEMBIC_DIR                       | The location of [Alembic](https://https://github.com/alembic/alembic)   | 
| OPENEXR_LOCATION                  | The location of [OpenEXR](http://www.openexr.com) |
| Imath_DIR (If not using OpenEXR)  | Path to the CMake package config of a Imath SDK install. (With OpenEXR 3+, Imath can be used explicitly instead of OpenEXR.)|

Either OpenEXR or Imath is required depending on which library is used by the
Alembic library specified in ALEMBIC_DIR.

See [3rd Party Library and Application Versions](VERSIONS.md) for version information.

Support for Alembic files using the HDF5 backend is enabled by default but can be
disabled by specifying the cmake flag `PXR_ENABLE_HDF5_SUPPORT=FALSE`. HDF5
support requires the following dependencies:

| Dependency Name    | Description     |
| ------------------ |---------------- |
| HDF5_LOCATION      | The location of [HDF5](https://www.hdfgroup.org/HDF5/) |

For further information see the documentation on the Alembic plugin [here](http://openusd.org/docs/Alembic-USD-Plugin.html).

##### Draco Plugin

Enable the [Draco](https://github.com/google/draco) plugin in the build by specifying the cmake flag `PXR_BUILD_DRACO_PLUGIN=TRUE`
when invoking cmake. This plugin is compatible with Draco 1.3.4. The additional dependencies that must be supplied when invoking cmake are:

| Dependency Name    | Description                              | Version |
| ------------------ |----------------------------------------  | ------- |
| DRACO_ROOT         | The root path to a Draco SDK install.    | 1.3.4   |

## Tests

Tests are built by default but can be disabled by specifying the cmake flag 
`PXR_BUILD_TESTS=FALSE` when invoking cmake.

##### Running Tests
Run tests by invoking ctest from the build directory, which is typically the 
directory in which cmake was originally invoked. For example, to run all tests 
in a release build with verbose output:

```bash
ctest -C Release -V
```

The "-R" argument may be used to specify a regular expression matching the names 
of tests to be run. For example, to run all tests in a release build matching 
"testUsdShade" with verbose output:

```bash
ctest -C Release -R testUsdShade -V
```

See the [ctest documentation](https://cmake.org/cmake/help/latest/manual/ctest.1.html) for more options.

##### Diagnosing Failed Tests

In order to aid with diagnosing of failing tests, test generated files for failing test are explicitly put in the following directories, where
<ctest_run_timestamp> (formatted as "%Y-%m-%dT%H.%M.%S") represents the timestamp when ctest was run for the failing test.
```
${CMAKE_BINARY_DIR}/Testing/Failed-Diffs/<ctest_run_timestamp>/${TEST_NAME}/${filename}.result.${ext}
${CMAKE_BINARY_DIR}/Testing/Failed-Diffs/<ctest_run_timestamp>/${TEST_NAME}/${filename}.baseline.${ext}
```

## Other Build Options

##### Plugin Metadata Location

Each library in the USD core generally has an associated file named 'plugInfo.json' that contains metadata about that library,
such as the schema types provided by that library. These files are consumed by USD's internal plugin system to lazily load
libraries when needed.

The plugin system requires knowledge of where these metadata files are located. The cmake build will ensure this is set up
properly based on the install location of the build. However, if you plan to relocate these files to a new location after
the build, you must inform the build by setting the cmake variable `PXR_INSTALL_LOCATION` to the intended final
directory where these files will be located. This variable may be a ':'-delimited list of paths.

Another way USD is locating plugins is the `PXR_PLUGINPATH_NAME` environment variable. This variable
may be a list of paths. If you do not want your USD build to use this default variable name, you can override the name
of the environment variable using the following CMake option:

```
-DPXR_OVERRIDE_PLUGINPATH_NAME=CUSTOM_USD_PLUGINPATHS
```

By doing this, USD will check the `CUSTOM_USD_PLUGINPATHS` environment variable for paths, instead of the default
`PXR_PLUGINPATH_NAME` one.

The values specified in `PXR_PLUGINPATH_NAME` or `PXR_INSTALL_LOCATION`
have the following characteristics:

- Values may contain any number of paths.

- Paths ending with slash ('/') have 'plugInfo.json' appended automatically.

- '*' may be used anywhere to match any character except slash.

- '**' may be used anywhere to match any character including slash.

- Paths follow Unix '$PATH'-like conventions; when duplicate definitions exist
  in the path, the first one found is used.

##### Shared library prefix

By default shared libraries will have the prefix 'lib'. This means, for a given
component such as [usdGeom](pxr/usd/lib/usdGeom), the build will generate a corresponding
libusdGeom object (libusdGeom.so on Linux, libusdGeom.dll on Windows
and libusdGeom.dylib on Mac). You can change the prefix (or remove it) through
`PXR_LIB_PREFIX`. For example,

```
-DPXR_LIB_PREFIX=pxr
```

Will generate pxrusdGeom.so on Linux, pxrusdGeom.dll on Windows and
pxrusdGeom.dylib on Mac for the usdGeom component.

> Note: This prefix does not apply to shared objects used for Python bindings.

## USD Developer Options

##### C++ Namespace Configuration

USD comes with options to enable and customize C++ namespaces via the following
flags:

| Option Name                    | Description                             | Default |
| ------------------------------ |-----------------------------------------| ------- |
| PXR_SET_EXTERNAL_NAMESPACE     | The outer namespace identifier          | `pxr`     |
| PXR_SET_INTERNAL_NAMESPACE     | The internal namespace identifier       | `pxrInternal_v_x_y` (for version x.y.z) |
| PXR_ENABLE_NAMESPACES          | Enable namespaces                       | `ON`    |

When enabled, there are a set of macros provided in a generated header,
pxr/pxr.h, which facilitates using namespaces:

| Macro Name                     | Description                             |
| ------------------------------ |-----------------------------------------|
| PXR_NAMESPACE_OPEN_SCOPE       | Opens the namespace scope.                                           |
| PXR_NAMESPACE_CLOSE_SCOPE      | Closes the namespace.                                                |
| PXR_NS                         | Explicit qualification on items, e.g. `PXR_NS::TfToken foo = ...`|
| PXR_NAMESPACE_USING_DIRECTIVE  | Enacts a using-directive, e.g. `using namespace PXR_NS;`         |

##### ASCII Parser Editing/Validation

There is an ASCII parser for the USD file format, which can be found in
[sdf](pxr/usd/sdf). Most users will not have a need to edit the parser, but
for the adventurous ones, there are a couple additional requirements.

If you choose to edit the ASCII parsers, make sure
`PXR_VALIDATE_GENERATED_CODE` is set to `TRUE`.  This flag enables tests
that check the generated code in [sdf](pxr/usd/lib/sdf) and
[gf](pxr/base/lib/gf).

| Dependency Name    | Description                                             |
| ------------------ | ------------------------------------------------------- |
| FLEX_EXECUTABLE    | Path to [flex](http://flex.sourceforge.net/) executable |
| BISON_EXECUTABLE   | Path to [bison](https://www.gnu.org/software/bison/) executable  |

See [3rd Party Library and Application Versions](VERSIONS.md) for version information.


##### USD Schema Generation

USD generates some code through a process called [schema
generation]. This process requires the following python modules be installed
and available on the syspath. You can learn more about Schemas and why you
might want to generate them
[here](http://openusd.org//docs/Generating-New-Schema-Classes.html).

| Python Module Name                         | Description                    |
| ------------------------------------------ |------------------------------- |
| [Jinja2](http://jinja.pocoo.org/docs/dev/) | Jinja is the core code generator of usdGenSchema                     |
| [Argparse](https://docs.python.org/3/library/argparse.html) | Argparse is used for basic command line arguments   |

See [3rd Party Library and Application Versions](VERSIONS.md) for version information.


## Optimization Options

There are certain optimizations that can be enabled in the build.

##### Malloc Library

We've found that USD performs best with allocators such as [Jemalloc](https://github.com/jemalloc/jemalloc).
In support of this, you can specify your own allocator through `PXR_MALLOC_LIBRARY`.
This variable should be set to a path to a shared object for the allocator. For example,

```bash
-DPXR_MALLOC_LIBRARY:path=/usr/local/lib/libjemalloc.so
```

If none are specified, the default allocator will be used. More information on getting the most out of
USD can be found [Getting the Best Performance with USD](http://openusd.org/docs/Maximizing-USD-Performance.html).

## Linker Options

There are four ways to link USD controlled by the following options:

| Option Name            | Default   | Description                               |
| ---------------------- | --------- | ----------------------------------------- |
| BUILD_SHARED_LIBS      | `ON`      | Build shared or static libraries          |
| PXR_BUILD_MONOLITHIC   | `OFF`     | Build single or several libraries         |
| PXR_MONOLITHIC_IMPORT  |           | CMake file defining usd_ms import library |

##### Shared Libraries

The default creates several shared libraries.  This option allows loading
just the libraries necessary for a given task.

| Option Name            | Value     |
| ---------------------- | --------- |
| BUILD_SHARED_LIBS      | `ON`      |
| PXR_BUILD_MONOLITHIC   | `OFF`     |
| PXR_MONOLITHIC_IMPORT  |           |

```bash
cmake -DBUILD_SHARED_LIBS=ON ...
```

##### Static Libraries

This mode builds several static libraries.  This option allows embedding
just the libraries necessary for a given task.  However, it does not allow
USD plugins or Python modules since that would necessarily cause multiple
symbol definitions;  for any given symbol we'd have an instance in the main
application and another in each plugin/module.

| Option Name            | Value     |
| ---------------------- | --------- |
| BUILD_SHARED_LIBS      | `OFF`     |
| PXR_BUILD_MONOLITHIC   | `OFF`     |
| PXR_MONOLITHIC_IMPORT  |           |

```bash
cmake -DBUILD_SHARED_LIBS=OFF ...
```

##### Internal Monolithic Library

This mode builds the core libraries (i.e. everything under `pxr/`) into a
single archive library, 'usd_m', and from that it builds a single shared
library, 'usd_ms'.  It builds plugins outside of `pxr/` and Python modules
as usual except they link against 'usd_ms' instead of the individual
libraries of the default mode.  Plugins inside of `pxr/` are compiled into
'usd_m' and 'usd_ms'.  plugInfo.json files under `pxr/` refer to 'usd_ms'.

This mode is useful to reduce the number of installed files and simplify
linking against USD.

| Option Name            | Value        |
| ---------------------- | ----------   |
| BUILD_SHARED_LIBS      | _Don't care_ |
| PXR_BUILD_MONOLITHIC   | `ON`         |
| PXR_MONOLITHIC_IMPORT  |              |

```bash
cmake -DPXR_BUILD_MONOLITHIC=ON ...
```

##### External Monolithic Library

This mode is similar to the
[Internal Monolithic Library](#internal-monolithic-library) except the
client has control of building the monolithic shared library.  This mode
is useful to embed USD into another shared library.  The build steps are
significantly more complicated and are described below.

| Option Name            | Value                 |
| ---------------------- | ----------            |
| BUILD_SHARED_LIBS      | _Don't care_          |
| PXR_BUILD_MONOLITHIC   | `ON`                  |
| PXR_MONOLITHIC_IMPORT  | _Path-to-import-file_ |

To build in this mode:

1. Choose a path where the import file will be.  You'll be creating a cmake
file with `add_library(usd_ms SHARED IMPORTED)` and one or more `set_property`
calls.  The file doesn't need to exist.  If it does exist it should be empty
or valid cmake code.
1. Configure the build in the usual way but with `PXR_BUILD_MONOLITHIC=ON`
and `PXR_MONOLITHIC_IMPORT` set to the path in step 1.
1. Build the usual way except the target is `monolithic`.
1. Create your shared library. If using cmake you can include the file
`pxr/usd-targets-<CONFIG>` under the USD binary (build) directory, where
`<CONFIG>` is the configuration you built in step 3. Then you can link your
library against 'usd_m'.  However, this isn't as simple as
`target_link_libraries(mylib PUBLIC usd_m)` because you must get
**everything** from 'usd_m'.  See [Linking Whole Archives](#linking-whole-archives)
for more details.
1. Edit the import file to describe your library.  Your cmake build may
be able to generate the file directly via `export()`.  The USD build
will include this file and having done so must be able to link against
your library by adding 'usd_ms' as a target link library.  The file
should look something like this:
    ```cmake
    add_library(usd_ms SHARED IMPORTED)
    set_property(TARGET usd_ms PROPERTY IMPORTED_LOCATION ...)
    # The following is necessary on Windows.
    #set_property(TARGET usd_ms PROPERTY IMPORTED_IMPLIB ...)
    set_property(TARGET usd_ms PROPERTY INTERFACE_COMPILE_DEFINITIONS ...)
    set_property(TARGET usd_ms PROPERTY INTERFACE_INCLUDE_DIRECTORIES ...)
    set_property(TARGET usd_ms PROPERTY INTERFACE_LINK_LIBRARIES ...)
    ```
1. Complete the USD build by building the usual way, either with the
default target or the 'install' target.

Two notes:
1. Your library does **not** need to be named usd_ms. That's simply the
name given to it by the import file. The IMPORTED_LOCATION  has the real
name and path to your library.
1. USD currently only supports installations where your library is in
the same directory the USD library/libraries would have been relative
to the other installed USD files.  Specifically, the location of your
library will be used to find plugInfo.json files using the relative
paths `../share/usd/plugins` and `../plugin/usd`.

###### Linking Whole Archives

Normally when linking against a static library the linker will only pull
in object files that provide a needed symbol. USD has many files that
have static global objects with constructors with side effects.  If
nothing uses any visible symbol from those object files then a normal
link would not include them. The side effects will not occur and USD
will not work.

To include everything you need to tell the linker to include the whole
archive.  That's platform dependent and you'll want code something like
this:

```cmake
if(MSVC)
    target_link_libraries(mylib -WHOLEARCHIVE:$<TARGET_FILE:usd_m> usd_m)
elseif(CMAKE_COMPILER_IS_GNUCXX)
    target_link_libraries(mylib -Wl,--whole-archive usd_m -Wl,--no-whole-archive)
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    target_link_libraries(mylib -Wl,-force_load usd_m)
endif()
```

On Windows cmake cannot recognize 'usd_m' as a library when appended to
 -WHOLEARCHIVE: because it's not a word to itself so we use TARGET_FILE
to get the path to the library. We also link 'usd_m' separately so cmake
will add usd_m's interface link libraries, etc. This second instance
doesn't increase the resulting file size because all symbols will be
found in the first (-WHOLEARCHIVE) instance.

###### Avoiding linking statically to Python

The default build with python support will link to the python static lib for
your interpreter. This is to support running python code from C++. If that is
not desirable, python static linking can be disabled using the flag

```
-DPXR_PY_UNDEFINED_DYNAMIC_LOOKUP=ON
```

The primary motivating case for this is generating wheel packages for PyPI, but
the parameter was made more generic in case it has other uses in the future. It
is useful when we want to take advantage of python's approach to ABI
compatibility.

Note that this flag has no effect on Windows, see 
[here for more info](https://docs.python.org/3/extending/windows.html)
    

## Build Issues FAQ

1. Boost_NO_BOOST_CMAKE: 
We currently set Boost_NO_BOOST_CMAKE=ON explicitly in USD builds for all 
platforms to avoid issues with Boost config files (introduced in Boost version 
1.70) and python, program options component requirements. If the user wants 
to use Boost specified config files for their USD build, specify 
-DBoost_NO_BOOST_CMAKE=OFF when running cmake.

2. Windows and Python 3.8+ (non-Anaconda)
Python 3.8 and later on Windows will no longer search PATH for DLL dependencies.
Instead, clients can call `os.add_dll_directory(p)` to set paths to search.
By default on that platform USD will iterate over PATH and add all paths using
`os.add_dll_directory()` when importing Python modules. Users may override
this by setting the environment variable `PXR_USD_WINDOWS_DLL_PATH` to a PATH-like
string. If this is set, USD will use these paths instead.

Note that the above does not apply to Anaconda python 3.8+ interpreters, as they
are modified to behave like pre-3.8 python interpreters, and so continue to use
the PATH for DLL dependencies.  When running under Anaconda users should
configure their system the same way they did for pre-python 3.8.
