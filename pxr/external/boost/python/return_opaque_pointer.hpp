//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright Gottfried Ganßauge 2003.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
/*
 * Generic Return value converter generator for opaque C++-pointers
 */
# ifndef PXR_EXTERNAL_BOOST_PYTHON_RETURN_OPAQUE_POINTER_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_RETURN_OPAQUE_POINTER_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/return_opaque_pointer.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"
# include "pxr/external/boost/python/opaque_pointer_converter.hpp"
# include "pxr/external/boost/python/detail/force_instantiate.hpp"
# include "pxr/external/boost/python/to_python_value.hpp"
# include "pxr/external/boost/python/detail/type_traits.hpp"
# include "pxr/external/boost/python/detail/value_arg.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python {

namespace detail
{
  template <class Pointee>
  static void opaque_pointee(Pointee const volatile*)
  {
      force_instantiate(opaque<Pointee>::instance);
  }
}

struct return_opaque_pointer
{
    template <class R>
    struct apply
    {
        static_assert( detail::is_pointer<R>::value, "RETURN_OPAQUE_POINTER_EXPECTS_A_POINTER_TYPE");
        
        struct type :  
          PXR_BOOST_NAMESPACE::python::to_python_value<
              typename detail::value_arg<R>::type
          >
        {
            type() { detail::opaque_pointee(R()); }
        };
    };
};

}} // namespace PXR_BOOST_NAMESPACE::python
#endif // PXR_USE_INTERNAL_BOOST_PYTHON
# endif // PXR_EXTERNAL_BOOST_PYTHON_RETURN_OPAQUE_POINTER_HPP
