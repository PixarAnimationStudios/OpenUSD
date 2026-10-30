/*
    nanobind/nanobind.h: Main include file for core nanobind components

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#if __cplusplus < 201703L && (!defined(_MSVC_LANG) || _MSVC_LANG < 201703L)
#  error The nanobind library requires C++17!
#endif

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable: 4702) // unreachable code (e.g. when binding a noreturn function)
   // The next two lines disable warnings that are "just noise" according to Stephan T. Lavavej (a MSFT STL maintainer)
#  pragma warning(disable: 4275) // non dll-interface class 'std::exception' used as base for dll-interface class [..]
#  pragma warning(disable: 4251) // [..] needs to have a dll-interface to be used by clients of class [..]
#endif

#define NB_VERSION_MAJOR 3
#define NB_VERSION_MINOR 2
#define NB_VERSION_PATCH 0
#define NB_VERSION_DEV   1 // A value > 0 indicates a development release

// nb_python.h includes Python.h, which according to
// https://docs.python.org/3/c-api/intro.html#include-files, must be included
// before standard headers because it overrides feature test macros
#include "nb_python.h"

// Core C++ headers that nanobind depends on
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <new>
#if defined(NB_FREE_THREADED)
#  include <atomic>
#endif

// Implementation. The nb_*.h files should only be included through nanobind.h
// IWYU pragma: begin_exports
#include "nb_defs.h"
#include "nb_platform.h"
#include "nb_traits.h"
#include "nb_tuple.h"
#include "nb_backend.h"
#include "nb_descr.h"
#include "nb_types.h"
#include "nb_accessor.h"
#include "nb_error.h"
#include "nb_attr.h"
#include "nb_cast.h"
#include "nb_misc.h"
#include "nb_call.h"
#include "nb_func.h"
#include "nb_class.h"
// IWYU pragma: end_exports

#if defined(_MSC_VER)
#  pragma warning(pop)
#endif
