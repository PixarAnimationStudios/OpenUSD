//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/tsTest_TsEvaluator.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/raii.h"
#include "pxr/base/ts/tsTest_SampleTimes.h"
#include "pxr/base/ts/typeHelpers.h"
#include "pxr/base/gf/interval.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

using STimes = TsTest_SampleTimes;

TsTest_SampleVec
TsTest_TsEvaluator::Eval(
    const TsSpline& spline,
    const STimes &sampleTimes) const
{
    if (spline.GetKnots().empty())
    {
        return {};
    }

    TsTest_SampleVec result;

    for (const STimes::SampleTime &time : sampleTimes.GetTimes())
    {
        double value = 0;
        if (!time.pre)
        {
            spline.Eval(time.time, &value);
        }
        else
        {
            spline.EvalPreValue(time.time, &value);
        }

        result.push_back(TsTest_Sample(time.time, value));
    }

    return result;
}

template <typename SampleData>
bool
TsTest_TsEvaluator::Sample(
    const TsSpline& spline,
    const GfInterval& timeInterval,
    const double timeScale,
    const double valueScale,
    const double tolerance,
    SampleData* splineSamples) const
{
    return spline.Sample(timeInterval,
                         timeScale,
                         valueScale,
                         tolerance,
                         splineSamples);
}

// Explicit templated method instantiation.
template bool
TsTest_TsEvaluator::Sample(
    const TsSpline& spline,
    const GfInterval& timeInterval,
    const double timeScale,
    const double valueScale,
    const double tolerance,
    TsSplineSamples<GfVec2d>* splineSamples) const;

template bool
TsTest_TsEvaluator::Sample(
    const TsSpline& spline,
    const GfInterval& timeInterval,
    const double timeScale,
    const double valueScale,
    const double tolerance,
    TsSplineSamplesWithSources<GfVec2d>* splineSamples) const;

PXR_NAMESPACE_CLOSE_SCOPE
