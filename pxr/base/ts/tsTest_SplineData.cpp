//
// Copyright 2023 Pixar
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

TsTest_SplineData::Knot::Knot() = default;

TsTest_SplineData::Knot::Knot(
    const Knot &other) = default;

TsTest_SplineData::Knot&
TsTest_SplineData::Knot::operator=(
    const Knot &other) = default;

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

TsTest_SplineData::InnerLoopParams::InnerLoopParams() = default;

TsTest_SplineData::InnerLoopParams::InnerLoopParams(
    const InnerLoopParams &other) = default;

TsTest_SplineData::InnerLoopParams&
TsTest_SplineData::InnerLoopParams::operator=(
    const InnerLoopParams &other) = default;

bool TsTest_SplineData::InnerLoopParams::operator==(
    const InnerLoopParams &other) const
{
    return enabled == other.enabled
        && protoStart == other.protoStart
        && protoEnd == other.protoEnd
        && preLoopStart == other.preLoopStart
        && postLoopEnd == other.postLoopEnd
        && closedEnd == other.closedEnd
        && valueOffset == other.valueOffset;
}

bool TsTest_SplineData::InnerLoopParams::operator!=(
    const InnerLoopParams &other) const
{
    return !(*this == other);
}

bool TsTest_SplineData::InnerLoopParams::IsValid() const
{
    if (!enabled) return true;

    if (protoEnd <= protoStart) return false;
    if (preLoopStart > protoStart) return false;
    if (postLoopEnd < protoEnd) return false;

    return true;
}

TsTest_SplineData::Extrapolation::Extrapolation() = default;

TsTest_SplineData::Extrapolation::Extrapolation(
    const TsTest_SplineData::ExtrapMethod methodIn)
    : method(methodIn) {}

TsTest_SplineData::Extrapolation::Extrapolation(
    const Extrapolation &other) = default;

TsTest_SplineData::Extrapolation&
TsTest_SplineData::Extrapolation::operator=(
    const Extrapolation &other) = default;

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

TsTest_SplineData::TsTest_SplineData() = default;

TsTest_SplineData::TsTest_SplineData(
    const TsTest_SplineData &other) = default;

TsTest_SplineData&
TsTest_SplineData::operator=(
    const TsTest_SplineData &other) = default;

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
            result |= FeatureDualValuedKnots;

        if (knot.preAuto || knot.postAuto)
            result |= FeatureAutoTangents;
    }

    if (_innerLoopParams.enabled)
        result |= FeatureInnerLoops;

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
        ss << " " << e.slope;
    else if (e.method == TsTest_SplineData::ExtrapLoop)
        ss << " " << TfEnum::GetName(e.loopMode).substr(4);

    return ss.str();
}

std::string
TsTest_SplineData::GetDebugDescription() const
{
    std::ostringstream ss;

    ss << "Spline:" << std::endl
       << "  hermite " << (_isHermite ? "true" : "false") << std::endl
       << "  preExtrap " << _GetExtrapDesc(_preExtrap) << std::endl
       << "  postExtrap " << _GetExtrapDesc(_postExtrap) << std::endl;

    if (_innerLoopParams.enabled)
    {
        ss << "Loop:" << std::endl
           << "  start " << _innerLoopParams.protoStart
           << ", end " << _innerLoopParams.protoEnd
           << ", preStart " << _innerLoopParams.preLoopStart
           << ", postEnd " << _innerLoopParams.postLoopEnd
           << ", closed " << _innerLoopParams.closedEnd
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

            ss << ", auto " << (knot.preAuto ? "true" : "false")
               << " / " << (knot.postAuto ? "true" : "false");
        }

        ss << std::endl;
    }

    return ss.str();
}

PXR_NAMESPACE_CLOSE_SCOPE
