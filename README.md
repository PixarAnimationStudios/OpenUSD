Universal Scene Description
===========================

Universal Scene Description (USD) is an efficient, scalable system for
authoring, reading, and streaming time-sampled scene description for
interchange between graphics applications.

For more details, please visit the web site [here](http://openusd.org).

HgiWebGPU ([USD-proposals](https://github.com/autodesk-forks/USD-proposals/tree/adsk/feature/webgpu/proposals/hgiwebgpu))
------------
This new graphics backend is a WebGPU based implementation of Hydra 
Graphics Interface (HGI) that allows to execute the HdStorm rasterization 
rendering pipeline on the web with the power of the new WebGPU capabilities. 
It reuses the existing shaders that are converted to SPIR-V by [Shaderc](https://github.com/google/shaderc) 
and then WGSL by [Tint](https://dawn.googlesource.com/tint), finally consumed by a browser with WebGPU feature 
enabled.

#### Getting Started with HgiWebGPU

As we worked on a WebGPU based backend, we developed 2 solutions for different scenarios according 
to their advantages and disadvantages:

- **Dawn-based**: It is used for easier debugging and developing on desktop (Win and macOS).
It can be tested by enabling the `--dawn` option when building using build_usd.py.
To try it out, you can use `usdview` but, due to some limitations for WebGPU,
some features need to be disabled.
```
export HDST_ENABLE_PIPELINE_DRAW_BATCH_GPU_FRUSTUM_CULLING=0
export HD_ENABLE_PACKED_NORMALS=0
export HDX_ENABLE_OIT=0
export HGI_ENABLE_WEBGPU=1
```

- **Emscripten-based**: It is used for rendering and viewing on a web browser.
The WebAssembly content can be tested using [usdviewweb](./pxr/usdImaging/bin/usdviewweb/README.md). For running an example on the browser, please follow the instructions [under usdviewweb folder](./pxr/usdImaging/bin/usdviewweb/README.md).
Please build USD with `--emscripten` option. 

**Limitations**

- OIT is disabled due to missing support of GLSL `atomicAdd` and binding of a 
writable buffer to the vertex shader, which is not supported in HgiWebgpu. Could be worked around as the vertex shader actually doesn't access that attribute
- Issue with Frustum cull
- Packed normals is disabled due to missing support for format HgiFormatPackedInt1010102
- Problems using SPIR-V and shader compilation of "OpenSubdiv::Osd::MTLPatchShaderSource::GetCommonShaderSource()" targeting Vulkan 1.0
- Doesn’t support dome light due to missing support for float32 filtering.
- UV problems because `primitive_ID` is not supported and used when doing `faceVarying` interpolation
- 4GB of memory limitation in WebAssembly
- Not possible to pass write buffers to the vertex stage (e.g. constPrimVars), As defined in https://www.w3.org/TR/webgpu/#dom-gpudevice-createbindgrouplayout in the device timeline section
- Only vertex, fragment and compute shaders are available. Some functionalities use either Tesselation shaders or Geometric Shaders is not supported.
- Missing builtins available in glsl but not by wgsl nor tint reader using SPIR-V:
  - determinant
  - isinf
  - isnan
  - gl_primitiveID
  - barycentric coords
- gl_Pointsize is readonly
- When there are multiple render targets, we exceed the limit of 32  accross all targets https://www.w3.org/TR/webgpu/#dom-supported-limits-maxcolorattachmentbytespersample. This usually happens when selection is included
- After conversion to webassembly, the wasm package has 70MB size, it could be smaller.


Build Status
------------

|         |   Linux   |  Windows  |   macOS   |
|:-------:|:---------:|:---------:|:---------:|
|   dev   | [![Build Status](https://dev.azure.com/PixarAnimationStudios/OpenUSD/_apis/build/status/PixarAnimationStudios.OpenUSD?branchName=dev&amp;jobName=Linux)](https://dev.azure.com/PixarAnimationStudios/OpenUSD/_build/latest?definitionId=2&branchName=dev) | [![Build Status](https://dev.azure.com/PixarAnimationStudios/OpenUSD/_apis/build/status/PixarAnimationStudios.OpenUSD?branchName=dev&amp;jobName=Windows)](https://dev.azure.com/PixarAnimationStudios/OpenUSD/_build/latest?definitionId=2&branchName=dev) | [![Build Status](https://dev.azure.com/PixarAnimationStudios/OpenUSD/_apis/build/status/PixarAnimationStudios.OpenUSD?branchName=dev&amp;jobName=macOS)](https://dev.azure.com/PixarAnimationStudios/OpenUSD/_build/latest?definitionId=2&branchName=dev) |
|  release | [![Build Status](https://dev.azure.com/PixarAnimationStudios/OpenUSD/_apis/build/status/PixarAnimationStudios.OpenUSD?branchName=release&amp;jobName=Linux)](https://dev.azure.com/PixarAnimationStudios/OpenUSD/_build/latest?definitionId=2&branchName=release) | [![Build Status](https://dev.azure.com/PixarAnimationStudios/OpenUSD/_apis/build/status/PixarAnimationStudios.OpenUSD?branchName=release&amp;jobName=Windows)](https://dev.azure.com/PixarAnimationStudios/OpenUSD/_build/latest?definitionId=2&branchName=release) | [![Build Status](https://dev.azure.com/PixarAnimationStudios/OpenUSD/_apis/build/status/PixarAnimationStudios.OpenUSD?branchName=release&amp;jobName=macOS)](https://dev.azure.com/PixarAnimationStudios/OpenUSD/_build/latest?definitionId=2&branchName=release) |

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
[file a bug](https://github.com/PixarAnimationStudios/OpenUSD/issues/new).

Supported Platforms
-------------------

USD is primarily developed on Linux platforms (CentOS 7), but is built, tested 
and supported on macOS and Windows.

Please see [VERSIONS.md](VERSIONS.md) for explicitly tested versions. 

Dependencies
------------

Required:
 - C/C++ compiler
 - [CMake](https://cmake.org/documentation/)
 - [Boost](https://boost.org)
 - [Intel TBB](https://www.threadingbuildingblocks.org/)

Optional:
 - [Python](https://python.org)

See [3rd Party Library and Application Versions](VERSIONS.md) for version information.

Additional dependencies are required for the following components. These 
components may be disabled at build-time. For further details see
[Advanced Build Configuration](BUILDING.md).

**Imaging and USD Imaging**

Required:
 - [OpenSubdiv](https://github.com/PixarAnimationStudios/OpenSubdiv)

Optional:
 - [OpenEXR](http://www.openexr.com)
 - [OpenImageIO](https://sites.google.com/site/openimageio/home)
 - [OpenColorIO](http://opencolorio.org/)
 - [OSL (OpenShadingLanguage)](https://github.com/imageworks/OpenShadingLanguage)
 - [Ptex](http://ptex.us/)                          

**usdview**

Required:

 - [PySide6](http://wiki.qt.io/PySide6) or [PySide2](http://wiki.qt.io/PySide2)
 - [PyOpenGL](https://pypi.python.org/pypi/PyOpenGL/)

Getting and Building the Code
-----------------------------

The simplest way to build USD is to run the supplied `build_usd.py`
script. This script will download required dependencies and build 
and install them along with USD in a given directory. 

Follow the instructions below to run the script with its default behavior, 
which will build the USD core libraries, Imaging, and USD Imaging components.
For more options and documentation, run the script with the `--help`
parameter.

See [Advanced Build Configuration](BUILDING.md) for examples and
additional documentation for running cmake directly.

#### 1. Install prerequisites (see [Dependencies](#dependencies) for required versions)

- Required:
    - C++ compiler:
        - gcc
        - Xcode
        - Microsoft Visual Studio
    - CMake
- Optional (Can be ignored by passing `--no-python` as an argument to `build_usd.py`)
    - Python (required for [bindings and tests](BUILDING.md#python)) 
    - PyOpenGL (required for [usdview](BUILDING.md#usd-imaging))
    - PySide6 or PySide2 (required for [usdview](BUILDING.md#usd-imaging))

#### 2. Download the USD source code

You can download source code archives from [GitHub](https://www.github.com/PixarAnimationStudios/OpenUSD) or use `git` to clone the repository.

```
> git clone https://github.com/PixarAnimationStudios/OpenUSD
Cloning into 'USD'...
```

#### 3. Run the script

##### Linux:

For example, the following will download, build, and install USD's dependencies,
then build and install USD into `/usr/local/USD`.

```
> python USD/build_scripts/build_usd.py /usr/local/USD
```

##### MacOS:

In a terminal, run `xcode-select` to ensure command line developer tools are
installed. Then run the script.

For example, the following will download, build, and install USD's dependencies,
then build and install USD into `/opt/local/USD`.

```
> python USD/build_scripts/build_usd.py /opt/local/USD
```

##### Windows:

Launch the "x64 Native Tools Command Prompt" for your version of Visual Studio
and run the script in the opened shell. Make sure to use the 64-bit (x64) 
command prompt and not the 32-bit (x86) command prompt.

See https://docs.microsoft.com/en-us/cpp/build/how-to-enable-a-64-bit-visual-cpp-toolset-on-the-command-line for more details.

For example, the following will download, build, and install USD's dependencies,
then build and install USD into `C:\USD`.

```
C:\> python USD\build_scripts\build_usd.py "C:\USD"
```

#### 4. Try it out

Set the environment variables specified by the script when it finishes and 
launch `usdview` with a sample asset.

```
> usdview USD/extras/usd/tutorials/convertingLayerFormats/Sphere.usda
```

Contributing
------------

If you'd like to contribute to USD (and we appreciate the help!), please see
the [Contributing](http://openusd.org/docs/Contributing-to-USD.html) page in the
documentation for more information.
