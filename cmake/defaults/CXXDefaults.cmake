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
include(CXXHelpers)
include(Version)
include(Options)

if (CMAKE_COMPILER_IS_GNUCXX)
    include(gccdefaults)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    include(clangdefaults)
elseif(MSVC)
    include(msvcdefaults)
endif()

# We don't use this prefix in the open source build
# because CMake bakes out full paths.
_add_define(BUILD_COMPONENT_SRC_PREFIX="")

_add_define(GL_GLEXT_PROTOTYPES)
_add_define(GLX_GLXEXT_PROTOTYPES)

# Python bindings for tf require this define.
_add_define(BOOST_PYTHON_NO_PY_SIGNATURES)

# Maya seems to require this
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    _add_define(LINUX)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    _add_define(BUILD_OPTLEVEL_OPT)
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    _add_define(BUILD_OPTLEVEL_OPT)
elseif(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    _add_define(BUILD_OPTLEVEL_OPT)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    _add_define(BUILD_OPTLEVEL_DEV)
endif()

set(_PXR_CXX_FLAGS ${_PXR_CXX_FLAGS} ${_PXR_CXX_WARNING_FLAGS})

# CMake list to string.
string(REPLACE ";" " "  _PXR_CXX_FLAGS "${_PXR_CXX_FLAGS}")

# Set namespace configuration.
if (PXR_ENABLE_NAMESPACES)
    set(PXR_USE_NAMESPACES "1")

    if (PXR_SET_EXTERNAL_NAMESPACE)
        set(PXR_EXTERNAL_NAMESPACE ${PXR_SET_EXTERNAL_NAMESPACE})
    else()
        set(PXR_EXTERNAL_NAMESPACE "pxr")
    endif()

    if (PXR_SET_INTERNAL_NAMESPACE)
        set(PXR_INTERNAL_NAMESPACE ${PXR_SET_INTERNAL_NAMESPACE})
    else()
        set(PXR_INTERNAL_NAMESPACE "pxrInternal_v${PXR_MAJOR_VERSION}_${PXR_MINOR_VERSION}")
    endif()

    message(STATUS "C++ namespace configured to (external) ${PXR_EXTERNAL_NAMESPACE}, (internal) ${PXR_INTERNAL_NAMESPACE}")
else()
    set(PXR_USE_NAMESPACES "0")
    message(STATUS "C++ namespaces disabled.")
endif()

# Set Python configuration
if (PXR_ENABLE_PYTHON_SUPPORT)
    set(PXR_PYTHON_SUPPORT_ENABLED "1")
else()
    set(PXR_PYTHON_SUPPORT_ENABLED "0")
endif()

# XXX: This is a workaround for an issue in which Python headers unequivocally
# redefine macros defined in standard library headers. This behavior 
# prevents users from running strict builds with PXR_STRICT_BUILD_MODE
# as the redefinition warnings would cause build failures.
#
# The python official docs call this out here:
# https://docs.python.org/2/c-api/intro.html#include-files
#
# The long term plan is to adhere to the required behavior.
include_directories(SYSTEM ${PYTHON_INCLUDE_DIR})
