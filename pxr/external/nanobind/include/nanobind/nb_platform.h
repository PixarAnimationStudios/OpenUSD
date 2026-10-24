/*
    nanobind/nb_platform.h: this file computes the platform ABI tag, a string
    capturing the properties of the compilation environment that determine
    whether two binaries may call each other at all. A mismatch means that no
    compatible backend exists. The other binary compatibility contracts are
    described in nb_backend.h.

    The implementation of this file is designed to be compatible with @rwgk's
    https://github.com/pybind/pybind11/blob/master/include/pybind11/conduit/pybind11_platform_abi_id.h
    though the resulting tag string is specific to nanobind.

    Use of this source code is governed by a BSD-style license that can be
    found in the LICENSE file.
*/

#pragma once

// Include a libstdc++ header so that _GLIBCXX_USE_CXX11_ABI is defined below
// regardless of include order
#include <cstddef>

#if defined(__MINGW32__)
#  define NB_COMPILER_TYPE "mingw"
#elif defined(__CYGWIN__)
#  define NB_COMPILER_TYPE "gcc_cygwin"
#elif defined(_MSC_VER)
#  define NB_COMPILER_TYPE "msvc"
#elif defined(__clang__) || defined(__GNUC__)
#  define NB_COMPILER_TYPE "system" // Assumed compatible with system compiler.
#else
#  error "Unknown compiler type. Please revise this code."
#endif

// Catch other conditions that imply ABI incompatibility
// - MSVC builds with different CRT versions
// - An anticipated MSVC ABI break ("vNext")
// - Builds using libc++ with unstable ABIs
// - Builds using libstdc++ with the legacy (pre-C++11) ABI, etc.
#if defined(_MSC_VER)
#  if defined(_MT) && defined(_DLL) // Corresponding to CL command line options /MD or /MDd.
#    if (_MSC_VER) / 100 == 19
#      define NB_BUILD_ABI "_md_mscver19"
#    else
#      error "Unknown MSVC major version. Please revise this code."
#    endif
#  elif defined(_MT) // Corresponding to CL command line options /MT or /MTd.
#    define NB_BUILD_ABI "_mt_mscver" NB_TOSTRING(_MSC_VER)
#  else
#    if (_MSC_VER) / 100 == 19
#      define NB_BUILD_ABI "_none_mscver19"
#    else
#      error "Unknown MSVC major version. Please revise this code."
#    endif
#  endif
#elif defined(_LIBCPP_ABI_VERSION) // https://libcxx.llvm.org/DesignDocs/ABIVersioning.html
#    define NB_BUILD_ABI "_libcpp_abi" NB_TOSTRING(_LIBCPP_ABI_VERSION)
#elif defined(_GLIBCXX_USE_CXX11_ABI)
#  if defined(__NVCOMPILER) && !defined(__GXX_ABI_VERSION)
#    error  "Unknown platform or compiler (_GLIBCXX_USE_CXX11_ABI). Please revise this code."
#  endif
#  if defined(__GXX_ABI_VERSION) && __GXX_ABI_VERSION < 1002 || __GXX_ABI_VERSION >= 2000
#    error "Unknown platform or compiler (__GXX_ABI_VERSION). Please revise this code."
#  endif
#  define NB_BUILD_ABI "_libstdcpp_gxx_abi_1xxx_use_cxx11_abi_" NB_TOSTRING(_GLIBCXX_USE_CXX11_ABI)
#else
#  error "Unknown platform or compiler. Please revise this code."
#endif

// MSVC debug builds and libstdc++'s _GLIBCXX_DEBUG mode are not
// ABI-compatible with regular builds (standard container layouts change)
#if (defined(_MSC_VER) && defined(_DEBUG)) || defined(_GLIBCXX_DEBUG)
#  define NB_BUILD_TYPE "_debug"
#else
#  define NB_BUILD_TYPE ""
#endif

// Free-threaded extensions lay out per-type state differently
#if defined(NB_FREE_THREADED)
#  define NB_FREE_THREADED_ABI "_ft"
#else
#  define NB_FREE_THREADED_ABI ""
#endif

// Separate one pre-release's boundary from the next; empty in releases. The
// release version is part of the tag because the dev counter restarts at 1
// with every release cycle
#if NB_VERSION_DEV > 0
#  define NB_VERSION_DEV_STR                                                   \
       "_" NB_TOSTRING(NB_VERSION_MAJOR) "_" NB_TOSTRING(NB_VERSION_MINOR)     \
       "_" NB_TOSTRING(NB_VERSION_PATCH) "_dev" NB_TOSTRING(NB_VERSION_DEV)
#else
#  define NB_VERSION_DEV_STR ""
#endif

// Tag to determine if inter-library C++ function calls are safe
#define NB_PLATFORM_ABI_TAG                                                    \
    "nanobind" NB_VERSION_DEV_STR "_"                                          \
        NB_COMPILER_TYPE NB_BUILD_ABI NB_BUILD_TYPE NB_FREE_THREADED_ABI
