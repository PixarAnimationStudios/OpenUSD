//
// Copyright 2023 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_META_H
