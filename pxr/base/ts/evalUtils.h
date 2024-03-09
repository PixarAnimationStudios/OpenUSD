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

#ifndef PXR_BASE_TS_EVAL_UTILS_H
#define PXR_BASE_TS_EVAL_UTILS_H

#include "pxr/pxr.h"
#include "pxr/base/ts/keyFrame.h"
#include "pxr/base/ts/keyFrameMap.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/types.h"

PXR_NAMESPACE_OPEN_SCOPE

enum Ts_EvalType {
    Ts_EvalValue,
    Ts_EvalDerivative
};

// Evaluate either the value or derivative at a given time on a given side.
VtValue Ts_Eval(
    const TsSpline &val,
    TsTime time, TsSide side,
    Ts_EvalType evalType);

// Return piecewise linear samples for a value between two times to within
// a given tolerance.
TsSamples Ts_Sample( const TsSpline & val,
    TsTime startTime, TsTime endTime,
    double timeScale, double valueScale,
    double tolerance );

// Return the minimum and maximum values of a value over an interval.
std::pair<VtValue, VtValue> Ts_GetRange( const TsSpline & val,
                                           TsTime startTime,
                                           TsTime endTime );

// k has exactly three key frames.  The first and last define a segment
// of a spline and the middle is where we want a breakdown.  This modifies
// tangents on the three key frames to keep the shape of the spline the
// same (as best it can).  We assume that the middle key frame's value has
// already been set correctly.
void Ts_Breakdown( TsKeyFrameMap* k );

TsExtrapolationType Ts_GetEffectiveExtrapolationType(
    const TsKeyFrame& kf,
    const TsExtrapolationPair &extrapolation,
    bool kfIsOnlyKeyFrame, 
    TsSide side);

TsExtrapolationType Ts_GetEffectiveExtrapolationType(
    const TsKeyFrame& kf,
    const TsSpline &spline,
    TsSide side);

// Returns true if the segment between the given (adjacent) key 
// frames is monotonic (i.e. no extremes).
bool Ts_IsSegmentValueMonotonic( const TsKeyFrame &kf1,
    const TsKeyFrame &kf2 );

PXR_NAMESPACE_CLOSE_SCOPE

#endif
