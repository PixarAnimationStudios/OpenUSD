//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_GF_TRAITS_H
#define PXR_BASE_GF_TRAITS_H

#include "pxr/pxr.h"

#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

/// A metafunction with a static const bool member 'value' that is true for
/// GfVec types, like GfVec2i, GfVec4d, etc and false for all other types.
template <class T>
struct GfIsGfVec { static const bool value = false; };

/// A metafunction with a static const bool member 'value' that is true for
/// GfMatrix types, like GfMatrix3d, GfMatrix4f, etc and false for all other
/// types.
template <class T>
struct GfIsGfMatrix { static const bool value = false; };

/// A metafunction with a static const bool member 'value' that is true for
/// GfQuat types and false for all other types.
template <class T>
struct GfIsGfQuat { static const bool value = false; };

/// A metafunction with a static const bool member 'value' that is true for
/// GfDualQuat types and false for all other types.
template <class T>
struct GfIsGfDualQuat { static const bool value = false; };

/// A metafunction with a static const bool member 'value' that is true for
/// GfRange types and false for all other types.
template <class T>
struct GfIsGfRange { static const bool value = false; };

/// A metafunction which is equivalent to std::is_floating_point but
/// allows for additional specialization for types like GfHalf
template <class T>
struct GfIsFloatingPoint : public std::is_floating_point<T>{};

/// A metafunction which is equivalent to std::arithmetic but
/// also includes any specializations from GfIsFloatingPoint (like GfHalf)
template <class T>
struct GfIsArithmetic : public std::integral_constant<
    bool, GfIsFloatingPoint<T>::value || std::is_arithmetic<T>::value>{};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_GF_TRAITS_H
