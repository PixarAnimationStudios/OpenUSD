//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
//  (C) Copyright David Abrahams 2000.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  The author gratefully acknowleges the support of Dragon Systems, Inc., in
//  producing this work.

//  Revision History:
//  04 Mar 01  Some fixes so it will compile with Intel C++ (Dave Abrahams)

#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_CONFIG_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_CONFIG_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

# if defined(_MSC_VER)

#  pragma warning (disable : 4786) // disable truncated debug symbols
#  pragma warning (disable : 4251) // disable exported dll function
#  pragma warning (disable : 4800) //'int' : forcing value to bool 'true' or 'false'
#  pragma warning (disable : 4275) // non dll-interface class

# elif defined(__ICL) && __ICL < 600 // Intel C++ 5

#  pragma warning(disable: 985) // identifier was truncated in debug information

# endif

/*****************************************************************************
 *
 *  Set up dll import/export options:
 *
 ****************************************************************************/

// backwards compatibility:
#if defined(PXR_BOOST_PYTHON_STATIC_LINK) && !defined(PXR_BOOST_PYTHON_STATIC_LIB)
#  define PXR_BOOST_PYTHON_STATIC_LIB
#endif

#if defined(PXR_BOOST_PYTHON_DYNAMIC_LINK) && !defined(PXR_BOOST_PYTHON_DYNAMIC_LIB)
#  define PXR_BOOST_PYTHON_DYNAMIC_LIB
#endif

#if !defined(PXR_BOOST_PYTHON_STATIC_LIB) && !defined(PXR_BOOST_PYTHON_DYNAMIC_LIB)
#  define PXR_BOOST_PYTHON_DYNAMIC_LIB
#endif

#if defined(PXR_BOOST_PYTHON_DYNAMIC_LIB)
#  if defined (_WIN32) || defined(_WIN64)
#     if defined(__GNUC__) || defined(__clang__)
#        define PXR_BOOST_PYTHON_SYMBOL_EXPORT __attribute__((dllexport))
#        define PXR_BOOST_PYTHON_SYMBOL_IMPORT __attribute__((dllimport))
#     else
#        define PXR_BOOST_PYTHON_SYMBOL_EXPORT __declspec(dllexport)
#        define PXR_BOOST_PYTHON_SYMBOL_IMPORT __declspec(dllimport)
#     endif
#  else
#     if defined(__GNUC__) || defined(__clang__)
#        define PXR_BOOST_PYTHON_SYMBOL_EXPORT __attribute__((visibility("default")))
#        define PXR_BOOST_PYTHON_SYMBOL_IMPORT
#     endif
#  endif
#
#  if defined(PXR_BOOST_PYTHON_SYMBOL_EXPORT)
#     if defined(PXR_BOOST_PYTHON_SOURCE)
#        define PXR_BOOST_PYTHON_DECL PXR_BOOST_PYTHON_SYMBOL_EXPORT
#     else
#        define PXR_BOOST_PYTHON_DECL PXR_BOOST_PYTHON_SYMBOL_IMPORT
#     endif
#  endif
#endif

#ifndef PXR_BOOST_PYTHON_DECL
#  define PXR_BOOST_PYTHON_DECL
#endif

# define PXR_BOOST_PYTHON_OFFSETOF offsetof

#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
#define PXR_BOOST_PYTHON_SUPPORTS_PY_SIGNATURES // enables smooth transition
#endif

// Upstream boost::python conditionally defines HAS_CXX11 as part of its
// build system. We define it here unconditionally since we don't support
// older versions of the standard.
#define PXR_BOOST_PYTHON_HAS_CXX11

#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_CONFIG_HPP
