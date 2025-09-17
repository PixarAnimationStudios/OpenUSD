//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_TYPE_HELPERS_H
#define PXR_BASE_TS_TYPE_HELPERS_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/gf/half.h"
#include "pxr/base/tf/type.h"

#include <cmath>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE


// Internal helper to avoid repeated lookups.
//
template <typename T>
TfType Ts_GetType();

// Compile-time type whose value is true only for supported value types.
//
template <typename T>
struct Ts_IsSupportedValueType;

// Default to false.
template <typename T>
struct Ts_IsSupportedValueType : public std::false_type {};

// Specializations for supported value types.
#define _MAKE_CLAUSE(unused, tuple)                                      \
    template <>                                                          \
    TS_API TfType Ts_GetType<TS_SPLINE_VALUE_CPP_TYPE(tuple)>();         \
    template <>                                                          \
    struct Ts_IsSupportedValueType<TS_SPLINE_VALUE_CPP_TYPE(tuple)> :    \
        public std::true_type {};
TF_PP_SEQ_FOR_EACH(_MAKE_CLAUSE, ~, TS_SPLINE_SUPPORTED_VALUE_TYPES)
#undef _MAKE_CLAUSE

// Mapping from Python type names to TfTypes for supported spline value types.
// These strings align with type names used in downstream libraries; we can't
// depend on them directly, so we replicate these few simple, stable type names
// here.
//
TS_API
TfType Ts_GetTypeFromTypeName(const std::string &typeName);

// Opposite of the above.
//
TS_API
std::string Ts_GetTypeNameFromType(TfType valueType);


// GfHalf doesn't have an overload for std::isfinite, so we provide an adapter.
//
template <typename T>
bool Ts_IsFinite(T value);

template <>
TS_API bool Ts_IsFinite(const GfHalf value);

template <typename T>
bool Ts_IsFinite(const T value)
{
    return std::isfinite(value);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
