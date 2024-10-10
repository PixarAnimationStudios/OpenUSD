//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
//  (C) Copyright Samuli-Petrus Korhonen 2017.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  The author gratefully acknowleges the support of NMR Solutions, Inc., in
//  producing this work.

//  Revision History:
//  15 Feb 17  Initial version

#ifndef PXR_EXTERNAL_BOOST_PYTHON_NUMPY_CONFIG_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_NUMPY_CONFIG_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/numpy/config.hpp>
#else

/*****************************************************************************
 *
 *  Set up dll import/export options:
 *
 ****************************************************************************/

// backwards compatibility:
#ifdef PXR_BOOST_NUMPY_STATIC_LIB
#  define PXR_BOOST_NUMPY_STATIC_LINK
# elif !defined(PXR_BOOST_NUMPY_DYNAMIC_LIB)
#  define PXR_BOOST_NUMPY_DYNAMIC_LIB
#endif

#if defined(PXR_BOOST_NUMPY_DYNAMIC_LIB)
#  if defined (_WIN32) || defined(_WIN64)
#     if defined(__GNUC__) || defined(__clang__)
#        define PXR_BOOST_NUMPY_SYMBOL_EXPORT __attribute__((dllexport))
#        define PXR_BOOST_NUMPY_SYMBOL_IMPORT __attribute__((dllimport))
#     else
#        define PXR_BOOST_NUMPY_SYMBOL_EXPORT __declspec(dllexport)
#        define PXR_BOOST_NUMPY_SYMBOL_IMPORT __declspec(dllimport)
#     endif
#  else
#     if defined(__GNUC__) || defined(__clang__)
#        define PXR_BOOST_NUMPY_SYMBOL_EXPORT __attribute__((visibility("default")))
#        define PXR_BOOST_NUMPY_SYMBOL_IMPORT
#     endif
#  endif
#
#  if defined(PXR_BOOST_NUMPY_SYMBOL_EXPORT)
#     if defined(PXR_BOOST_NUMPY_SOURCE)
#        define PXR_BOOST_NUMPY_DECL PXR_BOOST_NUMPY_SYMBOL_EXPORT
#     else
#        define PXR_BOOST_NUMPY_DECL PXR_BOOST_NUMPY_SYMBOL_IMPORT
#     endif
#  endif
#endif

#ifndef PXR_BOOST_NUMPY_DECL
#  define PXR_BOOST_NUMPY_DECL
#endif

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_NUMPY_CONFIG_HPP
