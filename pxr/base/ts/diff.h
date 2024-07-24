//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_DIFF_H
#define PXR_BASE_TS_DIFF_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/types.h"

PXR_NAMESPACE_OPEN_SCOPE

class TsSpline;

/// Returns the interval in which the splines \p s1 and \p s2 will
/// evaluate to different values or in which knots in the splines have
/// different values.
///
/// In particular, if the rightmost changed knot is a dual-valued knot
/// where the left value has changed and the right value is unchanged,
/// the returned interval will be closed on the right, even though the
/// value of the spline *at* the rightmost time does not change.
TS_API
GfInterval
TsFindChangedInterval(
    const TsSpline &s1,
    const TsSpline &s2);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
