//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_TS_TEST_TS_EVALUATOR_H
#define PXR_BASE_TS_TS_TEST_TS_EVALUATOR_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/ts/tsTest_Types.h"

#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

class TsTest_SampleTimes;
class TsSpline;
class GfInterval;

// Perform test evaluation using Ts.
//
class TsTest_TsEvaluator
{
public:
    ////////////////////////////////////////////////////////////////////////////
    // EVALUATION

    // Evaluate at specified times.
    TsTest_SampleVec Eval(
        const TsSpline& spline,
        const TsTest_SampleTimes &sampleTimes) const;

    // Produce bulk samples for drawing.  Sample times are determined adaptively
    // and cannot be controlled.
    template <typename SampleData>
    bool Sample(
        const TsSpline& spline,
        const GfInterval& timeInterval,
        double timeScale,
        double valueScale,
        double tolerance,
        SampleData* splineSamples) const;

    ////////////////////////////////////////////////////////////////////////////
    // TEST DATA TRANSFORMATION

    // Produce a copy of spline with inner loops, if any, baked out into
    // ordinary knots.
    /*
    TsSpline BakeInnerLoops(
        const TsSpline &spline) const;
    */
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
