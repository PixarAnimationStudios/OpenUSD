# This branch contains in-progress work on the Windows port for USD. It should be considered experimental at this time and may not build or work as expected. #
---
Universal Scene Description
===========================

Universal Scene Description (USD) is an efficient, scalable system for
authoring, reading, and streaming time-sampled scene description for
interchange between graphics applications.

For more details, please visit the web site [here](http://openusd.org).

Additional Documentation
------------------------

* [User Documentation and Tutorials](http://openusd.org/docs/index.html)
* [API Documentation](http://openusd.org/docs/api/index.html)
* [Advanced Build Configuration](BUILDING.md)


Getting Help
------------

Need help understanding certain concepts in USD? See
[Getting Help with USD](http://openusd.org/docs/Getting-Help-with-USD.html).

If you are experiencing undocumented problems with the software, 
please read our [issue guidelines](ISSUES.md) and file a bug via the
Issues page on the GitHub repository.

Supported Platforms
-------------------

USD is currently supported on Linux platforms and has been built and tested
on CentOS 7 and RHEL 7.

We are actively working on porting USD to both Windows and Mac platforms. 
Support for both platforms should be considered experimental at this time.
Currently, the tree will build on Mac, but will not build on Windows.

Dependencies
------------

| Name | Version |
| ---- | --------- |
| C++ compiler                                                      | GCC 4.8, Clang 3.5, MSVC 14.0(VS 2015) |
| C compiler                                                        | GCC 4.8, Clang 3.5, MSVC 14.0(VS 2015) |
| [CMake](https://cmake.org/documentation/)                         | 2.8.8 (Linux/OS X), 3.1.1 (Windows)    |
| [Python](https://python.org)                                      | 2.7.5              |
| [Boost](https://boost.org)                                        | 1.55 (Linux), 1.61.0 (OS X/Windows)    |
| [OpenEXR](http://www.openexr.com)                                 | 2.2.0              |
| [DoubleConversion](https://github.com/google/double-conversion)   | 1.1.1              |
| [Intel TBB](https://www.threadingbuildingblocks.org/)             | 4.3.1              |
| [OpenSubdiv](https://github.com/PixarAnimationStudios/OpenSubdiv) | 3.0.5              |
| [GLEW](http://glew.sourceforge.net/)                              | 1.10.0             |
| [OpenImageIO](https://sites.google.com/site/openimageio/home)     | 1.5.11             |
| [Ptex](http://ptex.us/)                                           | 2.0.30             |
| [Qt](http://doc.qt.io/qt-4.8)                                     | 4.8.0              |
| [Pyside](http://wiki.qt.io/PySide)                                | 1.2.2              |


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

### 3. Run a Basic build

#### On Linux 

```bash
cmake                                       \
-DTBB_tbb_LIBRARY=/path/to/libtbb.so        \    
-DOPENEXR_LOCATION=/path/to/openexr         \
-DOPENSUBDIV_LOCATION=/path/to/opensubdiv   \
-DPTEX_INCLUDE_DIR=/path/to/ptex            \
-DOIIO_BASE_DIR=/path/to/openimageio        \
-DBOOST_ROOT=/path/to/boost                 \
-DQT_QMAKE_EXECUTABLE=/path/to/qmake        \
..

make -j <NUM_CORES> install
```

#### On OS X (experimental)

The following will generate an Xcode project that can be used to build USD.
See notes in the [Supported Platforms](#supported-platforms) section
for more information.

```bash
cmake                                       \
-G "Xcode"                                  \
-DTBB_tbb_LIBRARY=/path/to/libtbb.dylib     \    
-DOPENEXR_LOCATION=/path/to/openexr         \
-DOPENSUBDIV_LOCATION=/path/to/opensubdiv   \
-DPTEX_INCLUDE_DIR=/path/to/ptex            \
-DOIIO_BASE_DIR=/path/to/openimageio        \
-DBOOST_ROOT=/path/to/boost/include         \
-DQT_QMAKE_EXECUTABLE=/path/to/qmake        \
..

make -j <NUM_CORES> install
```

#### On Windows (experimental)

The following will generate a Visual Studio 2015 (the minimum required version)
sln file which can be used to build USD. See notes in the 
[Supported Platforms](#supported-platforms) section for more information.

```powershell
C:\Program Files\CMake\bin\cmake.exe             ^
    -G "Visual Studio 14 Win64"                  ^
    -DTBB_tbb_LIBRARY=C:\path\to\tbb.lib         ^     
    -DOPENEXR_LOCATION=C:\path\to\openexr        ^ 
    -DOPENSUBDIV_LOCATION=C:\path\to\opensubdiv  ^ 
    -DPTEX_INCLUDE_DIR=C:\path\to\ptex           ^ 
    -DOIIO_BASE_DIR=C:\path\to\openimageio       ^ 
    -DBOOST_ROOT=C:\path\to\boost                ^ 
    -DQT_QMAKE_EXECUTABLE=C:\path\to\qmak        ^
    --build .. --config Release --target install

```  


There are many options you can specify throughout the build, such as
building third-party software plugins, disabling specific components, 
and enabling developer options. You can find information on these in [Build Configuration](BUILDING.md).

> Note 1: You will need to update the sample paths with your own.

> Note 2: You may have to supply additional defines to cmake if you have many versions of packages installed. 


### 4. Try It Out

Launch usdview with a sample asset.

```bash
$ export PYTHONPATH=$PYTHONPATH:USD_INSTALL_ROOT/lib/python
$ usdview extras/usd/tutorials/convertingLayerFormats/sphere.usda
```

Contributing
------------

If you'd like to contribute to USD (and we appreciate the help!), please see
the [Contributing](http://openusd.org/docs/Contributing-to-USD.html) page in the
documentation for more information.
