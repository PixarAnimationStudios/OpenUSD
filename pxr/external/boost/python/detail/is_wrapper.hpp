//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_IS_WRAPPER_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_IS_WRAPPER_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/is_wrapper.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"
# include <boost/mpl/bool.hpp>

namespace PXR_BOOST_NAMESPACE { namespace python {

template <class T> class wrapper;

namespace detail
{
  typedef char (&is_not_wrapper)[2];
  is_not_wrapper is_wrapper_helper(...);
  template <class T>
  char is_wrapper_helper(wrapper<T> const volatile*);

  // A metafunction returning true iff T is [derived from] wrapper<U> 
  template <class T>
  struct is_wrapper
    : mpl::bool_<(sizeof(detail::is_wrapper_helper((T*)0)) == 1)>
  {};

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_IS_WRAPPER_HPP
