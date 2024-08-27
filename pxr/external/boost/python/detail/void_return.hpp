//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_VOID_RETURN_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_VOID_RETURN_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/void_return.hpp>
#else

# include <boost/config.hpp>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

struct void_return
{
    void_return() {}
 private: 
    void operator=(void_return const&);
};

template <class T>
struct returnable
{
    typedef T type;
};

# ifdef BOOST_NO_VOID_RETURNS
template <>
struct returnable<void>
{
    typedef void_return type;
};

#  ifndef BOOST_NO_CV_VOID_SPECIALIZATIONS
template <> struct returnable<const void> : returnable<void> {};
template <> struct returnable<volatile void> : returnable<void> {};
template <> struct returnable<const volatile void> : returnable<void> {};
#  endif

# endif // BOOST_NO_VOID_RETURNS

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_VOID_RETURN_HPP
