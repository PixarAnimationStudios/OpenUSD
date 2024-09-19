//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_VALUE_IS_XXX_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_VALUE_IS_XXX_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/value_is_xxx.hpp>
#else

# include <boost/config.hpp>
# include "pxr/external/boost/python/detail/mpl2/bool.hpp"

# include "pxr/external/boost/python/detail/type_traits.hpp"
#  include "pxr/external/boost/python/detail/is_xxx.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail {

#  define PXR_BOOST_PYTHON_VALUE_IS_XXX_DEF(name, qualified_name, nargs)    \
template <class X_>                                                     \
struct value_is_##name                                                  \
{                                                                       \
    PXR_BOOST_PYTHON_IS_XXX_DEF(name,qualified_name,nargs)                  \
    BOOST_STATIC_CONSTANT(bool, value = is_##name<                      \
                               typename remove_cv<                      \
                                  typename remove_reference<X_>::type   \
                               >::type                                  \
                           >::value);                                   \
    typedef mpl2::bool_<value> type;                                     \
                                                                        \
};                                                              

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_VALUE_IS_XXX_HPP
