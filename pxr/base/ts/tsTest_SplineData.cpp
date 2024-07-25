//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/tsTest_SplineData.h"

#include "pxr/base/gf/interval.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/registryManager.h"

#include <algorithm>
#include <sstream>
#include <iomanip>

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfEnum)
{
    TF_ADD_ENUM_NAME(TsTest_SplineData::InterpHeld);
    TF_ADD_ENUM_NAME(TsTest_SplineData::InterpLinear);
    TF_ADD_ENUM_NAME(TsTest_SplineData::InterpCurve);

    TF_ADD_ENUM_NAME(TsTest_SplineData::ExtrapHeld);
    TF_ADD_ENUM_NAME(TsTest_SplineData::ExtrapLinear);
    TF_ADD_ENUM_NAME(TsTest_SplineData::ExtrapSloped);
    TF_ADD_ENUM_NAME(TsTest_SplineData::ExtrapLoop);

    TF_ADD_ENUM_NAME(TsTest_SplineData::LoopNone);
    TF_ADD_ENUM_NAME(TsTest_SplineData::LoopContinue);
    TF_ADD_ENUM_NAME(TsTest_SplineData::LoopRepeat);
    TF_ADD_ENUM_NAME(TsTest_SplineData::LoopReset);
    TF_ADD_ENUM_NAME(TsTest_SplineData::LoopOscillate);

    TF_ADD_ENUM_NAME(TsTest_SplineData::FeatureHeldSegments);
    TF_ADD_ENUM_NAME(TsTest_SplineData::FeatureLinearSegments);
    TF_ADD_ENUM_NAME(TsTest_SplineData::FeatureBezierSegments);
    TF_ADD_ENUM_NAME(TsTest_SplineData::FeatureHermiteSegments);
    TF_ADD_ENUM_NAME(TsTest_SplineData::FeatureDualValuedKnots);
    TF_ADD_ENUM_NAME(TsTest_SplineData::FeatureInnerLoops);
    TF_ADD_ENUM_NAME(TsTest_SplineData::FeatureExtrapolatingLoops);
}

////////////////////////////////////////////////////////////////////////////////

bool TsTest_SplineData::Knot::operator==(
    const Knot &other) const
{
    return time == other.time
        && nextSegInterpMethod == other.nextSegInterpMethod
        && value == other.value
        && isDualValued == other.isDualValued
        && preValue == other.preValue
        && preSlope == other.preSlope
        && postSlope == other.postSlope
        && preLen == other.preLen
        && postLen == other.postLen
        && preAuto == other.preAuto
        && postAuto == other.postAuto;
}

bool TsTest_SplineData::Knot::operator!=(
    const Knot &other) const
{
    return !(*this == other);
}

bool TsTest_SplineData::Knot::operator<(
    const Knot &other) const
{
    return time < other.time;
}

bool TsTest_SplineData::InnerLoopParams::operator==(
    const InnerLoopParams &other) const
{
    return enabled == other.enabled
        && protoStart == other.protoStart
        && protoEnd == other.protoEnd
        && numPreLoops == other.numPreLoops
        && numPostLoops == other.numPostLoops
        && valueOffset == other.valueOffset;
}

bool TsTest_SplineData::InnerLoopParams::operator!=(
    const InnerLoopParams &other) const
{
    return !(*this == other);
}

bool TsTest_SplineData::InnerLoopParams::IsValid() const
{
    return (
        enabled
        && protoEnd > protoStart
        && numPreLoops >= 0
        && numPostLoops >= 0);
}

TsTest_SplineData::Extrapolation::Extrapolation() = default;

TsTest_SplineData::Extrapolation::Extrapolation(
    const TsTest_SplineData::ExtrapMethod methodIn)
    : method(methodIn) {}

bool TsTest_SplineData::Extrapolation::operator==(
    const Extrapolation &other) const
{
    return method == other.method
        && (method != ExtrapSloped
            || slope == other.slope)
        && (method != ExtrapLoop
            || loopMode == other.loopMode);
}

bool TsTest_SplineData::Extrapolation::operator!=(
    const Extrapolation &other) const
{
    return !(*this == other);
}

////////////////////////////////////////////////////////////////////////////////

bool TsTest_SplineData::operator==(
    const TsTest_SplineData &other) const
{
    return _isHermite == other._isHermite
        && _knots == other._knots
        && _preExtrap == other._preExtrap
        && _postExtrap == other._postExtrap
        && _innerLoopParams == other._innerLoopParams;
}

bool TsTest_SplineData::operator!=(
    const TsTest_SplineData &other) const
{
    return !(*this == other);
}

void TsTest_SplineData::SetIsHermite(
    const bool hermite)
{
    _isHermite = hermite;
}

void TsTest_SplineData::AddKnot(
    const Knot &knot)
{
    _knots.erase(knot);
    _knots.insert(knot);
}

void TsTest_SplineData::SetKnots(
    const KnotSet &knots)
{
    _knots = knots;
}

void TsTest_SplineData::SetPreExtrapolation(
    const Extrapolation &preExtrap)
{
    _preExtrap = preExtrap;
}

void TsTest_SplineData::SetPostExtrapolation(
    const Extrapolation &postExtrap)
{
    _postExtrap = postExtrap;
}

void TsTest_SplineData::SetInnerLoopParams(
    const InnerLoopParams &params)
{
    _innerLoopParams = params;
}

bool TsTest_SplineData::GetIsHermite() const
{
    return _isHermite;
}

const TsTest_SplineData::KnotSet&
TsTest_SplineData::GetKnots() const
{
    return _knots;
}

const TsTest_SplineData::Extrapolation&
TsTest_SplineData::GetPreExtrapolation() const
{
    return _preExtrap;
}

const TsTest_SplineData::Extrapolation&
TsTest_SplineData::GetPostExtrapolation() const
{
    return _postExtrap;
}

const TsTest_SplineData::InnerLoopParams&
TsTest_SplineData::GetInnerLoopParams() const
{
    return _innerLoopParams;
}

TsTest_SplineData::Features
TsTest_SplineData::GetRequiredFeatures() const
{
    Features result = 0;

    for (const Knot &knot : _knots)
    {
        switch (knot.nextSegInterpMethod)
        {
            case InterpHeld: result |= FeatureHeldSegments; break;
            case InterpLinear: result |= FeatureLinearSegments; break;
            case InterpCurve:
                result |= (_isHermite ?
                    FeatureHermiteSegments : FeatureBezierSegments); break;
        }

        if (knot.isDualValued)
        {
            result |= FeatureDualValuedKnots;
        }

        if (knot.preAuto || knot.postAuto)
        {
            result |= FeatureAutoTangents;
        }
    }

    if (_innerLoopParams.enabled)
    {
        result |= FeatureInnerLoops;
    }

    if (_preExtrap.method == ExtrapSloped
        || _postExtrap.method == ExtrapSloped)
    {
        result |= FeatureExtrapolatingSlopes;
    }

    if (_preExtrap.method == ExtrapLoop
        || _postExtrap.method == ExtrapLoop)
    {
        result |= FeatureExtrapolatingLoops;
    }

    return result;
}

static std::string _GetExtrapDesc(
    const TsTest_SplineData::Extrapolation &e)
{
    std::ostringstream ss;

    ss << TfEnum::GetName(e.method).substr(6);

    if (e.method == TsTest_SplineData::ExtrapSloped)
    {
        ss << " " << e.slope;
    }
    else if (e.method == TsTest_SplineData::ExtrapLoop)
    {
        ss << " " << TfEnum::GetName(e.loopMode).substr(4);
    }

    return ss.str();
}

std::string
TsTest_SplineData::GetDebugDescription(
    int precision) const
{
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(precision);
    ss << std::boolalpha;

    ss << "Spline:" << std::endl
       << "  hermite " << _isHermite << std::endl
       << "  preExtrap " << _GetExtrapDesc(_preExtrap) << std::endl
       << "  postExtrap " << _GetExtrapDesc(_postExtrap) << std::endl;

    if (_innerLoopParams.enabled)
    {
        ss << "Loop:" << std::endl
           << "  start " << _innerLoopParams.protoStart
           << ", end " << _innerLoopParams.protoEnd
           << ", numPreLoops " << _innerLoopParams.numPreLoops
           << ", numPostLoops " << _innerLoopParams.numPostLoops
           << ", offset " << _innerLoopParams.valueOffset
           << std::endl;
    }

    ss << "Knots:" << std::endl;
    for (const Knot &knot : _knots)
    {
        ss << "  " << knot.time << ": "
           << knot.value
           << ", " << TfEnum::GetName(knot.nextSegInterpMethod).substr(6);

        if (knot.nextSegInterpMethod == InterpCurve)
        {
            ss << ", preSlope " << knot.preSlope
               << ", postSlope " << knot.postSlope;

            if (!_isHermite)
            {
                ss << ", preLen " << knot.preLen
                   << ", postLen " << knot.postLen;
            }

            ss << ", auto " << knot.preAuto
               << " / " << knot.postAuto;
        }

        ss << std::endl;
    }

    return ss.str();
}

PXR_NAMESPACE_CLOSE_SCOPE
