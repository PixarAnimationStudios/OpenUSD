//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/ts/tangentConversions.h"
#include "pxr/base/ts/valueTypeDispatch.h"

PXR_NAMESPACE_OPEN_SCOPE

template <typename T>
bool Ts_ConvertToStandardHelper(
    TsTime widthIn,
    T slopeOrHeightIn,
    bool convertHeightToSlope,
    bool divideValuesByThree,
    bool negateHeight,
    TsTime* widthOut,
    T* slopeOut)
{
    double width = widthIn;
    T slope = slopeOrHeightIn;

    if (convertHeightToSlope || divideValuesByThree) {
        // We're going to do math on the values that may lose precision. This
        // will involve rounding but do the math at full precision.
        double value = slopeOrHeightIn;
        if (convertHeightToSlope) {
            // Convert to slope before any possible division by 3
            value /= width;
            if (divideValuesByThree) {
                // Only width gets divided by 3 since value has already
                // been converted to a slope.
                width /= 3;
            }
        } else {
            value /= 3;
            width /= 3;
        }

        // Do not overflow which can happen if slopeOrHeight is large and width
        // is small.
        if (value > std::numeric_limits<T>::max()) {
            value = std::numeric_limits<T>::max();
        } else if (value < -std::numeric_limits<T>::max()) {
            // XXX: we should be using std::numeric_limits<T>::lowest() here.
            value = -std::numeric_limits<T>::max();
        }
        slope = value;
    }

    if (negateHeight) {
        slope = -slope;
    }

    if (widthOut) {
        *widthOut = width;
    }

    if (slopeOut) {
        *slopeOut = slope;
    }

    return true;
}

template <typename T>
bool Ts_ConvertFromStandardHelper(
    TsTime widthIn,
    T slopeIn,
    bool convertSlopeToHeight,
    bool multiplyValuesByThree,
    bool negateHeight,
    TsTime* widthOut,
    T* slopeOrHeightOut)
{
    // Do math in double precision regardless of the data type.
    double width = widthIn;
    T slopeOrHeight = slopeIn;
    
    if (convertSlopeToHeight || multiplyValuesByThree) {
        // We're going to do math on the values that may lose precision. This
        // will involve rounding but do the math at full precision.
        double soh = slopeOrHeight;
        if (convertSlopeToHeight) {
            if (multiplyValuesByThree) {
                width *= 3;
            }

            soh *= width;
        } else {
            // Just multiply by 3
            soh *= 3;
            width *= 3;
        }

        // Do not overflow which can happen if slope and width are large,
        // especially when multiplying by 3.
        if (soh > std::numeric_limits<T>::max()) {
            soh = std::numeric_limits<T>::max();
        } else if (soh < -std::numeric_limits<T>::max()) {
            // XXX: we should be using std::numeric_limits<T>::lowest() here.
            soh = -std::numeric_limits<T>::max();
        }
        
        slopeOrHeight = soh;
    }

    if (negateHeight) {
        slopeOrHeight = -slopeOrHeight;
    }

    if (widthOut) {
        *widthOut = width;
    }

    if (slopeOrHeightOut) {
        *slopeOrHeightOut = slopeOrHeight;
    }

    return true;
}

// Instantiate versions we want in the library
template
TS_API
bool Ts_ConvertToStandardHelper(
    TsTime widthIn,
    double slopeOrHeightIn,
    bool convertHeightToSlope,
    bool divideValuesByThree,
    bool negateHeight,
    TsTime* widthOut,
    double* slopeOut);

template
TS_API
bool Ts_ConvertToStandardHelper(
    TsTime widthIn,
    float slopeOrHeightIn,
    bool convertHeightToSlope,
    bool divideValuesByThree,
    bool negateHeight,
    TsTime* widthOut,
    float* slopeOut);

template
TS_API
bool Ts_ConvertToStandardHelper(
    TsTime widthIn,
    GfHalf slopeOrHeightIn,
    bool convertHeightToSlope,
    bool divideValuesByThree,
    bool negateHeight,
    TsTime* widthOut,
    GfHalf* slopeOut);

template
TS_API
bool Ts_ConvertFromStandardHelper(
    TsTime widthIn,
    double slopeIn,
    bool convertSlopeToHeight,
    bool multiplyValuesByThree,
    bool negateHeight,
    TsTime* widthOut,
    double* slopeOrHeightOut);

template
TS_API
bool Ts_ConvertFromStandardHelper(
    TsTime widthIn,
    float slopeIn,
    bool convertSlopeToHeight,
    bool multiplyValuesByThree,
    bool negateHeight,
    TsTime* widthOut,
    float* slopeOrHeightOut);

template
TS_API
bool Ts_ConvertFromStandardHelper(
    TsTime widthIn,
    GfHalf slopeIn,
    bool convertSlopeToHeight,
    bool multiplyValuesByThree,
    bool negateHeight,
    TsTime* widthOut,
    GfHalf* slopeOrHeightOut);


namespace { // anonymous namespace

template <typename T>
struct _VtConvertToStandardHelper {
    void operator()(
        TsTime widthIn,
        VtValue slopeOrHeightIn,
        bool convertHeightToSlope,
        bool divideValuesByThree,
        bool negateHeight,
        TsTime* widthOut,
        VtValue* slopeOut,
        bool *ok)
    {
        T soh = slopeOrHeightIn.UncheckedGet<T>();
        T so;
        *ok = TsConvertToStandardTangent(
            widthIn, soh, convertHeightToSlope, divideValuesByThree,
            negateHeight, widthOut, &so);
        if (*ok) {
            if (slopeOut) {
                *slopeOut = so;
            }
        }
    }
};

template <typename T>
struct _VtConvertFromStandardHelper {
    void operator()(
        TsTime widthIn,
        VtValue slopeIn,
        bool convertSlopeToHeight,
        bool multiplyValuesByThree,
        bool negateHeight,
        TsTime* widthOut,
        VtValue* slopeOrHeightOut,
        bool *ok)
    {
        T slope = slopeIn.UncheckedGet<T>();
        T soh;
        *ok = TsConvertFromStandardTangent(
            widthIn, slope, convertSlopeToHeight, multiplyValuesByThree,
            negateHeight, widthOut, &soh);
        if (*ok) {
            if (slopeOrHeightOut) {
                *slopeOrHeightOut = soh;
            }
        }
    }
};

} // end anonymous namespace

// VtValue versions of the routines.
bool TsConvertToStandardTangent(
    TsTime widthIn,
    const VtValue& slopeOrHeightIn,
    bool convertHeightToSlope,
    bool divideValuesByThree,
    bool negateHeight,
    TsTime* widthOut,
    VtValue* slopeOut)
{
    bool ok = true;

    TsDispatchToValueTypeTemplate<_VtConvertToStandardHelper>(
        slopeOrHeightIn.GetType(),
        widthIn, slopeOrHeightIn, convertHeightToSlope, divideValuesByThree,
        negateHeight, widthOut, slopeOut, &ok);

    return ok;
}

bool TsConvertFromStandardTangent(
    TsTime widthIn,
    const VtValue& slopeIn,
    bool convertSlopeToHeight,
    bool multiplyValuesByThree,
    bool negateHeight,
    TsTime* widthOut,
    VtValue* slopeOrHeightOut)
{

    bool ok = true;

    TsDispatchToValueTypeTemplate<_VtConvertFromStandardHelper>(
        slopeIn.GetType(),
        widthIn, slopeIn, convertSlopeToHeight, multiplyValuesByThree,
        negateHeight, widthOut, slopeOrHeightOut, &ok);

    return ok;
}

PXR_NAMESPACE_CLOSE_SCOPE

