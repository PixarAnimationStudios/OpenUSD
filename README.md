Universal Scene Description
===========================

Universal Scene Description (USD) is an efficient, scalable system for
authoring, reading, and streaming time-sampled scene description for
interchange between graphics applications.

For more details, please visit the web site [here](http://openusd.org).

Build Status
------------

|       | master | dev |
| ----- | ------ | --- |
| Linux | [![Build Status](https://travis-ci.org/PixarAnimationStudios/USD.svg?branch=master)](https://travis-ci.org/PixarAnimationStudios/USD) | [![Build Status](https://travis-ci.org/PixarAnimationStudios/USD.svg?branch=dev)](https://travis-ci.org/PixarAnimationStudios/USD) |

Additional Documentation
------------------------

* [User Documentation and Tutorials](http://openusd.org/docs/index.html)
* [API Documentation](http://openusd.org/docs/api/index.html)
* [Advanced Build Configuration](BUILDING.md)

Getting Help
------------

Need help understanding certain concepts in USD? See
[Getting Help with USD](http://openusd.org/docs/Getting-Help-with-USD.html) or
visit our [forum](https://groups.google.com/forum/#!forum/usd-interest).

If you are experiencing undocumented problems with the software, please 
[file a bug](https://github.com/PixarAnimationStudios/USD/issues/new).

Supported Platforms
-------------------

USD is currently supported on Linux platforms and has been built and tested
on CentOS 7 and RHEL 7.

We are actively working on porting USD to both Windows and Mac platforms. 
Support for both platforms should be considered experimental at this time.
Currently, the tree will build on Mac and Windows, but only limited testing
has been done on these platforms.

Dependencies
------------

The Core USD libraries (located in pxr/base and pxr/usd respectively) 
have the following dependencies.

| Name | Version |
| ---- | --------- |
| C++ compiler                                                      | GCC 4.8, Clang 3.5, MSVC 14.0(VS 2015) |
| C compiler                                                        | GCC 4.8, Clang 3.5, MSVC 14.0(VS 2015) |
| [CMake](https://cmake.org/documentation/)                         | 2.8.8 (Linux/OS X), 3.1.1 (Windows)    |
| [Python](https://python.org)                                      | 2.7.5                                  |
| [Boost](https://boost.org)                                        | 1.55 (Linux), 1.61.0 (OS X/Windows)    |
| [Intel TBB](https://www.threadingbuildingblocks.org/)             | 4.3.1                                  |

The Imaging and UsdImaging components (located in pxr/imaging and pxr/usdImaging
respectively) have the following additional dependencies. These components can
be disabled at build-time, for further details see [Advanced Build Configuration](BUILDING.md).

| Name | Version | Optional |
| ---- | --------- | ---------- |
| [OpenSubdiv](https://github.com/PixarAnimationStudios/OpenSubdiv) | 3.0.5 (Linux/OS X), 3.2.0 (Windows)                                  |  |
| [GLEW](http://glew.sourceforge.net/)                              | 1.10.0                                 |  |
| [OpenEXR](http://www.openexr.com)                                 | 2.2.0                                  |  |
| [OpenImageIO](https://sites.google.com/site/openimageio/home)     | 1.5.11                                 |  |
| [Ptex](http://ptex.us/)                                           | 2.0.30                                 | Y |
| [Pyside](http://wiki.qt.io/PySide)                                | 1.2.2                                  |  |
| [PyOpenGL](https://pypi.python.org/pypi/PyOpenGL/3.1.0)           | 3.1.0                                  | |

Getting and Building the Code
-----------------------------

### 1. Clone the repo:

```bash 
git clone https://github.com/PixarAnimationStudios/USD
```

### 2. Create a build location:
```bash
cd USD
mkdir build
cd build
```

### 3. Build:

#### On Linux 

```bash
cmake                                       \
-DTBB_ROOT_DIR=/path/to/tbb                 \    
-DOPENEXR_LOCATION=/path/to/openexr         \
-DOPENSUBDIV_ROOT_DIR=/path/to/opensubdiv   \
-DPTEX_LOCATION=/path/to/ptex               \
-DOIIO_LOCATION=/path/to/openimageio        \
-DBOOST_ROOT=/path/to/boost                 \
..

cmake --build . --target install -- -j <NUM_CORES>
```

#### On OS X (experimental)

The following will generate an Xcode project that can be used to build USD.
See notes in the [Supported Platforms](#supported-platforms) section
for more information.

```bash
cmake                                       \
-G "Xcode"                                  \
-DTBB_ROOT_DIR=/path/to/tbb                 \    
-DOPENEXR_LOCATION=/path/to/openexr         \
-DOPENSUBDIV_ROOT_DIR=/path/to/opensubdiv   \
-DPTEX_LOCATION=/path/to/ptex               \
-DOIIO_LOCATION=/path/to/openimageio        \
-DBOOST_ROOT=/path/to/boost                 \
..

cmake --build . --target install -- -j <NUM_CORES>
```

#### On Windows (experimental)

The following will generate a Visual Studio 2015 (the minimum required version)
sln file which can be used to build USD. See notes in the 
[Supported Platforms](#supported-platforms) section for more information.

```cmd.exe
"C:\Program Files\CMake\bin\cmake.exe"      ^
-G "Visual Studio 14 Win64"                 ^
-DTBB_ROOT_DIR=C:\path\to\tbb               ^
-DOPENEXR_LOCATION=C:\path\to\openexr       ^
-DOPENSUBDIV_ROOT_DIR=C:\path\to\opensubdiv ^
-DPTEX_LOCATION=C:\path\to\ptex             ^
-DOIIO_LOCATION=C:\path\to\openimageio      ^
-DBOOST_ROOT=C:\path\to\boost               ^
..

cmake --build . --config Release --target install -- /m:%NUMBER_OF_PROCESSORS%
```  

Scripts for building USD and its dependencies on the supported platforms
are located in the build_scripts/ subdirectory. You can find documentation
for these scripts in [Build Scripts](BUILDING.md#build-scripts).

There are many options you can specify throughout the build, such as
building third-party software plugins, disabling specific components, 
and enabling developer options. You can find information on these in 
[Advanced Build Configuration](BUILDING.md).

> Note 1: You will need to update the sample paths with your own.

> Note 2: You may have to supply additional defines to cmake if you have many 
> versions of packages installed. 

### 4. Try it out:

Launch usdview with a sample asset.

```bash
$ export PYTHONPATH=$PYTHONPATH:USD_INSTALL_ROOT/lib/python
$ usdview extras/usd/tutorials/convertingLayerFormats/Sphere.usda
```

> Note: Replace ```USD_INSTALL_ROOT``` with the location set in your build,
> usually via ```CMAKE_INSTALL_PREFIX```.

Contributing
------------

If you'd like to contribute to USD (and we appreciate the help!), please see
the [Contributing](http://openusd.org/docs/Contributing-to-USD.html) page in the
documentation for more information.
