//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// DEPRECATED HEADER (2006 Jan 12)
// Provided only for backward compatibility.
// The PXR_BOOST_NAMESPACE::python::len() function is now defined in object.hpp.

#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_API_PLACEHOLDER_HPP
#define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_API_PLACEHOLDER_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/api_placeholder.hpp>
#else

#include "pxr/external/boost/python/object.hpp"

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_API_PLACEHOLDER_HPP
