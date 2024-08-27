//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_ENABLE_IF_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_ENABLE_IF_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/enable_if.hpp>
#else

# include "pxr/external/boost/python/detail/sfinae.hpp"
# include <boost/detail/workaround.hpp>

#if !defined(BOOST_NO_SFINAE)
#  include <boost/utility/enable_if.hpp>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail { 

template <class C, class T = int>
struct enable_if_arg
  : enable_if<C,T>
{};
             
template <class C, class T = int>
struct disable_if_arg
  : disable_if<C,T>
{};
             
template <class C, class T = void>
struct enable_if_ret
  : enable_if<C,T>
{};
             
template <class C, class T = void>
struct disable_if_ret
  : disable_if<C,T>
{};
             
}}} // namespace PXR_BOOST_NAMESPACE::python::detail

# endif

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_ENABLE_IF_HPP
