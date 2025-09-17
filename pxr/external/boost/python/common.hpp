//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXTERNAL_BOOST_PYTHON_COMMON_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_COMMON_HPP

#include "pxr/pxr.h"

// Common definitions and utilities included by all headers

#if PXR_USE_NAMESPACES
#define PXR_BOOST_NAMESPACE PXR_INTERNAL_NS::pxr_boost
#else
#define PXR_BOOST_NAMESPACE pxr_boost
#endif

#define PXR_BOOST_PYTHON_NAMESPACE PXR_BOOST_NAMESPACE::python

#endif
