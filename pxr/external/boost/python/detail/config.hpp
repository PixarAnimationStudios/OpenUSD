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

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/config.hpp>
#else

# include <boost/config.hpp>

# if defined(BOOST_MSVC)

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
#ifdef PXR_BOOST_PYTHON_STATIC_LIB
#  define PXR_BOOST_PYTHON_STATIC_LINK
# elif !defined(PXR_BOOST_PYTHON_DYNAMIC_LIB)
#  define PXR_BOOST_PYTHON_DYNAMIC_LIB
#endif

#if defined(PXR_BOOST_PYTHON_DYNAMIC_LIB)
#  if defined(BOOST_SYMBOL_EXPORT)
#     if defined(PXR_BOOST_PYTHON_SOURCE)
#        define PXR_BOOST_PYTHON_DECL           BOOST_SYMBOL_EXPORT
#        define PXR_BOOST_PYTHON_DECL_FORWARD   BOOST_SYMBOL_FORWARD_EXPORT
#        define PXR_BOOST_PYTHON_DECL_EXCEPTION BOOST_EXCEPTION_EXPORT
#        define PXR_BOOST_PYTHON_BUILD_DLL
#     else
#        define PXR_BOOST_PYTHON_DECL           BOOST_SYMBOL_IMPORT
#        define PXR_BOOST_PYTHON_DECL_FORWARD   BOOST_SYMBOL_FORWARD_IMPORT
#        define PXR_BOOST_PYTHON_DECL_EXCEPTION BOOST_EXCEPTION_IMPORT
#     endif
#  endif
#endif

#ifndef PXR_BOOST_PYTHON_DECL
#  define PXR_BOOST_PYTHON_DECL
#endif

#ifndef PXR_BOOST_PYTHON_DECL_FORWARD
#  define PXR_BOOST_PYTHON_DECL_FORWARD
#endif

#ifndef PXR_BOOST_PYTHON_DECL_EXCEPTION
#  define PXR_BOOST_PYTHON_DECL_EXCEPTION
#endif

# define PXR_BOOST_PYTHON_OFFSETOF offsetof

//  enable automatic library variant selection  ------------------------------// 

#if !defined(PXR_BOOST_PYTHON_SOURCE) && !defined(BOOST_ALL_NO_LIB) && !defined(PXR_BOOST_PYTHON_NO_LIB)
//
// Set the name of our library, this will get undef'ed by auto_link.hpp
// once it's done with it:
//
#define _PXR_BOOST_PYTHON_CONCAT(N, M, m) N ## M ## m
#define PXR_BOOST_PYTHON_CONCAT(N, M, m) _PXR_BOOST_PYTHON_CONCAT(N, M, m)
#define BOOST_LIB_NAME PXR_BOOST_PYTHON_CONCAT(boost_python, PY_MAJOR_VERSION, PY_MINOR_VERSION)
//
// If we're importing code from a dll, then tell auto_link.hpp about it:
//
#ifdef PXR_BOOST_PYTHON_DYNAMIC_LIB
#  define BOOST_DYN_LINK
#endif
//
// And include the header that does the work:
//
#include <boost/config/auto_link.hpp>
#endif  // auto-linking disabled

#undef PXR_BOOST_PYTHON_CONCAT
#undef _PXR_BOOST_PYTHON_CONCAT

#ifndef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
#define PXR_BOOST_PYTHON_SUPPORTS_PY_SIGNATURES // enables smooth transition
#endif

#if !defined(BOOST_ATTRIBUTE_UNUSED) && defined(__GNUC__) && (__GNUC__ >= 4)
#  define BOOST_ATTRIBUTE_UNUSED __attribute__((unused))
#endif

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_CONFIG_HPP
