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
#include "pxr/base/ts/tsTest_SplineData.h"
#include "pxr/base/ts/tsTest_SampleTimes.h"
#include "pxr/base/ts/typeHelpers.h"
#include "pxr/base/gf/interval.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

using SData = TsTest_SplineData;
using STimes = TsTest_SampleTimes;

static TsExtrapolation _MakeExtrap(
    const SData::Extrapolation extrapIn)
{
    switch (extrapIn.method)
    {
        case SData::ExtrapHeld:
            return TsExtrapolation(TsExtrapHeld);

        case SData::ExtrapLinear:
            return TsExtrapolation(TsExtrapLinear);

        case SData::ExtrapSloped:
        {
            TsExtrapolation result(TsExtrapSloped);
            result.slope = extrapIn.slope;
            return result;
        }

        case SData::ExtrapLoop:
            switch (extrapIn.loopMode)
            {
                case SData::LoopRepeat:
                    return TsExtrapolation(TsExtrapLoopRepeat);

                case SData::LoopReset:
                    return TsExtrapolation(TsExtrapLoopReset);

                case SData::LoopOscillate:
                    return TsExtrapolation(TsExtrapLoopOscillate);

                default:
                    TF_CODING_ERROR("Unexpected extrapolating loop mode");
                    return TsExtrapolation();
            }

        default:
            TF_CODING_ERROR("Unexpected extrapolation mode");
            return TsExtrapolation();
    }
}

TsSpline TsTest_TsEvaluator::SplineDataToSpline(
    const SData &data) const
{
    const SData::Features features = data.GetRequiredFeatures();
    if ((features & SData::FeatureHermiteSegments)
        || (features & SData::FeatureAutoTangents))
    {
        TF_CODING_ERROR("Unsupported spline features");
        return TsSpline(Ts_GetType<double>());
    }

    // Don't de-regress.  If the SplineData is regressive, the Spline should be
    // too.
    TsAntiRegressionAuthoringSelector selector(TsAntiRegressionNone);

    TsSpline spline(Ts_GetType<double>());

    spline.SetPreExtrapolation(_MakeExtrap(data.GetPreExtrapolation()));
    spline.SetPostExtrapolation(_MakeExtrap(data.GetPostExtrapolation()));

    const SData::KnotSet &dataKnots = data.GetKnots();

    for (const SData::Knot &dataKnot : dataKnots)
    {
        TsDoubleKnot knot;
        knot.SetTime(dataKnot.time);
        knot.SetValue(dataKnot.value);

        knot.SetPreTanWidth(dataKnot.preLen);
        knot.SetPreTanSlope(dataKnot.preSlope);
        knot.SetPostTanWidth(dataKnot.postLen);
        knot.SetPostTanSlope(dataKnot.postSlope);

        switch (dataKnot.nextSegInterpMethod)
        {
            case SData::InterpHeld:
                knot.SetNextInterpolation(TsInterpHeld); break;
            case SData::InterpLinear:
                knot.SetNextInterpolation(TsInterpLinear); break;
            case SData::InterpCurve:
                knot.SetNextInterpolation(TsInterpCurve); break;
            default: TF_CODING_ERROR("Unexpected interpolation method");
        }

        if (dataKnot.isDualValued)
        {
            knot.SetPreValue(dataKnot.preValue);
        }

        spline.SetKnot(knot);
    }

    const SData::InnerLoopParams &loop = data.GetInnerLoopParams();
    if (loop.enabled)
    {
        TsLoopParams lp;
        lp.protoStart = loop.protoStart;
        lp.protoEnd = loop.protoEnd;
        lp.numPreLoops = loop.numPreLoops;
        lp.numPostLoops = loop.numPostLoops;
        lp.valueOffset = loop.valueOffset;

        spline.SetInnerLoopParams(lp);
    }

    return spline;
}

static SData::Extrapolation _MakeExtrap(
    const TsExtrapolation extrapIn)
{
    switch (extrapIn.mode)
    {
        case TsExtrapHeld:
            return SData::Extrapolation(SData::ExtrapHeld);

        case TsExtrapLinear:
            return SData::Extrapolation(SData::ExtrapLinear);

        case TsExtrapSloped:
        {
            SData::Extrapolation result(SData::ExtrapSloped);
            result.slope = extrapIn.slope;
            return result;
        }

        case TsExtrapLoopRepeat:
        {
            SData::Extrapolation result(SData::ExtrapLoop);
            result.loopMode = SData::LoopRepeat;
            return result;
        }

        case TsExtrapLoopReset:
        {
            SData::Extrapolation result(SData::ExtrapLoop);
            result.loopMode = SData::LoopReset;
            return result;
        }

        case TsExtrapLoopOscillate:
        {
            SData::Extrapolation result(SData::ExtrapLoop);
            result.loopMode = SData::LoopOscillate;
            return result;
        }

        default:
            TF_CODING_ERROR("Unexpected extrapolation mode");
            return SData::Extrapolation();
    }
}

TsTest_SplineData
TsTest_TsEvaluator::SplineToSplineData(
    const TsSpline &splineIn) const
{
    if (splineIn.GetValueType() != Ts_GetType<double>())
    {
        TF_CODING_ERROR("TsEvaluator: only double-valued splines supported");
        return SData();
    }

    SData result;

    // Convert extrapolation.
    result.SetPreExtrapolation(_MakeExtrap(splineIn.GetPreExtrapolation()));
    result.SetPostExtrapolation(_MakeExtrap(splineIn.GetPostExtrapolation()));

    // Convert loop params.
    if (splineIn.HasInnerLoops())
    {
        const TsLoopParams lp = splineIn.GetInnerLoopParams();

        SData::InnerLoopParams dataLp;
        dataLp.protoStart = lp.protoStart;
        dataLp.protoEnd = lp.protoEnd;
        dataLp.numPreLoops = lp.numPreLoops;
        dataLp.numPostLoops = lp.numPostLoops;
        dataLp.valueOffset = lp.valueOffset;

        result.SetInnerLoopParams(dataLp);
    }

    // Convert knots.
    for (const TsKnot &knot : splineIn.GetKnots())
    {
        SData::Knot dataKnot;
        dataKnot.time = knot.GetTime();
        knot.GetValue(&dataKnot.value);
        dataKnot.preLen = knot.GetPreTanWidth();
        knot.GetPreTanSlope(&dataKnot.preSlope);
        dataKnot.postLen = knot.GetPostTanWidth();
        knot.GetPostTanSlope(&dataKnot.postSlope);

        switch (knot.GetNextInterpolation())
        {
            case TsInterpHeld:
                dataKnot.nextSegInterpMethod = SData::InterpHeld; break;
            case TsInterpLinear:
                dataKnot.nextSegInterpMethod = SData::InterpLinear; break;
            case TsInterpCurve:
                dataKnot.nextSegInterpMethod = SData::InterpCurve; break;
            default: TF_CODING_ERROR("Unexpected knot type");
        }

        if (knot.IsDualValued())
        {
            dataKnot.isDualValued = true;
            knot.GetPreValue(&dataKnot.preValue);
        }

        result.AddKnot(dataKnot);
    }

    return result;
}

TsTest_SampleVec
TsTest_TsEvaluator::Eval(
    const SData &splineData,
    const STimes &sampleTimes) const
{
    const TsSpline spline = SplineDataToSpline(splineData);
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

PXR_NAMESPACE_CLOSE_SCOPE
