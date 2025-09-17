//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_TS_TEST_SAMPLE_BEZIER_H
#define PXR_BASE_TS_TS_TEST_SAMPLE_BEZIER_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/tsTest_SplineData.h"
#include "pxr/base/ts/tsTest_Types.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Produces (time, value) samples along a Bezier curve by walking the 't'
// parameter space.  The samples are evenly divided among the segments, and then
// uniformly in the 't' parameter for each segment.  Samples do not necessarily
// always go forward in time; Bezier segments may form loops that temporarily
// reverse direction.
//
// Only Bezier segments are supported.  No extrapolation is performed.
//
TS_API
TsTest_SampleVec
TsTest_SampleBezier(
    const TsTest_SplineData &splineData,
    int numSamples);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
