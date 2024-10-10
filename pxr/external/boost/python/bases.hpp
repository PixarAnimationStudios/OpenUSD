//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_BASES_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_BASES_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/bases.hpp>
#else

# include "pxr/external/boost/python/detail/prefix.hpp"
# include "pxr/external/boost/python/detail/type_list.hpp"
# include "pxr/external/boost/python/detail/type_traits.hpp"
# include "pxr/external/boost/python/detail/mpl2/if.hpp"
# include "pxr/external/boost/python/detail/mpl2/bool.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { 

  // A type list for specifying bases
  template <typename... Base>
  struct bases : detail::type_list<Base...>::type
  {};

  namespace detail
  {
    template <class T> struct specifies_bases
        : detail::mpl2::false_
    {
    };
    
    template <class... Base>
    struct specifies_bases< bases< Base... > >
        : detail::mpl2::true_
    {
    };
    template <class T, class Prev = bases<> >
    struct select_bases
        : detail::mpl2::if_<
                specifies_bases<T>
                , T
                , Prev
          >
    {
    };
  }
}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_BASES_HPP
