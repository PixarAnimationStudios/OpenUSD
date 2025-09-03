//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_TRAITS_H
#define PXR_EXEC_VDF_TRAITS_H

/// \file

#include "pxr/pxr.h"

#include <map>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Helper template for determining whether or not equality comparison is a
// valid operation for a given type.
//
// Primary template, evaluates to false.
template <typename, typename = void>
constexpr bool Vdf_IsEqualityComparableHelper = false;

// Evaluates to true if there is an equality operator defined for type T.
template <typename T>
constexpr bool Vdf_IsEqualityComparableHelper<
    T, std::void_t<decltype(std::declval<T>() == std::declval<T>())>> = true;


/// Variable template that returns `true` if equality comparison is a valid
/// operation for type \p T.
///
/// For compound types, specializations must be provided along the lines of the
/// following example:
///
/// ```cpp
/// // ContainerType<T> is equality comparable iff T is equality comparable.
/// template <typename T>
/// constexpr bool VdfIsEqualityComparable<ContainerType<T>> =
///    VdfIsEqualityComparable<T>;
/// ```
///
template <typename T>
constexpr bool VdfIsEqualityComparable =
    Vdf_IsEqualityComparableHelper<T>;

// std::vector<T> is equality comparable iff T is equality comparable.
template <class T, class Allocator>
constexpr bool VdfIsEqualityComparable<std::vector<T, Allocator>> =
    VdfIsEqualityComparable<T>;

// std::pair<T1, T2> is equality comparable iff T1 and T2 are equality
// comparable.
template <class T1, class T2>
constexpr bool VdfIsEqualityComparable<std::pair<T1, T2>> =
    VdfIsEqualityComparable<T1> && VdfIsEqualityComparable<T2>;

// std::map<Key, T> is equality comparable iff Key and T are equality
// comparable.
template <class Key, class T, class Compare, class Allocator>
constexpr bool VdfIsEqualityComparable<
    std::map<Key, T, Compare, Allocator>> =
    VdfIsEqualityComparable<Key> && VdfIsEqualityComparable<T>;

// std::unordered_map<Key, T> is equality comparable iff Key and T are equality
// comparable.
template <class Key, class T, class Hash, class KeyEqual, class Allocator>
constexpr bool VdfIsEqualityComparable<
    std::unordered_map<Key, T, Hash, KeyEqual, Allocator>> =
    VdfIsEqualityComparable<Key> && VdfIsEqualityComparable<T>;


// Helper template that returns if a type is small, but only performs the size
// check if the second template parameter is true. This allows us to prevent
// invoking sizeof(T) when T is forward declared.
// 
// Primary template evaluates to false.
template <typename, bool>
constexpr bool Vdf_AndTypeIsSmall = false;

// Specialization that performs the size check when the second template
// parameter is true.
template <typename T>
constexpr bool Vdf_AndTypeIsSmall<T, true> = sizeof(T) <= sizeof(void*);

/// Template that evaluates to either `T` or `const T &` depending on whether
/// \p T best be passed as value or const reference.
/// 
/// The heuristic here is as follows:
/// * `T` if \p T is a ptr, arithmetic type, or enum, and also <= size of a ptr
/// * `const T &` in all other cases
/// 
/// Note that this heuristic is a best guess given a generic type `T`, but if
/// the type is known the call site may be able to make a more informed decision
/// on how to pass parameters and return values without the use of this
/// facility.
/// 
template <typename T>
using VdfByValueOrConstRef = typename std::conditional_t<
    std::is_pointer_v<T> ||
    Vdf_AndTypeIsSmall<T, std::is_arithmetic_v<T>> ||
    Vdf_AndTypeIsSmall<T, std::is_enum_v<T>>,
    T, const T &>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
