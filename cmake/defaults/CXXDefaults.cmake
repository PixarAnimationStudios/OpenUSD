#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
include(CXXHelpers)
include(Version)
include(Options)

# Require C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_COMPILER_IS_GNUCXX)
    include(gccdefaults)
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    include(clangdefaults)
elseif(MSVC)
    include(msvcdefaults)
endif()

_add_define(GL_GLEXT_PROTOTYPES)
_add_define(GLX_GLXEXT_PROTOTYPES)

# Python bindings for tf require this define.
_add_define(BOOST_PYTHON_NO_PY_SIGNATURES)

# Parts of boost (in particular, boost::hash) rely on deprecated features
# of the STL that have been removed from some implementations under C++17.
# This define tells boost not to use those features.
#
# Under Visual Studio, boost automatically detects that this flag is
# needed so we don't need to define it ourselves. Otherwise, we'll get a
# C4005 macro redefinition warning.
if (NOT MSVC)
    _add_define(BOOST_NO_CXX98_FUNCTION_BASE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    _add_define(BUILD_OPTLEVEL_DEV)
endif()

# Set plugin path environment variable name
set(PXR_PLUGINPATH_NAME PXR_PLUGINPATH_NAME)
if (PXR_OVERRIDE_PLUGINPATH_NAME)
    set(PXR_PLUGINPATH_NAME ${PXR_OVERRIDE_PLUGINPATH_NAME})
    _add_define("PXR_PLUGINPATH_NAME=${PXR_PLUGINPATH_NAME}")
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
        set(PXR_INTERNAL_NAMESPACE "pxrInternal_v${PXR_MAJOR_VERSION}_${PXR_MINOR_VERSION}_${PXR_PATCH_VERSION}")
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

# Set safety/performance configuration
if (PXR_PREFER_SAFETY_OVER_SPEED)
   set(PXR_PREFER_SAFETY_OVER_SPEED "1")
else()
   set(PXR_PREFER_SAFETY_OVER_SPEED "0")
endif()
