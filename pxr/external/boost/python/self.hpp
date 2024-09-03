//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_SELF_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_SELF_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/self.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python {

#define PXR_BOOST_PYTHON_SELF_IS_CLASS

// Sink self_t into its own namespace so that we have a safe place to
// put the completely general operator templates which operate on
// it. It is possible to avoid this, but it turns out to be much more
// complicated and finally GCC 2.95.2 chokes on it.
namespace self_ns
{
# ifndef PXR_BOOST_PYTHON_SELF_IS_CLASS
  enum self_t { self };
# else 
  struct self_t {};
  extern PXR_BOOST_PYTHON_DECL self_t self;
# endif
}

using self_ns::self_t;
using self_ns::self;

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_SELF_HPP
