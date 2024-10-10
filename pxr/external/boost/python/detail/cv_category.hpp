//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_CV_CATEGORY_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_CV_CATEGORY_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/cv_category.hpp>
#else
# include "pxr/external/boost/python/detail/type_traits.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

template <bool is_const_, bool is_volatile_>
struct cv_tag
{
    static constexpr bool is_const = is_const_;
    static constexpr bool is_volatile = is_volatile_;
};

typedef cv_tag<false,false> cv_unqualified;
typedef cv_tag<true,false> const_;
typedef cv_tag<false,true> volatile_;
typedef cv_tag<true,true> const_volatile_;

template <class T>
struct cv_category
{
//    static constexpr bool c = is_const<T>::value;
//    static constexpr bool v = is_volatile<T>::value;
    typedef cv_tag<
        is_const<T>::value
      , is_volatile<T>::value
    > type;
};

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_CV_CATEGORY_HPP
