//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_SLICE_NIL_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_SLICE_NIL_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/slice_nil.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"
# include "pxr/external/boost/python/object_core.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace api {

class slice_nil : public object
{
 public:
    slice_nil() : object() {}
};

# ifndef _ // Watch out for GNU gettext users, who #define _(x)
static const slice_nil _ = slice_nil();
# endif 

template <class T>
struct slice_bound
{
    typedef object type;
};

template <>
struct slice_bound<slice_nil>
{
    typedef slice_nil type;
};

}

using api::slice_nil;
# ifndef _ // Watch out for GNU gettext users, who #define _(x)
using api::_;
# endif 

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_SLICE_NIL_HPP
