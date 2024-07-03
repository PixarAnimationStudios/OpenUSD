3rd Party Library and Application Versions
------------------------------------------

USD relies on an ecosystem of libraries and applications. This page outlines
the versions of these libraries used and tested against at Pixar.

Note that not everything here is required, see README.md for more information
about which are required and which are optional for the various subsystems.

## Tested

Our test machines have the following software versions installed.

| Software      | Linux                | macOS                        | Windows                        |
| ------------- | -------------------- | ---------------------------- | ------------------------------ |
| OS            | CentOS Linux 7       | 12.6.3, 14.5 for visionOS  | Windows 10                     |
| C++ Compiler  | gcc 9.3.1            | Apple clang 13.1.6 (Xcode 13.3)<br>Apple clang 15.0.0 (Xcode 15.4) for visionOS | Visual Studio 2017 15.9     |
| CMake         | 3.17.5               | 3.19.5                       | 3.25.3                         |
| Python        | 3.9.16               | 3.9.13                       | 3.9.13                         |
| Boost         | 1.76.0               | 1.78.0                       | 1.76.0                         |
| Intel TBB     | 2020 Update 3        | 2018 Update 1, 2020 Update 3 | 2020 Update 3                  |
| OpenSubdiv    | 3.6.0                | 3.6.0                        | 3.6.0                          |
| OpenImageIO   | 2.3.21.0             | 2.3.21.0                     | 2.3.21.0                       |
| OpenColorIO   | 2.1.3                | 2.1.3                        | 2.1.3                          |
| OSL           | 1.10.9               |                              |                                |
| Ptex          | 2.4.2                | 2.4.2                        | 2.4.2                          |
| Qt for Python | PySide2 5.15.2.1     | PySide6 6.3.1                | PySide2 5.15.2.1               |
| PyOpenGL      | 3.1.5                | 3.1.5                        | 3.1.5                          |
| Embree        | 3.2.2                | 3.13.3                       | 3.2.2                          |
| RenderMan     | 25.3                 | 25.3                         | 25.3                           |
| Alembic       | 1.8.5                | 1.8.5                        | 1.8.5                          |
| OpenEXR       | 3.1.11               | 3.1.11                       | 3.1.11                         |
| MaterialX     | 1.38.10              | 1.38.10                      | 1.38.10                        |
| Jinja2        | 2.0                  |                              |                                |
| Flex          | 2.5.39               |                              |                                |
| Bison         | 2.4.1                |                              |                                |
| Doxygen       | 1.9.6                |                              |                                |
| GraphViz      | 2.40.1               |                              |                                |
| OpenVDB       | 9.1.0                | 7.1.0, 9.1.0                 | 9.1.0                          |
| Vulkan SDK    | 1.3.243.0            | 1.3.243.0                    | 1.3.243.0                      |
| Draco         | 1.3.6                | 1.3.6                        | 1.3.6                          |
