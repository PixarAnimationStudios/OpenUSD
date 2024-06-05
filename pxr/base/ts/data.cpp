//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/data.h"

#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE

static const double Ts_slopeDiffMax = 1.0e-4;

template <>
void
Ts_TypedData<float>::ResetTangentSymmetryBroken()
{
    if (_knotType == TsKnotBezier) {
        float slopeDiff = fabs(_GetLeftTangentSlope() -
            _GetRightTangentSlope());
        if (slopeDiff >= Ts_slopeDiffMax) {
            SetTangentSymmetryBroken(true);
        }
    }
}

template <>
void
Ts_TypedData<double>::ResetTangentSymmetryBroken()
{
    if (_knotType == TsKnotBezier) {
        double slopeDiff =  fabs(_GetLeftTangentSlope() -
            _GetRightTangentSlope());
        if (slopeDiff >= Ts_slopeDiffMax) {
            SetTangentSymmetryBroken(true);
        }
    }
}

template <>
bool
Ts_TypedData<float>::ValueCanBeInterpolated() const
{
    return std::isfinite(_GetRightValue()) && 
        (!_isDual || std::isfinite(_GetLeftValue()));
}

template <>
bool
Ts_TypedData<double>::ValueCanBeInterpolated() const
{
    return std::isfinite(_GetRightValue()) && 
        (!_isDual || std::isfinite(_GetLeftValue()));
}

template class Ts_TypedData<float>;
template class Ts_TypedData<double>;

PXR_NAMESPACE_CLOSE_SCOPE
