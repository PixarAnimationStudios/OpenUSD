//
// Copyright 2024 Pixar
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Copyright David Abrahams 2004. Distributed under the Boost
// Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef PXR_EXTERNAL_BOOST_PYTHON_DETAIL_INDIRECT_TRAITS_HPP
# define PXR_EXTERNAL_BOOST_PYTHON_DETAIL_INDIRECT_TRAITS_HPP

#include "pxr/pxr.h"
#include "pxr/external/boost/python/common.hpp"

#ifndef PXR_USE_INTERNAL_BOOST_PYTHON
#include <boost/python/detail/indirect_traits.hpp>
#else

#include <type_traits>

namespace PXR_BOOST_NAMESPACE { namespace python {

namespace indirect_traits {

template <class T>
struct is_reference_to_const : std::false_type
{
};

template <class T>
struct is_reference_to_const<T const&> : std::true_type
{
};

template <class T>
struct is_reference_to_function : std::false_type
{
};

template <class T>
struct is_reference_to_function<T&> : std::is_function<T>
{
};

template <class T>
struct is_pointer_to_function : std::false_type
{
};

// There's no such thing as a pointer-to-cv-function, so we don't need
// specializations for those
template <class T>
struct is_pointer_to_function<T*> : std::is_function<T>
{
};

template <class T>
struct is_reference_to_member_function_pointer_impl : std::false_type
{
};

template <class T>
struct is_reference_to_member_function_pointer_impl<T&>
    : std::is_member_function_pointer<typename std::remove_cv<T>::type>
{
};


template <class T>
struct is_reference_to_member_function_pointer
    : is_reference_to_member_function_pointer_impl<T>
{
};

template <class T>
struct is_reference_to_function_pointer_aux
    : std::bool_constant<
          std::is_reference<T>::value &&
          is_pointer_to_function<
              typename std::remove_cv<
                  typename std::remove_reference<T>::type
              >::type
          >::value
      >
{
    // There's no such thing as a pointer-to-cv-function, so we don't need specializations for those
};

template <class T>
struct is_reference_to_function_pointer
    : std::conditional<
          is_reference_to_function<T>::value
        , std::false_type
        , is_reference_to_function_pointer_aux<T>
      >::type
{
};

template <class T>
struct is_reference_to_non_const
    : std::bool_constant<
          std::is_lvalue_reference<T>::value &&
          !is_reference_to_const<T>::value
      >
{
};

template <class T>
struct is_reference_to_volatile : std::false_type
{
};

template <class T>
struct is_reference_to_volatile<T volatile&> : std::true_type
{
};

template <class T>
struct is_reference_to_pointer : std::false_type
{
};

template <class T>
struct is_reference_to_pointer<T*&> : std::true_type
{
};

template <class T>
struct is_reference_to_pointer<T* const&> : std::true_type
{
};

template <class T>
struct is_reference_to_pointer<T* volatile&> : std::true_type
{
};

template <class T>
struct is_reference_to_pointer<T* const volatile&> : std::true_type
{
};

template <class T>
struct is_reference_to_class
    : std::bool_constant<
          std::is_lvalue_reference<T>::value &&
          std::is_class<
              typename std::remove_cv<
                  typename std::remove_reference<T>::type
              >::type
          >::value
      >
{
};

template <class T>
struct is_pointer_to_class
    : std::bool_constant<
          std::is_pointer<T>::value &&
          std::is_class<
              typename std::remove_cv<
                  typename std::remove_pointer<T>::type
              >::type
          >::value
      >
{
};


}

}} // namespace PXR_BOOST_NAMESPACE::python

#endif // PXR_USE_INTERNAL_BOOST_PYTHON
#endif // PXR_EXTERNAL_BOOST_PYTHON_DETAIL_INDIRECT_TRAITS_HPP
