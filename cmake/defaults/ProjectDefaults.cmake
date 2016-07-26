#
# Copyright 2016 Pixar
#
# Licensed under the Apache License, Version 2.0 (the "Apache License")
# with the following modification; you may not use this file except in
# compliance with the Apache License and the following modification to it:
# Section 6. Trademarks. is deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the trade
#    names, trademarks, service marks, or product names of the Licensor
#    and its affiliates, except as required to comply with Section 4(c) of
#    the License and to reproduce the content of the NOTICE file.
#
# You may obtain a copy of the Apache License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the Apache License with the above modification is
# distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the Apache License for the specific
# language governing permissions and limitations under the Apache License.
#
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # Set up the rpath such that libraries in lib and lib/python/pxr
    # can both link properly.
    set(CMAKE_INSTALL_RPATH "$ORIGIN:$ORIGIN/../../..:$ORIGIN/../../../lib:$ORIGIN/../lib")
elseif(APPLE)
    set(OSX_ARCHITECTURES "x86_64" CACHE STRING "Build architectures for OSX")
    set(CMAKE_MACOSX_RPATH ON)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_DYLIB_INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/lib" CACHE STRING "install_name path for dylib.")
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib"  CACHE STRING "rpaths separated by semicolons.")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
    if("${isSystemDir}" STREQUAL "-1")
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
    endif()
    message(WARNING "Building USD on Mac OSX is currently experimental.")
elseif(WIN32)
    # Windows specific set up
    message(WARNING "Building USD on Windows is currently experimental.")
endif()

# Allow local includes from source directory.
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Turn on folder usage
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# Enable CTest
enable_testing()
