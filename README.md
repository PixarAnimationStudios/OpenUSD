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

| Name | Version | Optional |
| ---- | ------- | :------: |
| C++ compiler                                                      | GCC 4.8, Clang 3.5, MSVC 14.0(VS 2015), MSVC 14.1(VS 2017) |   |
| C compiler                                                        | GCC 4.8, Clang 3.5, MSVC 14.0(VS 2015), MSVC 14.1(VS 2017) |   |
| [CMake](https://cmake.org/documentation/)                         | 2.8.8 (Linux), 3.12 (OS X/Windows)    |   |
| [Python](https://python.org)                                      | 2.7.5                                  | x |
| [Boost](https://boost.org)                                        | 1.55 (Linux), 1.61.0 (OS X/Windows VS 2015), 1.65.1 (Windows VS 2017)    |   |
| [Intel TBB](https://www.threadingbuildingblocks.org/)             | 4.4 Update 6                           |   |

Additional dependencies are required for the following components. These components
may be disabled at build-time, for further details see [Advanced Build Configuration](BUILDING.md).

**Imaging and USD Imaging**

| Name | Version | Optional |
| ---- | ------- | :------: |
| [OpenSubdiv](https://github.com/PixarAnimationStudios/OpenSubdiv) | 3.0.5 (Linux/OS X), 3.2.0 (Windows)         |   |
| [GLEW](http://glew.sourceforge.net/)                              | 2.0.0                                       | x |
| [OpenEXR](http://www.openexr.com)                                 | 2.2.0                                       |   |
| [OpenImageIO](https://sites.google.com/site/openimageio/home)     | 1.5.11                                      | x |
| [OSL (OpenShadingLanguage)](https://github.com/imageworks/OpenShadingLanguage)     | 1.5.12                                      | x |
| [Ptex](http://ptex.us/)                                           | 2.0.30                                      | x |

**usdview**

| Name | Version |
| ---- | ------- |
| [PySide](http://wiki.qt.io/PySide) or [PySide2](http://wiki.qt.io/PySide2) (experimental) | 1.2.2, 2.0.0~alpha0 |
| [PyOpenGL](https://pypi.python.org/pypi/PyOpenGL/3.1.0)                                   | 3.1.0 |

Getting and Building the Code
-----------------------------

The simplest way to build USD is to run the supplied ```build_usd.py``` 
script. This script will download required dependencies and build 
and install them along with USD in a given directory. 

Follow the instructions below to run the script with its default behavior, 
which will build the USD core libraries, Imaging, and USD Imaging components.
For more options and documentation, run the script with the ```--help```
parameter.

See [Advanced Build Configuration](BUILDING.md) for examples and
additional documentation for running cmake directly.

#### 1. Install prerequisites (see [Dependencies](#dependencies) for required versions)

- Required:
    - C++ compiler:
        - gcc
        - Xcode
        - Microsoft Visual Studio
    - NASM (required for Imaging on Windows)
    - CMake
- Optional (Can be ignored by passing `--no-python` as an argument to `build_usd.py`)
    - Python (required for [bindings and tests](BUILDING.md#python)) 
    - PyOpenGL (required for [usdview](BUILDING.md#usd-imaging))
    - PySide or PySide2 (experimental) (required for [usdview](BUILDING.md#usd-imaging))

#### 2. Download the USD source code

You can download source code archives from [GitHub](https://www.github.com/PixarAnimationStudios/USD) or use ```git``` to clone the repository.

```
> git clone https://github.com/PixarAnimationStudios/USD
Cloning into 'USD'...
```

#### 3. Run the script

##### Linux:

For example, the following will download, build, and install USD's dependencies,
then build and install USD into ```/usr/local/USD```.

```
> python USD/build_scripts/build_usd.py /usr/local/USD
```

##### MacOS:

In a terminal, run ```xcode-select``` to ensure command line developer tools are 
installed. Then run the script.

For example, the following will download, build, and install USD's dependencies,
then build and install USD into ```/opt/local/USD```.

```
> python USD/build_scripts/build_usd.py /opt/local/USD
```

##### Windows:

Launch the "Developer Command Prompt" for your version of Visual Studio and 
run the script in the opened shell. Make sure to use the 64-bit (x64) command
prompt and not the 32-bit (x86) command prompt.  (Note if you're trying to
build with Visual Studio 2017, use the "x86 Native Tools Command Prompt for VS
2017").

See https://docs.microsoft.com/en-us/dotnet/framework/tools/developer-command-prompt-for-vs for more details.

For example, the following will download, build, and install USD's dependencies,
then build and install USD into ```C:\Program Files\USD```.

```
C:\> python USD\build_scripts\build_usd.py "C:\Program Files\USD"
```

#### 4. Try it out

Set the environment variables specified by the script when it finishes and 
launch ```usdview``` with a sample asset.

```
> usdview extras/usd/tutorials/convertingLayerFormats/Sphere.usda
```

Contributing
------------

If you'd like to contribute to USD (and we appreciate the help!), please see
the [Contributing](http://openusd.org/docs/Contributing-to-USD.html) page in the
documentation for more information.
