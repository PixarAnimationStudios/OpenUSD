//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2002.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_POINTER_TYPE_ID_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_POINTER_TYPE_ID_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/converter/pointer_type_id.hpp>
#else

# include "pxr/external/boost/python/type_id.hpp"
# include "pxr/external/boost/python/detail/type_traits.hpp"

namespace PXR_BOOST_NAMESPACE { namespace python { namespace converter { 

namespace detail
{
  template <bool is_ref = false>
  struct pointer_typeid_select
  {
      template <class T>
      static inline type_info execute(T*(*)() = 0)
      {
          return type_id<T>();
      }
  };

  template <>
  struct pointer_typeid_select<true>
  {
      template <class T>
      static inline type_info execute(T* const volatile&(*)() = 0)
      {
          return type_id<T>();
      }
    
      template <class T>
      static inline type_info execute(T*volatile&(*)() = 0)
      {
          return type_id<T>();
      }
    
      template <class T>
      static inline type_info execute(T*const&(*)() = 0)
      {
          return type_id<T>();
      }

      template <class T>
      static inline type_info execute(T*&(*)() = 0)
      {
          return type_id<T>();
      }
  };
}

// Usage: pointer_type_id<T>()
//
// Returns a type_info associated with the type pointed
// to by T, which may be a pointer or a reference to a pointer.
template <class T>
type_info pointer_type_id(T(*)() = 0)
{
    return detail::pointer_typeid_select<
          PXR_BOOST_NAMESPACE::python::detail::is_lvalue_reference<T>::value
        >::execute((T(*)())0);
}

}}} // namespace PXR_BOOST_NAMESPACE::python::converter

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_CONVERTER_POINTER_TYPE_ID_HPP
