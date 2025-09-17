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

// Some of the test values are outside the range of a finite half value.
// Safely make finite values out of them. Note that any non-zero value
// will yield a non-zero half value.
GfHalf MakeHalf(double v)
{
    if (v == 0) {
        return 0;
    } else if (std::abs(v) < std::numeric_limits<GfHalf>::min()) {
        v = std::copysign(double(std::numeric_limits<GfHalf>::min()), v);
    } else if (std::abs(v) > std::numeric_limits<GfHalf>::max()) {
        v = std::copysign(double(std::numeric_limits<GfHalf>::max()), v);
    }

    return GfHalf(v);
}
    
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
    return SplineDataToSpline(data, TfType::Find<double>());
}

TsSpline TsTest_TsEvaluator::SplineDataToSpline(
    const SData &data,
    const TfType& valueType) const
{
    if (!TsSpline::IsSupportedValueType(valueType)) {
        TF_CODING_ERROR("Unsupported spline value type: '%s'",
                        valueType.GetTypeName().c_str());
        return TsSpline();
    }

    const SData::Features features = data.GetRequiredFeatures();
    if (features & SData::FeatureAutoTangents)
    {
        TF_CODING_ERROR("Unsupported spline features");
        return TsSpline(valueType);
    }

    // Don't de-regress.  If the SplineData is regressive, the Spline should be
    // too.
    TsAntiRegressionAuthoringSelector selector(TsAntiRegressionNone);

    TsSpline spline(valueType);
    if (data.GetIsHermite()) {
        spline.SetCurveType(TsCurveTypeHermite);
    }

    spline.SetPreExtrapolation(_MakeExtrap(data.GetPreExtrapolation()));
    spline.SetPostExtrapolation(_MakeExtrap(data.GetPostExtrapolation()));

    const SData::KnotSet &dataKnots = data.GetKnots();

    for (const SData::Knot &dataKnot : dataKnots)
    {
        TsKnot knot(valueType);
        knot.SetTime(dataKnot.time);

        knot.SetPreTanWidth(dataKnot.preLen);
        knot.SetPostTanWidth(dataKnot.postLen);

        if (valueType == Ts_GetType<double>()) {
            knot.SetValue(double(dataKnot.value));
            if (dataKnot.isDualValued)
            {
                knot.SetPreValue(double(dataKnot.preValue));
            }

            knot.SetPreTanSlope(double(dataKnot.preSlope));
            knot.SetPostTanSlope(double(dataKnot.postSlope));

        } else if (valueType == Ts_GetType<float>()) {
            knot.SetValue(float(dataKnot.value));
            if (dataKnot.isDualValued)
            {
                knot.SetPreValue(float(dataKnot.preValue));
            }

            knot.SetPreTanSlope(float(dataKnot.preSlope));
            knot.SetPostTanSlope(float(dataKnot.postSlope));

        } else if (valueType == Ts_GetType<GfHalf>()) {
            knot.SetValue(MakeHalf(dataKnot.value));
            if (dataKnot.isDualValued)
            {
                knot.SetPreValue(MakeHalf(dataKnot.preValue));
            }

            // Adjust tangents while maintaining general magnitude even if the
            // slope is changed by conversion.
            //
            // This is for one particular spline in the Museum that sets almost
            // vertical tangents. The slope is 1e+12 and the width is 1e-12, so
            // the tangent vector (1e-12, 1.0), or almost exactly (0, 1). When
            // the slope is mapped into a GfHalf, it becomes 65504.0. If the
            // width is not similarly changed, the the tangent would become the
            // vector (1e-12, 6.5504e-8) or almost exactly (0, 0), which changes
            // the shape of the curve significantly. This math computes the
            // tangent vector to be (1.5266e-5, 1.0) which is as close to
            // vertical as we can get with a GfHalf slope.
            //
            // We could apply this same math to float values above, but the range
            // of float extendes to 1e+/-38 so it hasn't been an issue.
            double preHeight = dataKnot.preSlope * dataKnot.preLen;
            GfHalf preSlope = MakeHalf(dataKnot.preSlope);
            GfHalf preWidth = MakeHalf(preHeight / preSlope);
            knot.SetPreTanSlope(preSlope);
            knot.SetPreTanWidth(preWidth);

            double postHeight = dataKnot.postSlope * dataKnot.postLen;
            GfHalf postSlope = MakeHalf(dataKnot.postSlope);
            GfHalf postWidth = MakeHalf(postHeight / postSlope);
            knot.SetPostTanSlope(postSlope);
            knot.SetPostTanWidth(postWidth);
        } else {
            TF_CODING_ERROR("Unimplemented spline value type: '%s'",
                            valueType.GetTypeName().c_str());
            return TsSpline(valueType);
        }
                            
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
        VtValue value;
        dataKnot.time = knot.GetTime();
        knot.GetValue(&value);
        dataKnot.value = value.Cast<double>().Get<double>();
        dataKnot.preLen = knot.GetPreTanWidth();
        knot.GetPreTanSlope(&value);
        dataKnot.preSlope = value.Cast<double>().Get<double>();
        dataKnot.postLen = knot.GetPostTanWidth();
        knot.GetPostTanSlope(&dataKnot.postSlope);
        dataKnot.preSlope = value.Cast<double>().Get<double>();

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
            knot.GetPreValue(&value);
            dataKnot.preValue = value.Cast<double>().Get<double>();
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

template <typename SampleData>
bool
TsTest_TsEvaluator::Sample(
    const SData &splineData,
    const GfInterval& timeInterval,
    const double timeScale,
    const double valueScale,
    const double tolerance,
    SampleData* splineSamples) const
{
    const TsSpline spline = SplineDataToSpline(splineData);

    return spline.Sample(timeInterval,
                         timeScale,
                         valueScale,
                         tolerance,
                         splineSamples);
}

// Explicit templated method instantiation.
template TS_API bool
TsTest_TsEvaluator::Sample(
    const SData &splineData,
    const GfInterval& timeInterval,
    const double timeScale,
    const double valueScale,
    const double tolerance,
    TsSplineSamples<GfVec2d>* splineSamples) const;

template TS_API bool
TsTest_TsEvaluator::Sample(
    const SData &splineData,
    const GfInterval& timeInterval,
    const double timeScale,
    const double valueScale,
    const double tolerance,
    TsSplineSamplesWithSources<GfVec2d>* splineSamples) const;

PXR_NAMESPACE_CLOSE_SCOPE
