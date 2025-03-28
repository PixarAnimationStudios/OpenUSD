//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TF_META_H
#define PXR_BASE_TF_META_H

#include "pxr/pxr.h"

#include <cstddef>
#include <tuple>
#include <type_traits>

// Some small metaprogramming utilities.

PXR_NAMESPACE_OPEN_SCOPE

// Simple compile-time type list.
template <class... Args> struct TfMetaList {};

// Helper for TfMetaApply.
template<template <class...> class Cls, class List>
struct Tf_MetaApplyImpl;
 
template<template <class...> class Cls, class... Args>
struct Tf_MetaApplyImpl<Cls, TfMetaList<Args...>>
{
    using Type = Cls<Args...>;
};

// Apply \p TypeList<Args...> to class template \p Cls, producing Cls<Args...>
template <template <class...> class Cls, class TypeList>
using TfMetaApply = typename Tf_MetaApplyImpl<Cls, TypeList>::Type;

// TfMetaHead<A1, A2, ... An> -> A1
template <class Head, class...>
using TfMetaHead = Head;

// TfMetaTail<A1, A2, ... An> -> TfMetaList<A2, ... An>.
template <class Head, class... Tail>
using TfMetaTail = TfMetaList<Tail...>;

// TfMetaDecay<A1, A2, ... An> ->
// TfMetaList<std::decay_t<A1>, ... std::decay_t<An>>
template <class... Ts>
using TfMetaDecay = TfMetaList<std::decay_t<Ts>...>;

// TfMetaLength produces an integral_constant<size_t, N> where N is the number
// of \p Xs.
template <class... Xs>
using TfMetaLength = std::integral_constant<size_t, sizeof...(Xs)>;

// Lighter-weight compile-time conditional type selection implementation.
template <bool Condition>
struct Tf_ConditionalImpl {
    template <class T, class>
    using Type = T;
};

template <>
struct Tf_ConditionalImpl<false> {
    template <class, class F>
    using Type = F;
};

// This is a bit lighter weight at compile time than std::conditional because it
// instantiates a separate template for the condition from the selector.
template <bool Cond, class T, class F>
using TfConditionalType =
    typename Tf_ConditionalImpl<Cond>::template Type<T, F>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_META_H
