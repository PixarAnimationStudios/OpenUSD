//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_VT_HASH_H
#define PXR_BASE_VT_HASH_H

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"
#include "pxr/base/tf/hash.h"
#include <typeinfo>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

namespace Vt_HashDetail {

// Issue a coding error when we attempt to hash a t.
VT_API void _IssueUnimplementedHashError(std::type_info const &t);

// A constexpr function that determines hashability.
template <class T, class = decltype(TfHash()(std::declval<T>()))>
constexpr bool _IsHashable(long) { return true; }
template <class T>
constexpr bool _IsHashable(...) { return false; }

// Hash implementations -- We're using an overload resolution ordering trick
// here (long vs ...) so that we pick TfHash() if possible, otherwise
// we issue a runtime error.
template <class T, class = decltype(TfHash()(std::declval<T>()))>
inline size_t
_HashValueImpl(T const &val, long)
{
    return TfHash()(val);
}

template <class T>
inline size_t
_HashValueImpl(T const &val, ...)
{
    Vt_HashDetail::_IssueUnimplementedHashError(typeid(T));
    return 0;
}

} // Vt_HashDetail


/// A constexpr function that returns true if T is hashable via VtHashValue,
/// false otherwise.  This is true if we can invoke TfHash()() on a T instance.
template <class T>
constexpr bool
VtIsHashable() {
    return Vt_HashDetail::_IsHashable<T>(0);
}

/// Compute a hash code for \p val by invoking TfHash()(val) or when not
/// possible issue a coding error and return 0.
template <class T>
size_t VtHashValue(T const &val)
{
    return Vt_HashDetail::_HashValueImpl(val, 0);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_HASH_H
