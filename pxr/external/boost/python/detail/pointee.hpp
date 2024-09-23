//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_POINTEE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_POINTEE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/pointee.hpp>
#else

# include "pxr/external/boost/python/detail/type_traits.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

template <bool is_ptr = true>
struct pointee_impl
{
    template <class T> struct apply : remove_pointer<T> {};
};

template <>
struct pointee_impl<false>
{
    template <class T> struct apply
    {
        typedef typename T::element_type type;
    };
};

template <class T>
struct pointee
    : pointee_impl<is_pointer<T>::value>::template apply<T>
{
};

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_POINTEE_HPP
