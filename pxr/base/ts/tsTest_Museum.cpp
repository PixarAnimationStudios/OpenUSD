//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/knotData.h"
#include "pxr/base/ts/raii.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/splineData.h"
#include "pxr/base/ts/tsTest_Museum.h"
#include "pxr/base/ts/types.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

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
} // anonymous namespace

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(TsTest_Museum::TwoKnotBezier);
    TF_ADD_ENUM_NAME(TsTest_Museum::TwoKnotBezierAutoEase);
    TF_ADD_ENUM_NAME(TsTest_Museum::TwoKnotHermite);
    TF_ADD_ENUM_NAME(TsTest_Museum::TwoKnotLinear);
    TF_ADD_ENUM_NAME(TsTest_Museum::FourKnotBezier);
    TF_ADD_ENUM_NAME(TsTest_Museum::FourKnotHermite);
    TF_ADD_ENUM_NAME(TsTest_Museum::SimpleInnerLoop);
    TF_ADD_ENUM_NAME(TsTest_Museum::InnerLoop2and2);
    TF_ADD_ENUM_NAME(TsTest_Museum::InnerLoopPre);
    TF_ADD_ENUM_NAME(TsTest_Museum::InnerLoopPost);
    TF_ADD_ENUM_NAME(TsTest_Museum::ExtrapLoopRepeat);
    TF_ADD_ENUM_NAME(TsTest_Museum::ExtrapLoopRepeatDualValued);
    TF_ADD_ENUM_NAME(TsTest_Museum::ExtrapLoopRepeatDualValuedBoundary);
    TF_ADD_ENUM_NAME(TsTest_Museum::ExtrapLoopRepeatBoundary);
    TF_ADD_ENUM_NAME(TsTest_Museum::ExtrapLoopReset);
    TF_ADD_ENUM_NAME(TsTest_Museum::ExtrapLoopResetDualValued);
    TF_ADD_ENUM_NAME(TsTest_Museum::ExtrapLoopResetBoundary);
    TF_ADD_ENUM_NAME(TsTest_Museum::ExtrapLoopResetInvalidBoundary);
    TF_ADD_ENUM_NAME(TsTest_Museum::ExtrapLoopOscillate);
    TF_ADD_ENUM_NAME(TsTest_Museum::ExtrapLoopOscillateBoundary);
    TF_ADD_ENUM_NAME(TsTest_Museum::InnerAndExtrapLoops);
    TF_ADD_ENUM_NAME(TsTest_Museum::RegressiveLoop);
    TF_ADD_ENUM_NAME(TsTest_Museum::RegressiveS);
    TF_ADD_ENUM_NAME(TsTest_Museum::RegressiveSStandard);
    TF_ADD_ENUM_NAME(TsTest_Museum::RegressiveSPreOut);
    TF_ADD_ENUM_NAME(TsTest_Museum::RegressiveSPostOut);
    TF_ADD_ENUM_NAME(TsTest_Museum::RegressiveSBothOut);
    TF_ADD_ENUM_NAME(TsTest_Museum::RegressivePreJ);
    TF_ADD_ENUM_NAME(TsTest_Museum::RegressivePostJ);
    TF_ADD_ENUM_NAME(TsTest_Museum::RegressivePreC);
    TF_ADD_ENUM_NAME(TsTest_Museum::RegressivePostC);
    TF_ADD_ENUM_NAME(TsTest_Museum::RegressivePreG);
    TF_ADD_ENUM_NAME(TsTest_Museum::RegressivePostG);
    TF_ADD_ENUM_NAME(TsTest_Museum::RegressivePreFringe);
    TF_ADD_ENUM_NAME(TsTest_Museum::RegressivePostFringe);
    TF_ADD_ENUM_NAME(TsTest_Museum::BoldS);
    TF_ADD_ENUM_NAME(TsTest_Museum::Cusp);
    TF_ADD_ENUM_NAME(TsTest_Museum::CenterVertical);
    TF_ADD_ENUM_NAME(TsTest_Museum::NearCenterVertical);
    TF_ADD_ENUM_NAME(TsTest_Museum::VerticalTorture);
    TF_ADD_ENUM_NAME(TsTest_Museum::FourThirdOneThird);
    TF_ADD_ENUM_NAME(TsTest_Museum::OneThirdFourThird);
    TF_ADD_ENUM_NAME(TsTest_Museum::StartVert);
    TF_ADD_ENUM_NAME(TsTest_Museum::EndVert);
    TF_ADD_ENUM_NAME(TsTest_Museum::FringeVert);
    TF_ADD_ENUM_NAME(TsTest_Museum::MarginalN);
    TF_ADD_ENUM_NAME(TsTest_Museum::ZeroTans);
    TF_ADD_ENUM_NAME(TsTest_Museum::ComplexParams);
}


static Ts_TypedSplineData<double> _TwoKnotBezier()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 1.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 1.0;
    knot1.postTanSlope = 1.0;
    knot1.postTanWidth = 0.5;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 5.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 2.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 0.5;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _TwoKnotBezierAutoEase()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 1.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 1.0;
    knot1.postTanAlgorithm = TsTangentAlgorithmAutoEase;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 5.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 2.0;
    knot2.preTanAlgorithm = TsTangentAlgorithmAutoEase;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    TsExtrapolation extrap(TsExtrapLoopRepeat);
    data.preExtrapolation = extrap;
    data.postExtrapolation = extrap;
    return data;
}

static Ts_TypedSplineData<double> _TwoKnotHermite()
{
    // Same as TwoKnotBezier but with Hermite interpolation
    Ts_TypedSplineData<double> data = _TwoKnotBezier();
    data.curveType = TsCurveTypeHermite;
    return data;
}

static Ts_TypedSplineData<double> _TwoKnotLinear()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 1.0;
    knot1.nextInterp = TsInterpLinear;
    knot1.value = 1.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 5.0;
    knot2.nextInterp = TsInterpLinear;
    knot2.value = 2.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _FourKnotBezier()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 1.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 1.0;
    knot1.postTanSlope = -0.25;
    knot1.postTanWidth = 0.25;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 2.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 2.0;
    knot2.preTanSlope = 0.25;
    knot2.preTanWidth = 0.25;
    knot2.postTanSlope = 0.25;
    knot2.postTanWidth = 0.25;

    Ts_TypedKnotData<double> knot3;
    knot3.time = 3.0;
    knot3.nextInterp = TsInterpCurve;
    knot3.value = 1.0;
    knot3.preTanSlope = -0.25;
    knot3.preTanWidth = 0.25;
    knot3.postTanSlope = -0.25;
    knot3.postTanWidth = 0.25;

    Ts_TypedKnotData<double> knot4;
    knot4.time = 4.0;
    knot4.nextInterp = TsInterpCurve;
    knot4.value = 2.0;
    knot4.preTanSlope = 0.25;
    knot4.preTanWidth = 0.25;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    data.SetKnot(&knot3, VtDictionary());
    data.SetKnot(&knot4, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _FourKnotHermite()
{
    // Same as FourKnotBezier but with Hermite interpolation
    Ts_TypedSplineData<double> data = _FourKnotBezier();
    data.curveType = TsCurveTypeHermite;
    return data;
}

static Ts_TypedSplineData<double> _SimpleInnerLoop()
{
    // proto len: 18
    // pre-loop len: 18 (1 iteration)
    // post-loop len: 18 (1 iteration)

    // pre-unlooped: 112
    // pre-shadowed: none
    // pre-echo: 119 (from 137), 127 (from 145)
    // proto: 137, 145
    // post-echo: 155 (from 137), 163 (from 145)
    // final echo: 173 (from 137)
    // post-shadowed: 155
    // post-unlooped: 181

    Ts_TypedKnotData<double> knot1;
    knot1.time = 112.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 8.8;
    knot1.postTanSlope = 15.0;
    knot1.postTanWidth = 0.9;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 137.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 0.0;
    knot2.preTanSlope = -5.3;
    knot2.postTanSlope = -5.3;
    knot2.preTanWidth = 1.3;
    knot2.postTanWidth = 1.8;

    Ts_TypedKnotData<double> knot3;
    knot3.time = 145.0;
    knot3.nextInterp = TsInterpCurve;
    knot3.value = 8.5;
    knot3.preTanSlope = 12.5;
    knot3.postTanSlope = 12.5;
    knot3.preTanWidth = 1.0;
    knot3.postTanWidth = 1.2;

    Ts_TypedKnotData<double> knot4;
    knot4.time = 155.0;
    knot4.nextInterp = TsInterpCurve;
    knot4.value = 20.2;
    knot4.preTanSlope = -15.7;
    knot4.postTanSlope = -15.7;
    knot4.preTanWidth = 0.7;
    knot4.postTanWidth = 0.8;

    Ts_TypedKnotData<double> knot5;
    knot5.time = 181.0;
    knot5.nextInterp = TsInterpCurve;
    knot5.value = 38.2;
    knot5.preTanSlope = -9.0;
    knot5.preTanWidth = 2.0;

    TsLoopParams lp;
    lp.protoStart = 137.0;
    lp.protoEnd = 155.0;
    lp.numPreLoops = 1;
    lp.numPostLoops = 1;
    lp.valueOffset = 20.2;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    data.SetKnot(&knot3, VtDictionary());
    data.SetKnot(&knot4, VtDictionary());
    data.SetKnot(&knot5, VtDictionary());
    data.loopParams = lp;
    return data;
}

static Ts_TypedSplineData<double> _InnerLoop2and2()
{
    // proto len: 10
    // pre-loop len: 20 (2 iterations)
    // post-loop len: 20 (2 iterations)

    // pre-unlooped: none
    // pre-shadowed: none
    // pre-echo:
    //   80 (from 100), 85 (from 105)
    //   90 (from 100), 95 (from 105)
    // proto: 100, 105
    // post-echo:
    //   110 (from 100), 115 (from 105)
    //   120 (from 100), 125 (from 105)
    // final echo: 130 (from 100)
    // post-shadowed: none
    // post-unlooped: none

    Ts_TypedKnotData<double> knot1;
    knot1.time = 100.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 20.0;
    knot1.preTanSlope = 2.0;
    knot1.postTanSlope = 2.0;
    knot1.preTanWidth = 2.0;
    knot1.postTanWidth = 2.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 105.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 10.0;
    knot2.preTanSlope = 1.5;
    knot2.postTanSlope = 1.5;
    knot2.preTanWidth = 2.5;
    knot2.postTanWidth = 2.5;

    TsLoopParams lp;
    lp.protoStart = 100.0;
    lp.protoEnd = 110.0;
    lp.numPreLoops = 2;
    lp.numPostLoops = 2;
    lp.valueOffset = -5.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    data.loopParams = lp;
    return data;
}

static Ts_TypedSplineData<double> _InnerLoopPre()
{
    // proto len: 10
    // pre-loop len: 20 (2 iterations)
    // post-loop len: 0 (0 iterations)

    // pre-unlooped: 70
    // pre-shadowed: 85
    // pre-echo:
    //   80 (from 100), 85 (from 105)
    //   90 (from 100), 95 (from 105)
    // proto: 100, 105
    // post-echo: none
    // final echo: 110 (from 100)
    // post-shadowed: none
    // post-unlooped: 120

    Ts_TypedKnotData<double> knot1;
    knot1.time = 70.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 8.8;
    knot1.postTanSlope = -1.0;
    knot1.postTanWidth = 2.2;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 85.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 35.0;
    knot2.preTanSlope = -5.3;
    knot2.postTanSlope = -5.3;
    knot2.preTanWidth = 1.3;
    knot2.postTanWidth = 1.8;

    Ts_TypedKnotData<double> knot3;
    knot3.time = 100.0;
    knot3.nextInterp = TsInterpCurve;
    knot3.value = 20.0;
    knot3.preTanSlope = 2.0;
    knot3.postTanSlope = 2.0;
    knot3.preTanWidth = 2.0;
    knot3.postTanWidth = 2.0;

    Ts_TypedKnotData<double> knot4;
    knot4.time = 105.0;
    knot4.nextInterp = TsInterpCurve;
    knot4.value = 10.0;
    knot4.preTanSlope = 1.5;
    knot4.postTanSlope = 1.5;
    knot4.preTanWidth = 2.5;
    knot4.postTanWidth = 2.5;

    Ts_TypedKnotData<double> knot5;
    knot5.time = 120.0;
    knot5.nextInterp = TsInterpCurve;
    knot5.value = 15.0;
    knot5.preTanSlope = -4.0;
    knot5.preTanWidth = 3.0;

    TsLoopParams lp;
    lp.protoStart = 100.0;
    lp.protoEnd = 110.0;
    lp.numPreLoops = 2;
    lp.numPostLoops = 0;
    lp.valueOffset = -5.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    data.SetKnot(&knot3, VtDictionary());
    data.SetKnot(&knot4, VtDictionary());
    data.SetKnot(&knot5, VtDictionary());
    data.loopParams = lp;
    return data;
}

static Ts_TypedSplineData<double> _InnerLoopPost()
{
    // proto len: 10
    // pre-loop len: 0 (0 iterations)
    // post-loop len: 20 (2 iterations)

    // pre-unlooped: 90
    // pre-shadowed: none
    // pre-echo: none
    // proto: 100, 105
    // post-echo:
    //   110 (from 100), 115 (from 105)
    //   120 (from 100), 125 (from 105)
    // final echo: 130 (from 100)
    // post-shadowed: 125
    // post-unlooped: 140

    Ts_TypedKnotData<double> knot1;
    knot1.time = 90.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 8.8;
    knot1.postTanSlope = -1.0;
    knot1.postTanWidth = 2.2;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 100.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 20.0;
    knot2.preTanSlope = 2.0;
    knot2.postTanSlope = 2.0;
    knot2.preTanWidth = 2.0;
    knot2.postTanWidth = 2.0;

    Ts_TypedKnotData<double> knot3;
    knot3.time = 105.0;
    knot3.nextInterp = TsInterpCurve;
    knot3.value = 10.0;
    knot3.preTanSlope = 1.5;
    knot3.postTanSlope = 1.5;
    knot3.preTanWidth = 2.5;
    knot3.postTanWidth = 2.5;

    Ts_TypedKnotData<double> knot4;
    knot4.time = 125.0;
    knot4.nextInterp = TsInterpCurve;
    knot4.value = 35.0;
    knot4.preTanSlope = -5.3;
    knot4.postTanSlope = -5.3;
    knot4.preTanWidth = 1.3;
    knot4.postTanWidth = 1.8;

    Ts_TypedKnotData<double> knot5;
    knot5.time = 140.0;
    knot5.nextInterp = TsInterpCurve;
    knot5.value = 15.0;
    knot5.preTanSlope = -4.0;
    knot5.preTanWidth = 3.0;

    TsLoopParams lp;
    lp.protoStart = 100.0;
    lp.protoEnd = 110.0;
    lp.numPreLoops = 0;
    lp.numPostLoops = 2;
    lp.valueOffset = -5.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    data.SetKnot(&knot3, VtDictionary());
    data.SetKnot(&knot4, VtDictionary());
    data.SetKnot(&knot5, VtDictionary());
    data.loopParams = lp;
    return data;
}

static Ts_TypedSplineData<double> _ExtrapLoopRepeat()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 100.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 10.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 3.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 105.0;
    knot2.nextInterp = TsInterpLinear;
    knot2.value = 20.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 3.0;

    Ts_TypedKnotData<double> knot3;
    knot3.time = 110.0;
    knot3.nextInterp = TsInterpHeld;
    knot3.value = 15.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    data.SetKnot(&knot3, VtDictionary());
    TsExtrapolation extrap(TsExtrapLoopRepeat);
    data.preExtrapolation = extrap;
    data.postExtrapolation = extrap;
    return data;
}

static Ts_TypedSplineData<double> _ExtrapLoopRepeatDualValued()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 100.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 10.0;
    knot1.preValue = -10.0;
    knot1.dualValued = true;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 3.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 105.0;
    knot2.nextInterp = TsInterpLinear;
    knot2.value = 20.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 3.0;

    Ts_TypedKnotData<double> knot3;
    knot3.time = 110.0;
    knot3.nextInterp = TsInterpHeld;
    knot3.value = 15.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    data.SetKnot(&knot3, VtDictionary());
    TsExtrapolation extrap(TsExtrapLoopRepeat);
    data.preExtrapolation = extrap;
    data.postExtrapolation = extrap;
    return data;
}

static Ts_TypedSplineData<double> _ExtrapLoopRepeatBoundary()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 100.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 10.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 3.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 105.0;
    knot2.nextInterp = TsInterpLinear;
    knot2.value = 20.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 3.0;

    Ts_TypedKnotData<double> knot3;
    knot3.time = 110.0;
    knot3.nextInterp = TsInterpHeld;
    knot3.value = 15.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    data.SetKnot(&knot3, VtDictionary());
    TsExtrapolation extrap(TsExtrapLoopRepeat);
    extrap.loopBoundaryTime = 105.0;
    data.preExtrapolation = extrap;

    extrap.loopBoundaryTime = 100.0;
    data.postExtrapolation = extrap;
    return data;
}

static Ts_TypedSplineData<double> _ExtrapLoopRepeatDualValuedBoundary()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 100.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 10.0;
    knot1.preValue = -10.0;
    knot1.dualValued = true;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 3.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 105.0;
    knot2.nextInterp = TsInterpLinear;
    knot2.value = 20.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 3.0;

    Ts_TypedKnotData<double> knot3;
    knot3.time = 110.0;
    knot3.nextInterp = TsInterpHeld;
    knot3.value = 15.0;
    knot3.preValue = 25.0;
    knot3.dualValued = true;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    data.SetKnot(&knot3, VtDictionary());
    TsExtrapolation extrap(TsExtrapLoopRepeat);
    extrap.loopBoundaryTime = 110.0;
    data.preExtrapolation = extrap;

    extrap.loopBoundaryTime = 105.0;
    data.postExtrapolation = extrap;
    return data;
}

static Ts_TypedSplineData<double> _ExtrapLoopReset()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 100.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 10.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 3.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 105.0;
    knot2.nextInterp = TsInterpLinear;
    knot2.value = 20.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 3.0;

    Ts_TypedKnotData<double> knot3;
    knot3.time = 110.0;
    knot3.nextInterp = TsInterpHeld;
    knot3.value = 15.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    data.SetKnot(&knot3, VtDictionary());
    TsExtrapolation extrap(TsExtrapLoopReset);
    data.preExtrapolation = extrap;
    data.postExtrapolation = extrap;
    return data;
}

static Ts_TypedSplineData<double> _ExtrapLoopResetDualValued()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 100.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 10.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 3.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 105.0;
    knot2.nextInterp = TsInterpLinear;
    knot2.value = 20.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 3.0;

    Ts_TypedKnotData<double> knot3;
    knot3.time = 110.0;
    knot3.nextInterp = TsInterpHeld;
    knot3.value = 20.0;
    knot3.dualValued = true;
    knot3.preValue = 15.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    data.SetKnot(&knot3, VtDictionary());
    TsExtrapolation extrap(TsExtrapLoopReset);
    data.preExtrapolation = extrap;
    data.postExtrapolation = extrap;
    return data;
}

static Ts_TypedSplineData<double> _ExtrapLoopResetBoundary()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 100.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 10.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 3.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 105.0;
    knot2.nextInterp = TsInterpLinear;
    knot2.value = 20.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 3.0;

    Ts_TypedKnotData<double> knot3;
    knot3.time = 110.0;
    knot3.nextInterp = TsInterpHeld;
    knot3.value = 15.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    data.SetKnot(&knot3, VtDictionary());
    TsExtrapolation extrap(TsExtrapLoopReset);
    extrap.loopBoundaryTime = 105.0;
    data.preExtrapolation = extrap;

    extrap.loopBoundaryTime = 105.0;
    data.postExtrapolation = extrap;
    return data;
}

static Ts_TypedSplineData<double> _ExtrapLoopResetInvalidBoundary()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 100.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 10.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 3.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 105.0;
    knot2.nextInterp = TsInterpLinear;
    knot2.value = 20.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 3.0;

    Ts_TypedKnotData<double> knot3;
    knot3.time = 110.0;
    knot3.nextInterp = TsInterpHeld;
    knot3.value = 15.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    data.SetKnot(&knot3, VtDictionary());
    TsExtrapolation extrap(TsExtrapLoopReset);
    extrap.loopBoundaryTime = 99.0;
    data.preExtrapolation = extrap;

    extrap.loopBoundaryTime = 107.0;
    data.postExtrapolation = extrap;
    return data;
}

static Ts_TypedSplineData<double> _ExtrapLoopOscillate()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 100.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 10.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 3.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 105.0;
    knot2.nextInterp = TsInterpLinear;
    knot2.value = 20.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 3.0;

    Ts_TypedKnotData<double> knot3;
    knot3.time = 110.0;
    knot3.nextInterp = TsInterpHeld;
    knot3.value = 15.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    data.SetKnot(&knot3, VtDictionary());
    TsExtrapolation extrap(TsExtrapLoopOscillate);
    data.preExtrapolation = extrap;
    data.postExtrapolation = extrap;
    return data;
}

static Ts_TypedSplineData<double> _ExtrapLoopOscillateBoundary()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 100.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 10.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 3.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 105.0;
    knot2.nextInterp = TsInterpLinear;
    knot2.value = 20.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 3.0;

    Ts_TypedKnotData<double> knot3;
    knot3.time = 110.0;
    knot3.nextInterp = TsInterpHeld;
    knot3.value = 15.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    data.SetKnot(&knot3, VtDictionary());
    TsExtrapolation extrap(TsExtrapLoopOscillate);
    extrap.loopBoundaryTime = 105.0;
    data.preExtrapolation = extrap;

    extrap.loopBoundaryTime = 110.0;
    data.postExtrapolation = extrap;
    return data;
}

static Ts_TypedSplineData<double> _InnerAndExtrapLoops()
{
    // Same knots and inner loop params as InnerLoop2and2

    Ts_TypedKnotData<double> knot1;
    knot1.time = 100.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 20.0;
    knot1.preTanSlope = 2.0;
    knot1.postTanSlope = 2.0;
    knot1.preTanWidth = 2.0;
    knot1.postTanWidth = 2.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 105.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 10.0;
    knot2.preTanSlope = 1.5;
    knot2.postTanSlope = 1.5;
    knot2.preTanWidth = 2.5;
    knot2.postTanWidth = 2.5;

    TsLoopParams lp;
    lp.protoStart = 100.0;
    lp.protoEnd = 110.0;
    lp.numPreLoops = 2;
    lp.numPostLoops = 2;
    lp.valueOffset = -5.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    data.loopParams = lp;
    TsExtrapolation extrap(TsExtrapLoopRepeat);
    data.preExtrapolation = extrap;
    extrap.mode = TsExtrapLoopOscillate;
    data.postExtrapolation = extrap;
    return data;
}

static Ts_TypedSplineData<double> _RegressiveLoop()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 156.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.preTanSlope = -1.3;
    knot1.postTanSlope = -1.3;
    knot1.preTanWidth = 6.2;
    knot1.postTanWidth = 15.8;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 167.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 28.8;
    knot2.preTanSlope = 2.4;
    knot2.postTanSlope = 2.4;
    knot2.preTanWidth = 21.7;
    knot2.postTanWidth = 5.5;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _RegressiveS()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 156.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = -1.3;
    knot1.postTanWidth = 15.8;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 167.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 28.8;
    knot2.preTanSlope = 0.4;
    knot2.preTanWidth = 16.8;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _RegressiveSStandard()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 1.2;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 1.2;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _RegressiveSPreOut()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.5;
    knot1.postTanWidth = 1.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 3.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _RegressiveSPostOut()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 3.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.5;
    knot2.preTanWidth = 1.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _RegressiveSBothOut()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 4.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 4.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _RegressivePreJ()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 2.5;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.6;
    knot2.preTanWidth = 2.5;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _RegressivePostJ()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.6;
    knot1.postTanWidth = 2.5;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 2.5;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _RegressivePreC()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 0.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 2.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _RegressivePostC()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 2.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 0.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _RegressivePreG()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 2.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.57;
    knot2.preTanWidth = 3.5;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _RegressivePostG()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.57;
    knot1.postTanWidth = 3.5;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 2.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _RegressivePreFringe()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 0.05;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 1.3;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _RegressivePostFringe()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 1.3;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 0.05;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _BoldS()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 1.25;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 0.5;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _Cusp()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.5;
    knot1.postTanWidth = 1.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 0.0;
    knot2.preTanSlope = -0.5;
    knot2.preTanWidth = 1.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _CenterVertical()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 1.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 1.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _NearCenterVertical()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 0.8;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 0.8;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _VerticalTorture()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.5;
    knot1.postTanWidth = 0.44092698519760592513;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 1.3227809555928178309;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _FourThirdOneThird()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 4.0 / 3.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 1.0 / 3.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _OneThirdFourThird()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 1.0 / 3.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 4.0 / 3.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _StartVert()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 0.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 1.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _EndVert()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 1.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 0.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _FringeVert()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = (2.0 + GfSqrt(3.0)) / 3.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = (2.0 - GfSqrt(3.0)) / 3.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _MarginalN()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 1e12;
    knot1.postTanWidth = 5e-12;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 1e12;
    knot2.preTanWidth = 5e-12;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _ZeroTans()
{
    Ts_TypedKnotData<double> knot1;
    knot1.time = 0.0;
    knot1.nextInterp = TsInterpCurve;
    knot1.value = 0.0;
    knot1.postTanSlope = 0.0;
    knot1.postTanWidth = 0.0;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 1.0;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 1.0;
    knot2.preTanSlope = 0.0;
    knot2.preTanWidth = 0.0;

    Ts_TypedSplineData<double> data{};
    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    return data;
}

static Ts_TypedSplineData<double> _ComplexParams()
{
    Ts_TypedSplineData<double> data{};

    data.preExtrapolation = TsExtrapolation(TsExtrapLinear);
    TsExtrapolation postExtrap(TsExtrapSloped);
    postExtrap.slope = 0.57;
    data.postExtrapolation = postExtrap;

    // Note: These params were never used in the original
    // implementation of _ComplexParams, so continue to omit
    // them. Enable them as desired.
    // TsInnerLoopParams lp;
    // lp.protoStart = 15;
    // lp.protoEnd = 25;
    // lp.numPreLoops = 1;
    // lp.numPostLoops = 2;
    // lp.valueOffset = 11.7;
    // data.loopParams = lp;

    Ts_TypedKnotData<double> knot1;
    knot1.time = 7;
    knot1.nextInterp = TsInterpHeld;
    knot1.dualValued = true;
    knot1.preValue = 5.5;
    knot1.value = 7.21;

    Ts_TypedKnotData<double> knot2;
    knot2.time = 15;
    knot2.nextInterp = TsInterpCurve;
    knot2.value = 8.18;
    knot2.postTanSlope = 1.17;
    knot2.postTanWidth = 2.49;

    Ts_TypedKnotData<double> knot3;
    knot3.time = 20;
    knot3.nextInterp = TsInterpCurve;
    knot3.value = 14.72;
    knot3.preTanSlope = -1.4;
    knot3.preTanWidth = 3.77;
    knot3.postTanSlope = -1.4;
    knot3.postTanWidth = 1.1;

    data.SetKnot(&knot1, VtDictionary());
    data.SetKnot(&knot2, VtDictionary());
    data.SetKnot(&knot3, VtDictionary());

    return data;
}

Ts_TypedSplineData<double>
TsTest_Museum::_GetData(DataId id)
{
    switch (id)
    {
        case TwoKnotBezier: return _TwoKnotBezier();
        case TwoKnotBezierAutoEase: return _TwoKnotBezierAutoEase();
        case TwoKnotHermite: return _TwoKnotHermite();
        case TwoKnotLinear: return _TwoKnotLinear();
        case FourKnotBezier: return _FourKnotBezier();
        case FourKnotHermite: return _FourKnotHermite();
        case SimpleInnerLoop: return _SimpleInnerLoop();
        case InnerLoop2and2: return _InnerLoop2and2();
        case InnerLoopPre: return _InnerLoopPre();
        case InnerLoopPost: return _InnerLoopPost();
        case ExtrapLoopRepeat: return _ExtrapLoopRepeat();
        case ExtrapLoopRepeatDualValued: return _ExtrapLoopRepeatDualValued();
        case ExtrapLoopRepeatBoundary: return _ExtrapLoopRepeatBoundary();
        case ExtrapLoopRepeatDualValuedBoundary: return _ExtrapLoopRepeatDualValuedBoundary();
        case ExtrapLoopReset: return _ExtrapLoopReset();
        case ExtrapLoopResetDualValued: return _ExtrapLoopResetDualValued();
        case ExtrapLoopResetBoundary: return _ExtrapLoopResetBoundary();
        case ExtrapLoopResetInvalidBoundary: return _ExtrapLoopResetInvalidBoundary();
        case ExtrapLoopOscillate: return _ExtrapLoopOscillate();
        case ExtrapLoopOscillateBoundary: return _ExtrapLoopOscillateBoundary();
        case InnerAndExtrapLoops: return _InnerAndExtrapLoops();
        case RegressiveLoop: return _RegressiveLoop();
        case RegressiveS: return _RegressiveS();
        case RegressiveSStandard: return _RegressiveSStandard();
        case RegressiveSPreOut: return _RegressiveSPreOut();
        case RegressiveSPostOut: return _RegressiveSPostOut();
        case RegressiveSBothOut: return _RegressiveSBothOut();
        case RegressivePreJ: return _RegressivePreJ();
        case RegressivePostJ: return _RegressivePostJ();
        case RegressivePreC: return _RegressivePreC();
        case RegressivePostC: return _RegressivePostC();
        case RegressivePreG: return _RegressivePreG();
        case RegressivePostG: return _RegressivePostG();
        case RegressivePreFringe: return _RegressivePreFringe();
        case RegressivePostFringe: return _RegressivePostFringe();
        case BoldS: return _BoldS();
        case Cusp: return _Cusp();
        case CenterVertical: return _CenterVertical();
        case NearCenterVertical: return _NearCenterVertical();
        case VerticalTorture: return _VerticalTorture();
        case FourThirdOneThird: return _FourThirdOneThird();
        case OneThirdFourThird: return _OneThirdFourThird();
        case StartVert: return _StartVert();
        case EndVert: return _EndVert();
        case FringeVert: return _FringeVert();
        case MarginalN: return _MarginalN();
        case ZeroTans: return _ZeroTans();
        case ComplexParams: return _ComplexParams();
    }

    return {};
}

TsSpline
TsTest_Museum::_SplineDataToSpline(
    const Ts_TypedSplineData<double>& data,
    const TfType valueType)
{
    if (!TsSpline::IsSupportedValueType(valueType)) {
        TF_CODING_ERROR("Unsupported spline value type: '%s'",
                        valueType.GetTypeName().c_str());
        return TsSpline();
    }

    TsSpline spline(valueType);
    spline.SetCurveType(data.curveType);
    spline.SetPreExtrapolation(data.preExtrapolation);
    spline.SetPostExtrapolation(data.postExtrapolation);
    spline.SetInnerLoopParams(data.loopParams);

    // Don't de-regress.  If the SplineData is regressive, the Spline should be
    // too.
    TsAntiRegressionAuthoringSelector selector(TsAntiRegressionNone);

    for (const auto& knotData : data.knots) {
        TsKnot knot(valueType);
        knot.SetTime(knotData.time);
        knot.SetPreTanWidth(knotData.preTanWidth);
        knot.SetPostTanWidth(knotData.postTanWidth);
        knot.SetNextInterpolation(knotData.nextInterp);
        knot.SetPreTanAlgorithm(knotData.preTanAlgorithm);
        knot.SetPostTanAlgorithm(knotData.postTanAlgorithm);

        if (valueType == Ts_GetType<double>()) {
            knot.SetValue(knotData.value);      
            if (knotData.dualValued) {                                                                                                       
                knot.SetPreValue(knotData.preValue);                                                                                         
            }                                                                                                                                
            knot.SetPreTanSlope(knotData.preTanSlope);                                                                                       
            knot.SetPostTanSlope(knotData.postTanSlope);                                                                                     
        } else if (valueType == Ts_GetType<float>()) {
            knot.SetValue(float(knotData.value));
            if (knotData.dualValued) {
                knot.SetPreValue(float(knotData.preValue));
            }
            knot.SetPreTanSlope(float(knotData.preTanSlope));
            knot.SetPostTanSlope(float(knotData.postTanSlope));

        } else if (valueType == Ts_GetType<GfHalf>()) {
            knot.SetValue(MakeHalf(knotData.value));
            if (knotData.dualValued) {
                knot.SetPreValue(MakeHalf(knotData.preValue));
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
            // We could apply this same math to float values above, but the
            // range of float extendes to 1e+/-38 so it hasn't been an issue.
            double preHeight = knotData.preTanSlope * knotData.preTanWidth;
            GfHalf preTanSlope = MakeHalf(knotData.preTanSlope);
            double preWidth = preTanSlope == 0
                              ? knotData.preTanWidth
                              : preHeight / preTanSlope;
            knot.SetPreTanSlope(preTanSlope);
            knot.SetPreTanWidth(preWidth);

            double postHeight = knotData.postTanSlope * knotData.postTanWidth;
            GfHalf postTanSlope = MakeHalf(knotData.postTanSlope);
            double postWidth = postTanSlope == 0
                               ? knotData.postTanWidth
                               : postHeight / postTanSlope;
            knot.SetPostTanSlope(postTanSlope);
            knot.SetPostTanWidth(postWidth);
        } else {
            TF_CODING_ERROR("Unimplemented spline value type: '%s'",
                            valueType.GetTypeName().c_str());
            return TsSpline(valueType);
        }

        spline.SetKnot(knot);
    }

    return spline;
}


TsSpline
TsTest_Museum::GetSpline(const DataId id, const TfType valueType)
{
    const Ts_TypedSplineData<double> data = _GetData(id);
    return _SplineDataToSpline(data, valueType);
}

std::vector<std::string>
TsTest_Museum::GetAllNames()
{
    return TfEnum::GetAllNames<DataId>();
}

TsSpline
TsTest_Museum::GetSplineByName(const std::string &name, const TfType valueType)
{
    bool found = false;
    const DataId id = TfEnum::GetValueFromName<DataId>(name, &found);
    if (!found)
    {
        TF_CODING_ERROR("No Museum exhibit named '%s'", name.c_str());
        return TsSpline();
    }

    return GetSpline(id, valueType);
}


PXR_NAMESPACE_CLOSE_SCOPE
