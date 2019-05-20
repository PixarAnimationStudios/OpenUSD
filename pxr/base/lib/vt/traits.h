//
// Copyright 2016 Pixar
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
#ifndef VT_TRAITS_H
#define VT_TRAITS_H

/// \file vt/traits.h

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"
#include "pxr/base/tf/preprocessorUtils.h"

#include <boost/type_traits/has_trivial_assign.hpp>

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

/// Array concept. By default, types are not arrays.
template <typename T>
struct VtIsArray : public std::false_type {};

// We attempt to use local storage if a given type will fit and if it has a
// cheap copy operation.  By default we only treat types with trivial
// assignments as "cheap to copy".  Typically types that would fit in local
// space but do not have a trivial assignment are not cheap to copy.  E.g. std::
// containers.  Clients can specialize this template for their own types that
// aren't trivially assignable but are cheap to copy to enable local storage.
template <class T>
struct VtValueTypeHasCheapCopy : boost::has_trivial_assign<T> {};

#define VT_TYPE_IS_CHEAP_TO_COPY(T)                                            \
    template <> struct VtValueTypeHasCheapCopy<TF_PP_EAT_PARENS(T)>            \
    : std::true_type {}

// Clients that implement value proxies for VtValue can either derive
// VtValueProxyBase or specialize the VtIsValueProxy template so that VtValue
// recognizes the proxy as a proxy.
struct VtValueProxyBase {};

// Metafunction used by VtValue to determine whether a given type T is a proxy.
template <class T>
struct VtIsValueProxy : std::is_base_of<VtValueProxyBase, T> {};

// Clients may use this macro to indicate their type is a VtValue proxy type.
#define VT_TYPE_IS_VALUE_PROXY(T)                               \
    template <> struct VtIsValueProxy<TF_PP_EAT_PARENS(T)>      \
    : std::true_type {}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // VT_TRAITS_H
