//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_TANGENT_CONVERSION_H
#define PXR_BASE_TS_TANGENT_CONVERSION_H

#include "pxr/pxr.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_OPEN_SCOPE

template <typename T>
bool TsConvertToStandardTangent(
    TsTime widthIn,
    T slopeOrHeightIn,
    bool convertHeightToSlope,
    bool divideValuesByThree,
    bool negateHeight,
    TsTime* widthOut,
    T* slopeOut);

TS_API
bool TsConvertToStandardTangent(
    TsTime widthIn,
    const VtValue& slopeOrHeightIn,
    bool convertHeightToSlope,
    bool divideValuesByThree,
    bool negateHeight,
    TsTime* widthOut,
    VtValue* slopeOut);

template <typename T>
bool TsConvertFromStandardTangent(
    TsTime widthIn,
    T slopeIn,
    bool convertSlopeToHeight,
    bool multiplyValuesByThree,
    bool negateHeight,
    TsTime* widthOut,
    T* slopeOrHeightOut);

TS_API
bool TsConvertFromStandardTangent(
    TsTime widthIn,
    const VtValue& slopeIn,
    bool convertSlopeToHeight,
    bool multiplyValuesByThree,
    bool negateHeight,
    TsTime* widthOut,
    VtValue* slopeOrHeightOut);

////////////////////////////////////////////////////////////////////////////////
// TEMPLATE HELPERS
template <typename T>
bool Ts_ConvertToStandardHelper(
    const TsTime widthIn,
    const T slopeOrHeightIn,
    bool convertHeightToSlope,
    bool divideValuesByThree,
    bool negateHeight,
    TsTime* widthOut,
    T* slopeOut);

template <typename T>
bool Ts_ConvertFromStandardHelper(
    TsTime widthIn,
    T slopeIn,
    bool convertSlopeToHeight,
    bool multiplyValuesByThree,
    bool negateHeight,
    TsTime* widthOut,
    T* slopeOrHeightOut);

////////////////////////////////////////////////////////////////////////////////
// TEMPLATE DEFINITIONS

#define _MAKE_CLAUSE(unused, tuple)                                 \
    std::is_same_v<NonVolatileT, TS_SPLINE_VALUE_CPP_TYPE(tuple)> ||

template <typename T>
bool TsConvertToStandardTangent(
    const TsTime widthIn,
    const T slopeOrHeightIn,
    bool convertHeightToSlope,
    bool divideValuesByThree,
    bool negateHeight,
    TsTime* widthOut,
    T* slopeOut)
{
    using NonVolatileT = typename std::remove_volatile<T>::type;

    static_assert((
        TF_PP_SEQ_FOR_EACH(_MAKE_CLAUSE, ~, TS_SPLINE_SUPPORTED_VALUE_TYPES) \
            false), "Can only use the values supported by the spline system.");

    return Ts_ConvertToStandardHelper(
        widthIn, slopeOrHeightIn, convertHeightToSlope, divideValuesByThree,
        negateHeight, widthOut, slopeOut);
}

template <typename T>
bool TsConvertFromStandardTangent(
    const TsTime widthIn,
    const T slopeIn,
    bool convertSlopeToHeight,
    bool multiplyValuesByThree,
    bool negateHeight,
    TsTime* widthOut,
    T* slopeOrHeightOut)
{
    using NonVolatileT = typename std::remove_volatile<T>::type;

    static_assert((
        TF_PP_SEQ_FOR_EACH(_MAKE_CLAUSE, ~, TS_SPLINE_SUPPORTED_VALUE_TYPES) \
            false), "Can only use the values supported by the spline system.");

    return Ts_ConvertFromStandardHelper(
        widthIn, slopeIn, convertSlopeToHeight, multiplyValuesByThree,
        negateHeight, widthOut, slopeOrHeightOut);
}

#undef _MAKE_CLAUSE

PXR_NAMESPACE_CLOSE_SCOPE

#endif
