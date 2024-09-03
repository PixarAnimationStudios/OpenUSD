//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_UNWRAP_TYPE_ID_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_UNWRAP_TYPE_ID_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/unwrap_type_id.hpp>
#else

# include "pxr/external/boost/python/type_id.hpp"

# include <boost/mpl/bool.hpp>

namespace PXR_BOOST_NAMESPACE { namespace python {

template <class T> class wrapper;

namespace detail { 

template <class T>
inline type_info unwrap_type_id(T*, ...)
{
    return type_id<T>();
}

template <class U, class T>
inline type_info unwrap_type_id(U*, wrapper<T>*)
{
    return type_id<T>();
}

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_UNWRAP_TYPE_ID_HPP
