#
# Copyright 2016 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#

# This file contains a set of flags/settings shared between our 
# GCC and Clang configs. This allows clangdefaults and gccdefaults
# to remain minimal, marking the points where divergence is required.
include(Options)

# Enable all warnings.
# Windows builds may encounter linker or other runtime errors with struct/class 
# mismatches. In order for us to catch these issues on all platforms we enable 
# the warning for mismatched-tags
# here for all GCC/Clang builds.
set(_PXR_GCC_CLANG_SHARED_CXX_FLAGS "${_PXR_GCC_CLANG_SHARED_CXX_FLAGS} -Wall -Wformat-security -Wmismatched-tags")

# Errors are warnings in strict build mode.
if (${PXR_STRICT_BUILD_MODE})
    set(_PXR_GCC_CLANG_SHARED_CXX_FLAGS "${_PXR_GCC_CLANG_SHARED_CXX_FLAGS} -Werror")
endif()

# We use hash_map, suppress deprecation warning.
_disable_warning("deprecated")
_disable_warning("deprecated-declarations")

# Suppress unused typedef warnings emanating from boost.
#
# XXX:
# Although we no longer have a boost dependency, we still
# trip this warning in various places in the code and need to
# fix those up before we can remove this.
if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR
    NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 3.6)
    if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" OR
        NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.1)
            _disable_warning("unused-local-typedefs")
    endif()
endif()

# If using pthreads then tell the compiler.  This should automatically cause
# the linker to pull in the pthread library if necessary so we also clear
# PXR_THREAD_LIBS.
if(CMAKE_USE_PTHREADS_INIT)
    set(_PXR_GCC_CLANG_SHARED_CXX_FLAGS "${_PXR_GCC_CLANG_SHARED_CXX_FLAGS} -pthread")
    set(PXR_THREAD_LIBS "")
endif()
