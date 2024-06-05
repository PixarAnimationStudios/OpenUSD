//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
