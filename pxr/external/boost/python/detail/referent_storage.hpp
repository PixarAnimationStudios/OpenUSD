//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_REFERENT_STORAGE_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_REFERENT_STORAGE_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/referent_storage.hpp>
#else
# include <boost/mpl/if.hpp>
# include <boost/type_traits/aligned_storage.hpp>
# include <cstddef>

namespace PXR_BOOST_NAMESPACE { namespace python { namespace detail {

template <std::size_t size, std::size_t alignment = std::size_t(-1)>
struct aligned_storage
{
  union type
  {
    typename ::boost::aligned_storage<size, alignment>::type data;
    char bytes[size];
  };
};
      
  // Compute the size of T's referent. We wouldn't need this at all,
  // but sizeof() is broken in CodeWarriors <= 8.0
  template <class T> struct referent_size;
  
  
  template <class T>
  struct referent_size<T&>
  {
      BOOST_STATIC_CONSTANT(
          std::size_t, value = sizeof(T));
  };

// A metafunction returning a POD type which can store U, where T ==
// U&. If T is not a reference type, returns a POD which can store T.
template <class T>
struct referent_storage
{
    typedef typename aligned_storage<
        ::PXR_BOOST_NAMESPACE::python::detail::referent_size<T>::value, 
        alignment_of<T>::value
    >::type type;
};

}}} // namespace PXR_BOOST_NAMESPACE::python::detail

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_REFERENT_STORAGE_HPP
