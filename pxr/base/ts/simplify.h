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

#ifndef PXR_BASE_TS_SIMPLIFY_H
#define PXR_BASE_TS_SIMPLIFY_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/gf/multiInterval.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Remove as many knots as possible from spline without introducing
/// error greater than maxErrorFraction, where maxErrorFraction is a
/// percentage of the spline's total range (if the spline's value varies
/// over a range of x, the largest error allowed will be x*maxErrorFraction).
/// Only remove knots in intervals.
TS_API
void TsSimplifySpline(
    TsSpline* spline,
    const GfMultiInterval &intervals,
    double maxErrorFraction,
    double extremeMaxErrFract = .001);

/// Run TsSimplifySpline() on a vector of splines in parallel.  The splines
/// in 'splines' are mutated in place.  The first two args must have the same
/// length, unless the intervals arg is empty, in which case the full frame
/// range of each spline is used.  The remaining args are as in
/// TsSimplifySpline.
TS_API
void TsSimplifySplinesInParallel(
    const std::vector<TsSpline *> &splines,
    const std::vector<GfMultiInterval>& intervals,
    double maxErrorFraction,
    double extremeMaxErrFract = .001);

/// First densely samples the spline within the given intervals
/// by adding one knot per frame. Then executes the simplify algorithm
/// to remove as many knots as possibe while keeping the error below
/// the given maximum.
TS_API
void TsResampleSpline(
    TsSpline* spline,
    const GfMultiInterval &intervals,
    double maxErrorFraction);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TS_SIMPLIFY_H
