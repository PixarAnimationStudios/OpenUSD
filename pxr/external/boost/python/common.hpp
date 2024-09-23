//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTERNAL_BOOST_PYTHON_COMMON_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_COMMON_HPP

#include "pxr/pxr.h"

// Common definitions and utilities included by all headers

#ifdef PXR_USE_INTERNAL_BOOST_PYTHON

#if PXR_USE_NAMESPACES
#define PXR_BOOST_NAMESPACE PXR_INTERNAL_NS::pxr_boost
#else
#define PXR_BOOST_NAMESPACE pxr_boost
#endif

#define PXR_BOOST_PYTHON_NAMESPACE PXR_BOOST_NAMESPACE::python

// Allow lookups in the boost namespace to accommodate code that
// relied on this library being in the boost namespace as well.
namespace boost { }
namespace PXR_BOOST_NAMESPACE {
    using namespace boost;
}

#else

// Set up a namespace alias so that code that uses pxr_boost::python
// will automatically pick up boost::python.
namespace boost::python { }

#if PXR_USE_NAMESPACES
namespace PXR_INTERNAL_NS::pxr_boost {
#else
namespace pxr_boost {
#endif
    namespace python = ::boost::python;
}

#define PXR_BOOST_NAMESPACE boost
#define PXR_BOOST_PYTHON_NAMESPACE boost::python

#define PXR_BOOST_PYTHON_MODULE BOOST_PYTHON_MODULE

#define PXR_BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS
#define PXR_BOOST_PYTHON_FUNCTION_OVERLOADS BOOST_PYTHON_FUNCTION_OVERLOADS

#ifdef PXR_BOOST_PYTHON_MAX_ARITY
#define BOOST_PYTHON_MAX_ARITY PXR_BOOST_PYTHON_MAX_ARITY
#endif

#ifdef PXR_BOOST_PYTHON_NO_PY_SIGNATURES
#define BOOST_PYTHON_NO_PY_SIGNATURES
#endif

#endif // PXR_USE_INTERNAL_BOOST_PYTHON

#endif
