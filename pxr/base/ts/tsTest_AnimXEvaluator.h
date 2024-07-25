//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_TS_TEST_ANIM_X_EVALUATOR_H
#define PXR_BASE_TS_TS_TEST_ANIM_X_EVALUATOR_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/tsTest_Types.h"

PXR_NAMESPACE_OPEN_SCOPE

class TsTest_SplineData;
class TsTest_SampleTimes;

class TsTest_AnimXEvaluator
{
public:
    enum AutoTanType
    {
        AutoTanAuto,
        AutoTanSmooth
    };

    TS_API
    TsTest_AnimXEvaluator(
        AutoTanType autoTanType = AutoTanAuto);

    TS_API
    TsTest_SampleVec Eval(
        const TsTest_SplineData &splineData,
        const TsTest_SampleTimes &sampleTimes) const;

private:
    const AutoTanType _autoTanType;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
