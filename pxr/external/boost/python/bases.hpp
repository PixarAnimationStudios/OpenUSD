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
# include <boost/mpl/if.hpp>
# include <boost/mpl/bool.hpp>
# include <boost/preprocessor/enum_params_with_a_default.hpp>
# include <boost/preprocessor/enum_params.hpp>

namespace PXR_BOOST_NAMESPACE { namespace python { 

# define PXR_BOOST_PYTHON_BASE_PARAMS BOOST_PP_ENUM_PARAMS_Z(1, PXR_BOOST_PYTHON_MAX_BASES, Base)

  // A type list for specifying bases
  template < BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(PXR_BOOST_PYTHON_MAX_BASES, typename Base, mpl::void_) >
  struct bases : detail::type_list< PXR_BOOST_PYTHON_BASE_PARAMS >::type
  {};

  namespace detail
  {
    template <class T> struct specifies_bases
        : mpl::false_
    {
    };
    
    template < BOOST_PP_ENUM_PARAMS_Z(1, PXR_BOOST_PYTHON_MAX_BASES, class Base) >
    struct specifies_bases< bases< PXR_BOOST_PYTHON_BASE_PARAMS > >
        : mpl::true_
    {
    };
    template <class T, class Prev = bases<> >
    struct select_bases
        : mpl::if_<
                specifies_bases<T>
                , T
                , Prev
          >
    {
    };
  }
# undef PXR_BOOST_PYTHON_BASE_PARAMS
}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_BASES_HPP
