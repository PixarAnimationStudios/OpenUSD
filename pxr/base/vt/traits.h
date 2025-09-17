//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_VT_TRAITS_H
#define PXR_BASE_VT_TRAITS_H

/// \file vt/traits.h

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

/// A trait to detect instantiations of VtArray, specialized in array.h.
template <typename T>
struct VtIsArray : public std::false_type {};

/// A trait to detect instantiations of VtArrayEdit, specialized in arrayEdit.h
template <typename T>
struct VtIsArrayEdit : public std::false_type {};

// We attempt to use local storage if a given type will fit and if it has a
// cheap copy operation.  By default we only treat types with trivial
// assignments as "cheap to copy".  Typically types that would fit in local
// space but do not have a trivial assignment are not cheap to copy.  E.g. std::
// containers.  Clients can specialize this template for their own types that
// aren't trivially assignable but are cheap to copy to enable local storage.
template <class T>
struct VtValueTypeHasCheapCopy : std::is_trivially_copy_assignable<T> {};

#define VT_TYPE_IS_CHEAP_TO_COPY(T)                                            \
    template <> struct VtValueTypeHasCheapCopy<TF_PP_EAT_PARENS(T)>            \
    : std::true_type {}

// VtValue supports two kinds of "value proxy":
//
// 1. Typed proxies, where given a proxy type P, we can determine the underlying
// proxied type at compile-time.
//
// 2. Erased proxies, where we cannot know the underlying proxied type at
// compile-time.
//
// Typed proxies are mostly useful from a performance standpoint, where you can
// produce a VtValue that holds an object that is not stored in its own storage
// area.  That is, you can make a VtValue that points at an object you own
// rather than copying/swapping/moving it to the VtValue.
//
// To implement a Typed proxy, either have it derive VtTypedValueProxyBase or
// specialize the VtIsTypedValueProxy class template (possibly by way of the
// VT_TYPE_IS_TYPED_VALUE_PROXY macro).  Then provide an implementation of
// VtGetProxiedObject that accepts `TypedProxy const &` and returns a const
// lvalue reference to the underlying proxied type.  That reference must be
// valid so long as the YourProxyType argument reference is valid.  Like:
//
// ProxiedType const &VtGetProxiedObject(TypedProxy const &);
//
// Erased proxies are mostly useful to enable producing VtValues holding
// "deferred" values; values whose types are not yet loaded in the process.  For
// example, this can be used to produce VtValues holding objects whose types are
// provided in plugins that are not yet loaded.  When a real object instance is
// required, VtValue will call `VtGetErasedProxiedVtValue(YourErasedProxy const
// &)` which must return a pointer to a VtValue holding the underlying proxied
// type.  This can be manufactured "on-the-fly" (with affordances for
// thread-safety).
//
// To implement an Erased proxy, either have it derive VtErasedValueProxyBase or
// specialize the VtIsErasedValueProxy class template (possibly by way of the
// VT_TYPE_IS_ERASED_VALUE_PROXY macro).  Then provide implementations of:
//
// bool VtErasedProxyHoldsType(ErasedProxy const &, std::type_info const &);
// TfType VtGetErasedProxiedTfType(ErasedProxy const &);
// VtValue const *VtGetErasedProxiedVtValue(ErasedProxy const &);
//
// The pointer returned by VtGetErasedProxiedVtValue must be valid as long as
// the ErasedProxy argument reference is valid.
//
// A note on Equality Comparisons.  If a proxy type provides equality
// comparison, then two VtValues that hold the same proxy types will invoke that
// equality comparison when compared.  Otherwise, the underlying proxied objects
// will be compared.  This is beneficial when equality can be checked without
// having to actually instantiate the proxied object (for proxies that load &
// construct the proxied object lazily).

// Clients may derive VtTypedValueProxyBase, specialize VtIsTypedValueProxy,
// or use the VT_TYPE_IS_TYPED_VALUE_PROXY macro to indicate their type is a
// VtValue proxy type.
struct VtTypedValueProxyBase {};
template <class T>
struct VtIsTypedValueProxy : std::is_base_of<VtTypedValueProxyBase, T> {};
#define VT_TYPE_IS_TYPED_VALUE_PROXY(T)                                        \
    template <> struct VtIsTypedValueProxy<TF_PP_EAT_PARENS(T)>                \
    : std::true_type {}

// Base implementation for VtGetProxiedObject (for non-proxy types).
template <class T,
          typename std::enable_if<
              !VtIsTypedValueProxy<T>::value, int>::type = 0>
T const &
VtGetProxiedObject(T const &nonProxy) {
    return nonProxy;
}

// Metafunction to determine the proxied type for a typed proxy.
template <class T>
struct VtGetProxiedType
{
    using type = typename std::decay<
        decltype(VtGetProxiedObject(std::declval<T>()))>::type;
};

// Clients may derive VtErasedValueProxyBase, specialize VtIsErasedValueProxy,
// or use the VT_TYPE_IS_ERASED_VALUE_PROXY macro to indicate their type is a
// VtValue proxy type.
struct VtErasedValueProxyBase {};
template <class T>
struct VtIsErasedValueProxy : std::is_base_of<VtErasedValueProxyBase, T> {};
#define VT_TYPE_IS_ERASED_VALUE_PROXY(T)                                     \
    template <> struct VtIsErasedValueProxy<TF_PP_EAT_PARENS(T)>             \
    : std::true_type {}

// Metafunction to determine whether or not a given type T is a value proxy
// (either typed or type-erased).
template <class T>
struct VtIsValueProxy :
    std::integral_constant<
    bool, VtIsTypedValueProxy<T>::value || VtIsErasedValueProxy<T>::value> {};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_TRAITS_H
