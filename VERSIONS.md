3rd Party Library and Application Versions
------------------------------------------

USD relies on an ecosystem of libraries and applications.  This page outlines
the versions of these libraries used and tested against at Pixar as well as
ones that have been tried by others.

Note that not everything here is required, see README.md for more information
about which are required and which are optional for the various subsystems.

## Tested

Our test machines have the following software versions installed

| Software      | Linux        | macOS        | Windows      |
| ------------- | ------------ | ------------ | ------------ |
| C++ Compiler  | gcc 4.8.5    | AppleClang 10.0.0 | MSVC 14.0 (Visual Studio 2015) |
| CMake         | 2.8.11, 3.6.3 | 3.4.0, 3.9.0  | 3.7.0-rc1    |
| Python        | 2.7.5        | 2.7.5,2.71.0 | 2.7.5,2.7.12 |
| Boost         | 1.55         | 1.61.0       | 1.61.0       |
| Intel TBB     | 4.4 Update 6 | 4.4 Update 6, 2017.0 | 4.4 Update 6, 2017.0 |
| OpenSubdiv    | 3.0.5, 3.4   | 3.0.5, 3.1.1 | 3.1.1        |
| GLEW          | 2.0.0        | 2.0.0        | 2.0.0        |
| OpenImageIO   | 1.7.14       | 1.7.14       | 1.7.14       |
| OpenColorIO   | 1.1.0        | 1.1.0        | 1.1.0        |
| OSL           | 1.5.12       | 1.5.12       | 1.5.12       |
| Ptex          | 2.1.28       | 2.1.28       | 2.1.28       |
| PySide        | 1.2.2, 1.2.4 | 1.2.1        | 1.2.2        |
| PyOpenGL      | 3.1.0        | 3.1.0        | 3.1.0        |
| Embree        | 2.16.4       | 2.16.4       | 2.16.4       |
| RenderMan     | 22.5         | 22.5         | 22.5         |
| Alembic       | 1.7.1        | 1.7.1        | 1.7.1        |
| OpenEXR       | 2.2.0        | 2.2.0        | 2.2.0        |
| Maya          | 2016.5       | 2017         | 2016 Ext 2   |
| Katana        | 3.0v6        |              |              |
| Houdini       | 16.5         |              |              |
| MaterialX     | 1.36.0       | 1.36.0       | 1.36.0       |
| Jinja2        | 2.0          | 2.0          | 2.0          |
| Flex          | 2.5.39       | 2.5.35       |              |
| Bison         | 2.4.1        | 2.4.1        |              |
| Doxygen       | 1.8.5        |              |              |
| GraphViz      | 2.3.1        |              |              |


## Other Known Versions

These other versions have been known to work as well:

| Software      | Linux        | macOS        | Windows      |
| ------------- | ------------ | ------------ | ------------ |
| C++ Compiler  |              |              | MSVC 15.0 (Visual Studio 2017) |
| Boost         |              |              | 1.65.1 (VS 2017) |
| Alembic       | 1.5.8, 1.7.9 |              |              |
| Maya          | 2018.3       | 2018.3       | 2018.3       |
| PySide2       | 2.0.0~alpha0 |              | 2.0.0~alpha0 |
| HDF5          | 1.8.11       | 1.8.11       | 1.8.11       |
| Houdini       | 17.0         | 16.5, 17.0   | 16.5, 17.0   |
| OpenImageIO   | 1.9.1        |              |              |
