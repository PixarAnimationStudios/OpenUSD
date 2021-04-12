3rd Party Library and Application Versions
------------------------------------------

USD relies on an ecosystem of libraries and applications.  This page outlines
the versions of these libraries used and tested against at Pixar as well as
ones that have been tried by others.

Note that not everything here is required, see README.md for more information
about which are required and which are optional for the various subsystems.

## Tested

Our test machines have the following software versions installed

| Software      | Linux                | macOS                        | Windows                        |
| ------------- | -------------------- | ---------------------------- | ------------------------------ |
| OS            | CentOS Linux 7       | 10.14.6                      | Windows 10                     |
| C++ Compiler  | gcc 6.3.1            | Apple LLVM 10.0.0 (Xcode 10.1) | MSVC 14.0 (Visual Studio 2015) |
| CMake         | 3.14.6               | 3.16.5                       | 3.16.5                         |
| Python        | 2.7.16, 3.6.8        | 2.7.10, 3.7.7                | 2.7.12, 3.7.4                  |
| Boost         | 1.61.0, 1.70.0       | 1.61.0, 1.70.0               | 1.61.0, 1.70.0                 |
| Intel TBB     | 2017 Update 6        | 2017 Update 6                | 2017 Update 6                  |
| OpenSubdiv    | 3.4.3                | 3.4.3                        | 3.4.3                          |
| OpenImageIO   | 2.1.16.0             | 2.1.16.0                     | 2.1.16.0                       |
| OpenColorIO   | 1.0.9                | 1.1.0                        | 1.1.0                          |
| OSL           | 1.8.12               |                              |                                |
| Ptex          | 2.1.28               | 2.1.28                       | 2.1.28                         |
| PySide2       | 2.0.0~alpha0, 5.14.1 | 5.14.1                       | 5.14.1                         |
| PyOpenGL      | 3.1.5                | 3.1.5                        | 3.1.5                          |
| Embree        | 3.2.2                | 3.2.2                        | 3.2.2                          |
| RenderMan     | 23.5                 | 23.5                         | 23.5                           |
| Alembic       | 1.7.10               | 1.7.10                       | 1.7.10                         |
| OpenEXR       | 2.2.0                | 2.2.0                        | 2.2.0                          |
| MaterialX     | 1.38.0               | 1.38.0                       | 1.38.0                         |
| Jinja2        | 2.0                  |                              |                                |
| Flex          | 2.5.39               |                              |                                |
| Bison         | 2.4.1                |                              |                                |
| Doxygen       | 1.8.14               |                              |                                |
| GraphViz      | 2.40.1               |                              |                                |
| OpenVDB       | 5.2.0, 6.1.0         | 6.1.0                        | 6.1.0                          |
| Vulkan SDK    | 1.2.135.0            | 1.2.135.0                    | 1.2.135.0                      |

## Other Known Versions

These other versions have been known to work as well:

| Software      | Linux        | macOS        | Windows      |
| ------------- | ------------ | ------------ | ------------ |
| C++ Compiler  |              |              | MSVC 15.0 (Visual Studio 2017) |
| Boost         |              |              | 1.65.1 (VS 2017) |
| PySide        | 1.2.2, 1.2.4 | 1.2.4        | 1.2.4        |
| HDF5          | 1.8.11       | 1.8.11       | 1.8.11       |
| OpenImageIO   | 1.9.1        |              |              |
