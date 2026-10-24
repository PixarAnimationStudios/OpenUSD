/*
    nanobind/stl/detail/traits.h: detail::is_copy_constructible<T>
    partial overloads for STL types

    Adapted from pybind11.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

/* The builtin `std::is_copy_constructible` type trait merely checks whether
   a copy constructor is present and returns `true` even when the this copy
   constructor cannot be compiled. This a problem for older STL types like
   `std::vector<T>` when `T` is noncopyable. The alternative below recurses
   into STL types to work around this problem. */

template <typename T>
struct is_copy_constructible<
    T, enable_if_t<
           std::is_same_v<typename T::value_type &, typename T::reference> &&
           std::is_copy_constructible_v<T> &&
           !std::is_same_v<T, typename T::value_type>>> {
    static constexpr bool value =
        is_copy_constructible<typename T::value_type>::value;
};

// std::pair is copy-constructible <=> both constituents are copy-constructible
template <typename T1, typename T2>
struct is_copy_constructible<std::pair<T1, T2>> {
    static constexpr bool value =
        is_copy_constructible<T1>::value &&
        is_copy_constructible<T2>::value;
};

// Analogous template for checking copy-assignability
template <typename T, typename SFINAE = int>
struct is_copy_assignable : std::is_copy_assignable<T> { };

template <typename T>
struct is_copy_assignable<T,
                          enable_if_t<std::is_copy_assignable_v<T> &&
                                      std::is_same_v<typename T::value_type &,
                                                     typename T::reference>>> {
    static constexpr bool value = is_copy_assignable<typename T::value_type>::value;
};

template <typename T1, typename T2>
struct is_copy_assignable<std::pair<T1, T2>> {
    static constexpr bool value =
            is_copy_assignable<T1>::value &&
            is_copy_assignable<T2>::value;
};

template <typename T>
constexpr bool is_copy_assignable_v = is_copy_assignable<T>::value;

// Analogous template for checking comparability
template <typename T> using comparable_test = decltype(std::declval<T>() == std::declval<T>());

template <typename T, typename SFINAE = int>
struct is_equality_comparable {
    static constexpr bool value = is_detected_v<comparable_test, T>;
};

template <typename T>
struct is_equality_comparable<T, enable_if_t<is_detected_v<comparable_test, T> &&
                                      std::is_same_v<typename T::value_type &,
                                                     typename T::reference>>> {
    static constexpr bool value = is_equality_comparable<typename T::value_type>::value;
};

template <typename T1, typename T2>
struct is_equality_comparable<std::pair<T1, T2>> {
    static constexpr bool value =
            is_equality_comparable<T1>::value &&
            is_equality_comparable<T2>::value;
};


template <typename T>
constexpr bool is_equality_comparable_v = is_equality_comparable<T>::value;

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)

