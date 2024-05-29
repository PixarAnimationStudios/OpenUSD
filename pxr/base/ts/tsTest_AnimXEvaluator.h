//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_EXTRAS_BASE_TS_TEST_ANIM_X
#define PXR_EXTRAS_BASE_TS_TEST_ANIM_X

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/tsTest_Evaluator.h"

PXR_NAMESPACE_OPEN_SCOPE

class TS_API TsTest_AnimXEvaluator : public TsTest_Evaluator
{
public:
    enum AutoTanType
    {
        AutoTanAuto,
        AutoTanSmooth
    };

    TsTest_AnimXEvaluator(
        AutoTanType autoTanType = AutoTanAuto);

    TsTest_SampleVec Eval(
        const TsTest_SplineData &splineData,
        const TsTest_SampleTimes &sampleTimes) const override;

private:
    const AutoTanType _autoTanType;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
