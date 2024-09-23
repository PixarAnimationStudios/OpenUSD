//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

# ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_RESULT_HPP
#  define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_RESULT_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/result.hpp>
#else

#  include <boost/type.hpp>

#  include "pxr/external/boost/python/detail/preprocessor.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail {

// Defines a family of overloaded function which, given x, a function
// pointer, member [function] pointer, or an AdaptableFunction object,
// returns a pointer to type<R>*, where R is the result type of
// invoking the result of bind(x).
//
// In order to work around bugs in deficient compilers, if x might be
// an AdaptableFunction object, you must pass OL as a second argument
// to get this to work portably.

template <class R, class... A>
boost::type<R>* result(R (*)(A...), int = 0)
{
    return 0;
}

#define PXR_BOOST_PYTHON_RESULT_MEMBER_FN(Q, ...)       \
template <class R, class T, class... A>                 \
boost::type<R>* result(R (T::*)(A...) Q, int = 0)       \
{                                                       \
    return 0;                                           \
}

PXR_BOOST_PYTHON_APPLY_QUALIFIERS(PXR_BOOST_PYTHON_RESULT_MEMBER_FN)

#undef PXR_BOOST_PYTHON_RESULT_MEMBER_FN

template <class R, class T>
boost::type<R>* result(R (T::*), int = 0) { return 0; }

template <class X>
boost::type<typename X::result_type>*
result(X const&, short = 0) { return 0; }

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
# endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_RESULT_HPP
