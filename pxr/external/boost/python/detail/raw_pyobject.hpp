//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_RAW_PYOBJECT_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_RAW_PYOBJECT_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/raw_pyobject.hpp>
#else

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

//
// Define some types which we can use to get around the vagaries of
// PyObject*. We will use these to initialize object instances, and
// keep them in namespace detail to make sure they stay out of the
// hands of users. That is much simpler than trying to grant
// friendship to all the appropriate parties.
//

// New references are normally checked for null
struct new_reference_t;
typedef new_reference_t* new_reference;

// Borrowed references are assumed to be non-null
struct borrowed_reference_t;
typedef borrowed_reference_t* borrowed_reference;

// New references which aren't checked for null
struct new_non_null_reference_t;
typedef new_non_null_reference_t* new_non_null_reference;

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_RAW_PYOBJECT_HPP
