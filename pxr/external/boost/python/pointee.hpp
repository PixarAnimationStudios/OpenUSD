//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_POINTEE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_POINTEE_HPP

# include "pxr/external/boost/python/detail/prefix.hpp"
# include "pxr/external/boost/python/detail/type_traits.hpp"

namespace boost { namespace python {

namespace detail
{
  template <bool is_ptr = true>
  struct pointee_impl
  {
      template <class T> struct apply : detail::remove_pointer<T> {};
  };

  template <>
  struct pointee_impl<false>
  {
      template <class T> struct apply
      {
          typedef typename T::element_type type;
      };
  };
}

template <class T>
struct pointee
    : detail::pointee_impl<
        detail::is_pointer<T>::value
      >::template apply<T>
{
};

}} // namespace boost::python

#endif // PXR_EXTERNAL_BOOST_PYTHON_POINTEE_HPP
