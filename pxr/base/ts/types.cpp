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


PXR_NAMESPACE_CLOSE_SCOPE
