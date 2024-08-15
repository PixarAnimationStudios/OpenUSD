//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_VALUE_ARG_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_VALUE_ARG_HPP

# include "pxr/external/boost/python/detail/copy_ctor_mutates_rhs.hpp"
# include <boost/mpl/if.hpp>
# include "pxr/external/boost/python/detail/indirect_traits.hpp"

namespace boost { namespace python { namespace detail { 

template <class T>
struct value_arg
  : mpl::if_<
        copy_ctor_mutates_rhs<T>
      , T
      , typename add_lvalue_reference<
            typename add_const<T>::type
        >::type
  >
{};
  
}}} // namespace boost::python::detail

#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_VALUE_ARG_HPP
