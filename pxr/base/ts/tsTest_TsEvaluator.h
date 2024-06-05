//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_TS_TEST_TS_EVALUATOR_H
#define PXR_BASE_TS_TS_TEST_TS_EVALUATOR_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/tsTest_Evaluator.h"

PXR_NAMESPACE_OPEN_SCOPE

// Perform test evaluation using Ts.
//
class TS_API TsTest_TsEvaluator : public TsTest_Evaluator
{
public:
    TsTest_SampleVec Eval(
        const TsTest_SplineData &splineData,
        const TsTest_SampleTimes &sampleTimes) const override;

    TsTest_SampleVec Sample(
        const TsTest_SplineData &splineData,
        double tolerance) const override;

    TsTest_SplineData BakeInnerLoops(
        const TsTest_SplineData &splineData) const override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
