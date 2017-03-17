Advanced Build Configuration
============================

## Table of Contents
- [Optional Components](#optional-components)
- [Third Party Plugins](#third-party-plugins)
- [Tests](#tests)
- [Other Build Options](#other-build-options)
- [USD Developer Options](#usd-developer-options)
- [Optimization Options](#optimization-options)

## Optional Components

USD contains several optional components that are enabled by default
but may be disabled when invoking cmake. Disabling these components
removes the need for their dependencies when building USD.

##### Imaging

This component contains Hydra, a high-performance graphics rendering engine.

Disable this component by specifying the cmake flag ```PXR_BUILD_IMAGING=FALSE``` when 
invoking cmake. Disabling this component will also disable the [USD Imaging](#usd-imaging)
component.

##### USD Imaging

This component provides the USD imaging delegates for Hydra, as well as
usdview, a standalone native viewer for USD files.

Disable this component by specifying the cmake flag ```PXR_BUILD_USD_IMAGING=FALSE``` when
invoking cmake. Enabling this component will enable the [Imaging](#imaging)
component.

## Third Party Plugins

USD provides several plugins for integration with third-party software packages,
including Maya, Katana, and Alembic. There is additional documentation on each plugin 
[here](http://openusd.org/docs/USD-3rd-Party-Plugins.html).
These plugins are not built by default and must be enabled via the instructions below.

##### Alembic Plugin

Enable the [Alembic](https://github.com/alembic/alembic) plugin in the build
by specifying the cmake flag ```PXR_BUILD_ALEMBIC_PLUGIN=TRUE``` when invoking cmake.
This plugin is compatible with Alembic 1.5.2. The additional dependencies that must be supplied when invoking cmake are:

| Dependency Name    | Description                                                             | Version |
| ------------------ |-----------------------------------------------------------------------  | ------- |
| ALEMBIC_LOCATION   | The location of [Alembic](https://https://github.com/alembic/alembic)   | 1.5.2   |
| HDF5_LOCATION      | The location of [HDF5](https://www.hdfgroup.org/HDF5/)                  | 1.8.11  |

For further information see the documentation on the Alembic plugin [here](http://openusd.org/docs/Alembic-USD-Plugin.html).

##### Maya Plugin

Enable the Maya plugin in the build by specifying the cmake flag ```PXR_BUILD_MAYA_PLUGIN=TRUE``` 
when invoking cmake. This plugin is compatible with Maya 2016. The additional dependencies that must be supplied when invoking cmake are:

| Dependency Name   | Description                                                                                     | Version   |
| ----------------- | -----------------------------------                                                             | --------- |
| MAYA_LOCATION     | The root path to a Maya SDK install                                                             | Maya 2016 EXT2 SP2 |
| MAYA_tbb_LIBRARY  | The location of TBB, this should be the same as TBB_tbb_LIBRARY provided to the core USD build  |           |

For further information see the documentation on the Maya plugin [here](http://openusd.org/docs/Maya-USD-Plugins.html).

##### Katana Plugin

Enable the Katana plugin in the build by specifying the cmake flag ```PXR_BUILD_KATANA_PLUGIN=TRUE``` 
when invoking cmake. This plugin is compatible with Katana 2.0v5. The additional dependencies that must be supplied when invoking cmake are:

| Dependency Name       | Description                           | Version   |
| --------------        | -----------------------------------   | -------   |
| KATANA_API_LOCATION   | The root path to a Katana SDK install.| 2.0v5     |

For further information see our additional documentation on the Katana plugins [here](http://openusd.org/docs/Katana-USD-Plugins.html).

## Tests

Disable unit testing and prevent tests from being built by specifying the cmake flag ```PXR_BUILD_TESTS=FALSE``` 
when invoking cmake.

## Other Build Options

##### Plugin Metadata Location

Each library in the USD core generally has an associated file named 'plugInfo.json' that contains metadata about that library,
such as the schema types provided by that library. These files are consumed by USD's internal plugin system to lazily load
libraries when needed. 

The plugin system requires knowledge of where these metadata files are located. The cmake build will ensure this is set up
properly based on the install location of the build. However, if you plan to relocate these files to a new location after
the build, you must inform the build by setting the cmake variable ```PXR_INSTALL_LOCATION``` to the intended final
directory where these files will be located. This variable may be a ':'-delimited list of paths.

##### Shared library prefix

By default shared libraries will have the prefix 'lib'. This means, for a given
component such as [usdGeom](pxr/usd/lib/usdGeom), the build will generate a corresponding 
libusdGeom object (libusdGeom.so on Linux, libusdGeom.dll on Windows 
and libusdGeom.dylib on Mac). You can change the prefix (or remove it) through 
```PXR_LIB_PREFIX```. For example,

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
| PXR_SET_EXTERNAL_NAMESPACE     | The outer namespace identifier          | ```pxr```     |
| PXR_SET_INTERNAL_NAMESPACE     | The internal namespace identifier       | ```pxrInternal_v_x_y``` (for version x.y.z) |
| PXR_ENABLE_NAMESPACES          | Enable namespaces                       | ```OFF```    |

When enabled, there are a set of macros provided in a generated header, 
pxr/pxr.h, which facilitates using namespaces:

| Macro Name                     | Description                             | 
| ------------------------------ |-----------------------------------------| 
| PXR_NAMESPACE_OPEN_SCOPE       | Opens the namespace scope.                                           |
| PXR_NAMESPACE_CLOSE_SCOPE      | Closes the namespace.                                                |
| PXR_NS                         | Explicit qualification on items, e.g. ```PXR_NS::TfToken foo = ...```|
| PXR_NAMESPACE_USING_DIRECTIVE  | Enacts a using-directive, e.g. ```using namespace PXR_NS;```         |

##### ASCII Parser Editing/Validation

There is an ASCII parser for the USD file format, which can be found in 
[sdf](pxr/usd/lib/sdf/). Most users will not have a need to edit the parser, but 
for the adventurous ones, there are a couple additional requirements.

If you choose to edit the ASCII parsers, make sure 
```PXR_VALIDATE_GENERATED_CODE``` is set to ```TRUE```.  This flag enables tests 
that check the generated code in [sdf](pxr/usd/lib/sdf) and 
[gf](pxr/base/lib/gf).

| Dependency Name                | Description                                                      | Version |
| ------------------------------ |----------------------------------------------------------------- | ------- |
| FLEX_EXECUTABLE                | Path to [flex](http://flex.sourceforge.net/) executable          | 2.5.35  |
| BISON_EXECUTABLE               | Path to [bison](https://www.gnu.org/software/bison/) executable  | 2.4.1   |                        

##### USD Schema Generation

USD generates some code through a process called [schema 
generation]. This process requires the following python modules be installed
and available on the syspath. You can learn more about Schemas and why you 
might want to generate them
[here](http://openusd.org//docs/Generating-New-Schema-Classes.html).

| Python Module Name                                           | Description                                                      | Version |
| ------------------------------------------------------------ |----------------------------------------------------------------- | ------- |
| [Jinja2](http://jinja.pocoo.org/docs/dev/)                   | Jinja is the core code generator of usdGenSchema                 | 2.0     |
| [Argparse](https://docs.python.org/3/library/argparse.html)  | Argparse is used for basic command line arguments                |         |


## Optimization Options

There are certain optimizations that can be enabled in the build.

##### Malloc Library

We've found that USD performs best with allocators such as [Jemalloc](https://github.com/jemalloc/jemalloc).
In support of this, you can specify your own allocator through ```PXR_MALLOC_LIBRARY```.
This variable should be set to a path to a shared object for the allocator. For example,

```bash
-DPXR_MALLOC_LIBRARY:path=/usr/local/lib/libjemalloc.so
```

If none are specified, the default allocator will be used. More information on getting the most out of
USD can be found [Getting the Best Performance with USD](http://openusd.org/docs/Maximizing-USD-Performance.html).

