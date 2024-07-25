//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/tsTest_Museum.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE

using SData = TsTest_SplineData;

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(TsTest_Museum::TwoKnotBezier);
    TF_ADD_ENUM_NAME(TsTest_Museum::TwoKnotLinear);
    TF_ADD_ENUM_NAME(TsTest_Museum::FourKnotBezier);
    TF_ADD_ENUM_NAME(TsTest_Museum::SimpleInnerLoop);
    TF_ADD_ENUM_NAME(TsTest_Museum::InnerLoop2and2);
    TF_ADD_ENUM_NAME(TsTest_Museum::InnerLoopPre);
    TF_ADD_ENUM_NAME(TsTest_Museum::InnerLoopPost);
    TF_ADD_ENUM_NAME(TsTest_Museum::ExtrapLoopRepeat);
    TF_ADD_ENUM_NAME(TsTest_Museum::ExtrapLoopReset);
    TF_ADD_ENUM_NAME(TsTest_Museum::ExtrapLoopOscillate);
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


static TsTest_SplineData _TwoKnotBezier()
{
    SData::Knot knot1;
    knot1.time = 1.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 1.0;
    knot1.postSlope = 1.0;
    knot1.postLen = 0.5;

    SData::Knot knot2;
    knot2.time = 5.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 2.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 0.5;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _TwoKnotLinear()
{
    SData::Knot knot1;
    knot1.time = 1.0;
    knot1.nextSegInterpMethod = SData::InterpLinear;
    knot1.value = 1.0;

    SData::Knot knot2;
    knot2.time = 5.0;
    knot2.nextSegInterpMethod = SData::InterpLinear;
    knot2.value = 2.0;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _FourKnotBezier()
{
    SData::Knot knot1;
    knot1.time = 1.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 1.0;
    knot1.postSlope = -0.25;
    knot1.postLen = 0.25;

    SData::Knot knot2;
    knot2.time = 2.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 2.0;
    knot2.preSlope = 0.25;
    knot2.preLen = 0.25;
    knot2.postSlope = 0.25;
    knot2.postLen = 0.25;

    SData::Knot knot3;
    knot3.time = 3.0;
    knot3.nextSegInterpMethod = SData::InterpCurve;
    knot3.value = 1.0;
    knot3.preSlope = -0.25;
    knot3.preLen = 0.25;
    knot3.postSlope = -0.25;
    knot3.postLen = 0.25;

    SData::Knot knot4;
    knot4.time = 4.0;
    knot4.nextSegInterpMethod = SData::InterpCurve;
    knot4.value = 2.0;
    knot4.preSlope = 0.25;
    knot4.preLen = 0.25;

    SData data;
    data.SetKnots({knot1, knot2, knot3, knot4});
    return data;
}

static TsTest_SplineData _SimpleInnerLoop()
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

    SData::Knot knot1;
    knot1.time = 112.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 8.8;
    knot1.postSlope = 15.0;
    knot1.postLen = 0.9;

    SData::Knot knot2;
    knot2.time = 137.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 0.0;
    knot2.preSlope = -5.3;
    knot2.postSlope = -5.3;
    knot2.preLen = 1.3;
    knot2.postLen = 1.8;

    SData::Knot knot3;
    knot3.time = 145.0;
    knot3.nextSegInterpMethod = SData::InterpCurve;
    knot3.value = 8.5;
    knot3.preSlope = 12.5;
    knot3.postSlope = 12.5;
    knot3.preLen = 1.0;
    knot3.postLen = 1.2;

    SData::Knot knot4;
    knot4.time = 155.0;
    knot4.nextSegInterpMethod = SData::InterpCurve;
    knot4.value = 20.2;
    knot4.preSlope = -15.7;
    knot4.postSlope = -15.7;
    knot4.preLen = 0.7;
    knot4.postLen = 0.8;

    SData::Knot knot5;
    knot5.time = 181.0;
    knot5.nextSegInterpMethod = SData::InterpCurve;
    knot5.value = 38.2;
    knot5.preSlope = -9.0;
    knot5.preLen = 2.0;

    SData::InnerLoopParams lp;
    lp.enabled = true;
    lp.protoStart = 137.0;
    lp.protoEnd = 155.0;
    lp.numPreLoops = 1;
    lp.numPostLoops = 1;
    lp.valueOffset = 20.2;

    SData data;
    data.SetKnots({knot1, knot2, knot3, knot4, knot5});
    data.SetInnerLoopParams(lp);
    return data;
}

static TsTest_SplineData _InnerLoop2and2()
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

    SData::Knot knot1;
    knot1.time = 100.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 20.0;
    knot1.preSlope = 2.0;
    knot1.postSlope = 2.0;
    knot1.preLen = 2.0;
    knot1.postLen = 2.0;

    SData::Knot knot2;
    knot2.time = 105.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 10.0;
    knot2.preSlope = 1.5;
    knot2.postSlope = 1.5;
    knot2.preLen = 2.5;
    knot2.postLen = 2.5;

    SData::InnerLoopParams lp;
    lp.enabled = true;
    lp.protoStart = 100.0;
    lp.protoEnd = 110.0;
    lp.numPreLoops = 2;
    lp.numPostLoops = 2;
    lp.valueOffset = -5.0;

    SData data;
    data.SetKnots({knot1, knot2});
    data.SetInnerLoopParams(lp);
    return data;
}

static TsTest_SplineData _InnerLoopPre()
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

    SData::Knot knot1;
    knot1.time = 70.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 8.8;
    knot1.postSlope = -1.0;
    knot1.postLen = 2.2;

    SData::Knot knot2;
    knot2.time = 85.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 35.0;
    knot2.preSlope = -5.3;
    knot2.postSlope = -5.3;
    knot2.preLen = 1.3;
    knot2.postLen = 1.8;

    SData::Knot knot3;
    knot3.time = 100.0;
    knot3.nextSegInterpMethod = SData::InterpCurve;
    knot3.value = 20.0;
    knot3.preSlope = 2.0;
    knot3.postSlope = 2.0;
    knot3.preLen = 2.0;
    knot3.postLen = 2.0;

    SData::Knot knot4;
    knot4.time = 105.0;
    knot4.nextSegInterpMethod = SData::InterpCurve;
    knot4.value = 10.0;
    knot4.preSlope = 1.5;
    knot4.postSlope = 1.5;
    knot4.preLen = 2.5;
    knot4.postLen = 2.5;

    SData::Knot knot5;
    knot5.time = 120.0;
    knot5.nextSegInterpMethod = SData::InterpCurve;
    knot5.value = 15.0;
    knot5.preSlope = -4.0;
    knot5.preLen = 3.0;

    SData::InnerLoopParams lp;
    lp.enabled = true;
    lp.protoStart = 100.0;
    lp.protoEnd = 110.0;
    lp.numPreLoops = 2;
    lp.numPostLoops = 0;
    lp.valueOffset = -5.0;

    SData data;
    data.SetKnots({knot1, knot2, knot3, knot4, knot5});
    data.SetInnerLoopParams(lp);
    return data;
}

static TsTest_SplineData _InnerLoopPost()
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

    SData::Knot knot1;
    knot1.time = 90.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 8.8;
    knot1.postSlope = -1.0;
    knot1.postLen = 2.2;

    SData::Knot knot2;
    knot2.time = 100.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 20.0;
    knot2.preSlope = 2.0;
    knot2.postSlope = 2.0;
    knot2.preLen = 2.0;
    knot2.postLen = 2.0;

    SData::Knot knot3;
    knot3.time = 105.0;
    knot3.nextSegInterpMethod = SData::InterpCurve;
    knot3.value = 10.0;
    knot3.preSlope = 1.5;
    knot3.postSlope = 1.5;
    knot3.preLen = 2.5;
    knot3.postLen = 2.5;

    SData::Knot knot4;
    knot4.time = 125.0;
    knot4.nextSegInterpMethod = SData::InterpCurve;
    knot4.value = 35.0;
    knot4.preSlope = -5.3;
    knot4.postSlope = -5.3;
    knot4.preLen = 1.3;
    knot4.postLen = 1.8;

    SData::Knot knot5;
    knot5.time = 140.0;
    knot5.nextSegInterpMethod = SData::InterpCurve;
    knot5.value = 15.0;
    knot5.preSlope = -4.0;
    knot5.preLen = 3.0;

    SData::InnerLoopParams lp;
    lp.enabled = true;
    lp.protoStart = 100.0;
    lp.protoEnd = 110.0;
    lp.numPreLoops = 0;
    lp.numPostLoops = 2;
    lp.valueOffset = -5.0;

    SData data;
    data.SetKnots({knot1, knot2, knot3, knot4, knot5});
    data.SetInnerLoopParams(lp);
    return data;
}

static TsTest_SplineData _ExtrapLoopRepeat()
{
    SData::Knot knot1;
    knot1.time = 100.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 10.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 3.0;

    SData::Knot knot2;
    knot2.time = 105.0;
    knot2.nextSegInterpMethod = SData::InterpLinear;
    knot2.value = 20.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 3.0;

    SData::Knot knot3;
    knot3.time = 110.0;
    knot3.nextSegInterpMethod = SData::InterpHeld;
    knot3.value = 15.0;

    SData data;
    data.SetKnots({knot1, knot2, knot3});
    SData::Extrapolation extrap(SData::ExtrapLoop);
    extrap.loopMode = SData::LoopRepeat;
    data.SetPreExtrapolation(extrap);
    data.SetPostExtrapolation(extrap);
    return data;
}

static TsTest_SplineData _ExtrapLoopReset()
{
    SData::Knot knot1;
    knot1.time = 100.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 10.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 3.0;

    SData::Knot knot2;
    knot2.time = 105.0;
    knot2.nextSegInterpMethod = SData::InterpLinear;
    knot2.value = 20.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 3.0;

    SData::Knot knot3;
    knot3.time = 110.0;
    knot3.nextSegInterpMethod = SData::InterpHeld;
    knot3.value = 15.0;

    SData data;
    data.SetKnots({knot1, knot2, knot3});
    SData::Extrapolation extrap(SData::ExtrapLoop);
    extrap.loopMode = SData::LoopReset;
    data.SetPreExtrapolation(extrap);
    data.SetPostExtrapolation(extrap);
    return data;
}

static TsTest_SplineData _ExtrapLoopOscillate()
{
    SData::Knot knot1;
    knot1.time = 100.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 10.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 3.0;

    SData::Knot knot2;
    knot2.time = 105.0;
    knot2.nextSegInterpMethod = SData::InterpLinear;
    knot2.value = 20.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 3.0;

    SData::Knot knot3;
    knot3.time = 110.0;
    knot3.nextSegInterpMethod = SData::InterpHeld;
    knot3.value = 15.0;

    SData data;
    data.SetKnots({knot1, knot2, knot3});
    SData::Extrapolation extrap(SData::ExtrapLoop);
    extrap.loopMode = SData::LoopOscillate;
    data.SetPreExtrapolation(extrap);
    data.SetPostExtrapolation(extrap);
    return data;
}

static TsTest_SplineData _InnerAndExtrapLoops()
{
    // Same knots and inner loop params as InnerLoop2and2

    SData::Knot knot1;
    knot1.time = 100.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 20.0;
    knot1.preSlope = 2.0;
    knot1.postSlope = 2.0;
    knot1.preLen = 2.0;
    knot1.postLen = 2.0;

    SData::Knot knot2;
    knot2.time = 105.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 10.0;
    knot2.preSlope = 1.5;
    knot2.postSlope = 1.5;
    knot2.preLen = 2.5;
    knot2.postLen = 2.5;

    SData::InnerLoopParams lp;
    lp.enabled = true;
    lp.protoStart = 100.0;
    lp.protoEnd = 110.0;
    lp.numPreLoops = 2;
    lp.numPostLoops = 2;
    lp.valueOffset = -5.0;

    SData data;
    data.SetKnots({knot1, knot2});
    data.SetInnerLoopParams(lp);
    SData::Extrapolation extrap(SData::ExtrapLoop);
    extrap.loopMode = SData::LoopRepeat;
    data.SetPreExtrapolation(extrap);
    extrap.loopMode = SData::LoopOscillate;
    data.SetPostExtrapolation(extrap);
    return data;
}

static TsTest_SplineData _RegressiveLoop()
{
    SData::Knot knot1;
    knot1.time = 156.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.preSlope = -1.3;
    knot1.postSlope = -1.3;
    knot1.preLen = 6.2;
    knot1.postLen = 15.8;

    SData::Knot knot2;
    knot2.time = 167.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 28.8;
    knot2.preSlope = 2.4;
    knot2.postSlope = 2.4;
    knot2.preLen = 21.7;
    knot2.postLen = 5.5;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _RegressiveS()
{
    SData::Knot knot1;
    knot1.time = 156.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = -1.3;
    knot1.postLen = 15.8;

    SData::Knot knot2;
    knot2.time = 167.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 28.8;
    knot2.preSlope = 0.4;
    knot2.preLen = 16.8;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _RegressiveSStandard()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 1.2;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 1.2;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _RegressiveSPreOut()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.5;
    knot1.postLen = 1.0;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 3.0;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _RegressiveSPostOut()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 3.0;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.5;
    knot2.preLen = 1.0;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _RegressiveSBothOut()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 4.0;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 4.0;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _RegressivePreJ()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 2.5;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.6;
    knot2.preLen = 2.5;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _RegressivePostJ()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.6;
    knot1.postLen = 2.5;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 2.5;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _RegressivePreC()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 0.0;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 2.0;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _RegressivePostC()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 2.0;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 0.0;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _RegressivePreG()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 2.0;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.57;
    knot2.preLen = 3.5;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _RegressivePostG()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.57;
    knot1.postLen = 3.5;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 2.0;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _RegressivePreFringe()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 0.05;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 1.3;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _RegressivePostFringe()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 1.3;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 0.05;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _BoldS()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 1.25;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 0.5;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _Cusp()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.5;
    knot1.postLen = 1.0;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 0.0;
    knot2.preSlope = -0.5;
    knot2.preLen = 1.0;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _CenterVertical()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 1.0;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 1.0;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _NearCenterVertical()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 0.8;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 0.8;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _VerticalTorture()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.5;
    knot1.postLen = 0.44092698519760592513;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 1.3227809555928178309;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _FourThirdOneThird()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 4.0 / 3.0;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 1.0 / 3.0;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _OneThirdFourThird()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 1.0 / 3.0;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 4.0 / 3.0;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _StartVert()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 0.0;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 1.0;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _EndVert()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 1.0;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 0.0;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _FringeVert()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.0;
    knot1.postLen = (2.0 + GfSqrt(3.0)) / 3.0;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.0;
    knot2.preLen = (2.0 - GfSqrt(3.0)) / 3.0;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _MarginalN()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 1e12;
    knot1.postLen = 5e-12;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 1e12;
    knot2.preLen = 5e-12;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _ZeroTans()
{
    SData::Knot knot1;
    knot1.time = 0.0;
    knot1.nextSegInterpMethod = SData::InterpCurve;
    knot1.value = 0.0;
    knot1.postSlope = 0.0;
    knot1.postLen = 0.0;

    SData::Knot knot2;
    knot2.time = 1.0;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 1.0;
    knot2.preSlope = 0.0;
    knot2.preLen = 0.0;

    SData data;
    data.SetKnots({knot1, knot2});
    return data;
}

static TsTest_SplineData _ComplexParams()
{
    SData data;

    data.SetPreExtrapolation(SData::Extrapolation(SData::ExtrapLinear));
    SData::Extrapolation postExtrap(SData::ExtrapSloped);
    postExtrap.slope = 0.57;
    data.SetPostExtrapolation(postExtrap);

    SData::InnerLoopParams lp;
    lp.protoStart = 15;
    lp.protoEnd = 25;
    lp.numPreLoops = 1;
    lp.numPostLoops = 2;
    lp.valueOffset = 11.7;
    data.SetInnerLoopParams(lp);

    SData::Knot knot1;
    knot1.time = 7;
    knot1.nextSegInterpMethod = SData::InterpHeld;
    knot1.isDualValued = true;
    knot1.preValue = 5.5;
    knot1.value = 7.21;

    SData::Knot knot2;
    knot2.time = 15;
    knot2.nextSegInterpMethod = SData::InterpCurve;
    knot2.value = 8.18;
    knot2.postSlope = 1.17;
    knot2.postLen = 2.49;

    SData::Knot knot3;
    knot3.time = 20;
    knot3.nextSegInterpMethod = SData::InterpCurve;
    knot3.value = 14.72;
    knot3.preSlope = -1.4;
    knot3.preLen = 3.77;
    knot3.postSlope = -1.4;
    knot3.postLen = 1.1;

    data.SetKnots({knot1, knot2, knot3});

    return data;
}

TsTest_SplineData
TsTest_Museum::GetData(const DataId id)
{
    switch (id)
    {
        case TwoKnotBezier: return _TwoKnotBezier();
        case TwoKnotLinear: return _TwoKnotLinear();
        case FourKnotBezier: return _FourKnotBezier();
        case SimpleInnerLoop: return _SimpleInnerLoop();
        case InnerLoop2and2: return _InnerLoop2and2();
        case InnerLoopPre: return _InnerLoopPre();
        case InnerLoopPost: return _InnerLoopPost();
        case ExtrapLoopRepeat: return _ExtrapLoopRepeat();
        case ExtrapLoopReset: return _ExtrapLoopReset();
        case ExtrapLoopOscillate: return _ExtrapLoopOscillate();
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

std::vector<std::string>
TsTest_Museum::GetAllNames()
{
    return TfEnum::GetAllNames<DataId>();
}

TsTest_SplineData
TsTest_Museum::GetDataByName(const std::string &name)
{
    bool found = false;
    const DataId id = TfEnum::GetValueFromName<DataId>(name, &found);
    if (!found)
    {
        TF_CODING_ERROR("No Museum exhibit named '%s'", name.c_str());
        return TsTest_SplineData();
    }

    return GetData(id);
}


PXR_NAMESPACE_CLOSE_SCOPE
