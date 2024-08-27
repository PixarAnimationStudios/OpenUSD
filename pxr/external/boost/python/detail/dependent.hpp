//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_DEPENDENT_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_DEPENDENT_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/dependent.hpp>
#else

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

// A way to turn a concrete type T into a type dependent on U. This
// keeps conforming compilers (those implementing proper 2-phase
// name lookup for templates) from complaining about incomplete
// types in situations where it would otherwise be inconvenient or
// impossible to re-order code so that all types are defined in time.

// One such use is when we must return an incomplete T from a member
// function template (which must be defined in the class body to
// keep MSVC happy).
template <class T, class U>
struct dependent
{
    typedef T type;
};

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_DEPENDENT_HPP
