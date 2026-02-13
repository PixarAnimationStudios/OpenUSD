//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(TsInterpValueBlock, "Value Block");
    TF_ADD_ENUM_NAME(TsInterpHeld, "Held");
    TF_ADD_ENUM_NAME(TsInterpLinear, "Linear");
    TF_ADD_ENUM_NAME(TsInterpCurve, "Curve");

    TF_ADD_ENUM_NAME(TsCurveTypeBezier, "Bezier");
    TF_ADD_ENUM_NAME(TsCurveTypeHermite, "Hermite");

    TF_ADD_ENUM_NAME(TsExtrapValueBlock, "Value Block");
    TF_ADD_ENUM_NAME(TsExtrapHeld, "Held");
    TF_ADD_ENUM_NAME(TsExtrapLinear, "Linear");
    TF_ADD_ENUM_NAME(TsExtrapSloped, "Sloped");
    TF_ADD_ENUM_NAME(TsExtrapLoopRepeat, "Loop Repeat");
    TF_ADD_ENUM_NAME(TsExtrapLoopReset, "Loop Reset");
    TF_ADD_ENUM_NAME(TsExtrapLoopOscillate, "Loop Oscillate");

    TF_ADD_ENUM_NAME(TsAntiRegressionNone, "None");
    TF_ADD_ENUM_NAME(TsAntiRegressionContain, "Contain");
    TF_ADD_ENUM_NAME(TsAntiRegressionKeepRatio, "Keep Ratio");
    TF_ADD_ENUM_NAME(TsAntiRegressionKeepStart, "Keep Start");

    TF_ADD_ENUM_NAME(TsSourcePreExtrap, "Pre Extrapolation");
    TF_ADD_ENUM_NAME(TsSourcePreExtrapLoop, "Pre Extrapolation Loop");
    TF_ADD_ENUM_NAME(TsSourceInnerLoopPreEcho, "Pre Inner Loop");
    TF_ADD_ENUM_NAME(TsSourceInnerLoopProto, "Inner Loop Prototype");
    TF_ADD_ENUM_NAME(TsSourceInnerLoopPostEcho, "Post Inner Loop");
    TF_ADD_ENUM_NAME(TsSourceKnotInterp, "Knot Interpolation");
    TF_ADD_ENUM_NAME(TsSourcePostExtrap, "Post Extrapolation");
    TF_ADD_ENUM_NAME(TsSourcePostExtrapLoop, "Post Extrapolation Loop");

    TF_ADD_ENUM_NAME(TsTangentAlgorithmNone, "None");
    TF_ADD_ENUM_NAME(TsTangentAlgorithmCustom, "Custom");
    TF_ADD_ENUM_NAME(TsTangentAlgorithmAutoEase, "Auto Ease");
}

bool TsLoopParams::operator==(const TsLoopParams &other) const
{
    return
        protoStart == other.protoStart
        && protoEnd == other.protoEnd
        && numPreLoops == other.numPreLoops
        && numPostLoops == other.numPostLoops
        && valueOffset == other.valueOffset;
}

bool TsLoopParams::operator!=(const TsLoopParams &other) const
{
    return !(*this == other);
}

GfInterval TsLoopParams::GetPrototypeInterval() const
{
    return GfInterval(
        protoStart, protoEnd,
        /* minClosed = */ true, /* maxClosed = */ false);
}

GfInterval TsLoopParams::GetLoopedInterval() const
{
    const TsTime protoSpan = protoEnd - protoStart;
    return GfInterval(
        protoStart - numPreLoops * protoSpan,
        protoEnd + numPostLoops * protoSpan);
}

TsExtrapolation::TsExtrapolation() = default;

TsExtrapolation::TsExtrapolation(TsExtrapMode modeIn)
    : mode(modeIn)
{
}

TsExtrapolation::TsExtrapolation(TsExtrapMode modeIn,
                                 double slopeIn)
    : mode(modeIn)
    , slope(slopeIn)
{
}

bool TsExtrapolation::operator==(const TsExtrapolation &other) const
{
    return
        mode == other.mode
        && (mode != TsExtrapSloped || slope == other.slope);
}

bool TsExtrapolation::operator!=(const TsExtrapolation &other) const
{
    return !(*this == other);
}

bool TsExtrapolation::IsLooping() const
{
    return (mode >= TsExtrapLoopRepeat && mode <= TsExtrapLoopOscillate);
}

////////////////////////////////////////////////////////////////////////////////
// TEMPLATE IMPLEMENTATIONS

// Instantiate the supported samples classes
#define TS_SAMPLE_EXPLICIT_INST(unused, tuple)                          \
    template class                                                      \
        TsSplineSamples< TS_SPLINE_VALUE_CPP_TYPE(tuple) >;             \
    template class                                                      \
        TsSplineSamplesWithSources< TS_SPLINE_VALUE_CPP_TYPE(tuple) >;

TF_PP_SEQ_FOR_EACH(TS_SAMPLE_EXPLICIT_INST, ~, TS_SPLINE_SAMPLE_VERTEX_TYPES)
#undef TS_SAMPLE_EXTERN_IMPL


PXR_NAMESPACE_CLOSE_SCOPE
