//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/splineData.h"
#include "pxr/base/ts/raii.h"
#include "pxr/base/ts/regressionPreventer.h"

#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/registryManager.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<TsSpline>();
}


// static
bool TsSpline::IsSupportedValueType(const TfType valueType)
{
#define _CHECK_TYPE(unused, tuple) \
    (valueType == Ts_GetType<TS_SPLINE_VALUE_CPP_TYPE(tuple)>()) ||

    return (TF_PP_SEQ_FOR_EACH(_CHECK_TYPE, ~, TS_SPLINE_SUPPORTED_VALUE_TYPES)
        false);
#undef _CHECK_TYPE
}

////////////////////////////////////////////////////////////////////////////////
// Construction and value semantics

TsSpline::TsSpline() = default;

TsSpline::TsSpline(const TfType valueType)
    : _data(Ts_SplineData::Create(valueType))
{
}

TsSpline::TsSpline(const TsSpline &other)
    : _data(other._data)
{
}

TsSpline& TsSpline::operator=(const TsSpline &other)
{
    _data = other._data;
    return *this;
}

bool TsSpline::operator==(const TsSpline &other) const
{
    // Get data for both sides.
    const Ts_SplineData* const data = _GetData();
    const Ts_SplineData* const otherData = other._GetData();

    // If we're sharing data, we're equal.
    if (data == otherData)
    {
        return true;
    }

    // Compare data.
    return *data == *otherData;
}

bool TsSpline::operator!=(const TsSpline &other) const
{
    return !(*this == other);
}

////////////////////////////////////////////////////////////////////////////////
// Value types

TfType TsSpline::GetValueType() const
{
    return _GetData()->GetValueType();
}

void TsSpline::SetTimeValued(const bool timeValued)
{
    _PrepareForWrite();
    _data->timeValued = timeValued;
}

bool TsSpline::IsTimeValued() const
{
    return _GetData()->timeValued;
}

////////////////////////////////////////////////////////////////////////////////
// Curve types

void TsSpline::SetCurveType(const TsCurveType curveType)
{
    _PrepareForWrite();
    _data->curveType = curveType;
}

TsCurveType TsSpline::GetCurveType() const
{
    return _GetData()->curveType;
}

////////////////////////////////////////////////////////////////////////////////
// Extrapolation

void TsSpline::SetPreExtrapolation(
    const TsExtrapolation &extrap)
{
    _PrepareForWrite();
    _data->preExtrapolation = extrap;
}

TsExtrapolation TsSpline::GetPreExtrapolation() const
{
    return _GetData()->preExtrapolation;
}

void TsSpline::SetPostExtrapolation(
    const TsExtrapolation &extrap)
{
    _PrepareForWrite();
    _data->postExtrapolation = extrap;
}

TsExtrapolation TsSpline::GetPostExtrapolation() const
{
    return _GetData()->postExtrapolation;
}

////////////////////////////////////////////////////////////////////////////////
// Inner Loops

void TsSpline::SetInnerLoopParams(
    const TsLoopParams &params)
{
    _PrepareForWrite();

    // Store a copy.
    _data->loopParams = params;

    // Ignore negative loop counts.
    if (_data->loopParams.numPreLoops < 0)
    {
        _data->loopParams.numPreLoops = 0;
    }
    if (_data->loopParams.numPostLoops < 0)
    {
        _data->loopParams.numPostLoops = 0;
    }
}

TsLoopParams TsSpline::GetInnerLoopParams() const
{
    return _GetData()->loopParams;
}

////////////////////////////////////////////////////////////////////////////////
// Knots

void TsSpline::SetKnots(const TsKnotMap &knots)
{
    if (_GetData()->isTyped && knots.GetValueType() != GetValueType())
    {
        TF_CODING_ERROR(
            "Mismatched knot map type '%s' passed to TsSpline::SetKnots "
            "for spline of type '%s'",
            knots.GetValueType().GetTypeName().c_str(),
            GetValueType().GetTypeName().c_str());
        return;
    }

    _PrepareForWrite(knots.GetValueType());

    // Remove existing knots.
    _data->ClearKnots();

    // Copy knot data.
    _data->ReserveForKnotCount(knots.size());
    for (const TsKnot &knot : knots)
        _data->PushKnot(knot._GetData(), knot.GetCustomData());

    // De-regress.
    if (TsEditBehaviorBlock::GetStack().empty())
    {
        AdjustRegressiveTangents();
    }
}

bool TsSpline::CanSetKnot(
    const TsKnot &knot,
    std::string* const reasonOut) const
{
    if (_GetData()->isTyped && knot.GetValueType() != GetValueType())
    {
        if (reasonOut)
        {
            *reasonOut = TfStringPrintf(
                "Cannot set knot of value type '%s' "
                "into spline of value type '%s'",
                knot.GetValueType().GetTypeName().c_str(),
                GetValueType().GetTypeName().c_str());
        }
        return false;
    }

    if (knot.GetCurveType() != GetCurveType())
    {
        if (reasonOut)
        {
            *reasonOut = TfStringPrintf(
                "Cannot set knot of curve type '%s' "
                "into spline of curve type '%s'",
                TfEnum::GetName(knot.GetCurveType()).c_str(),
                TfEnum::GetName(GetCurveType()).c_str());
        }
        return false;
    }

    return true;
}

bool TsSpline::SetKnot(
    const TsKnot &knot,
    GfInterval *affectedIntervalOut)
{
    // XXX TODO: affectedIntervalOut

    std::string msg;
    if (!CanSetKnot(knot, &msg))
    {
        TF_CODING_ERROR(msg);
        return false;
    }

    _PrepareForWrite(knot.GetValueType());

    // Copy knot data.
    const size_t idx = _data->SetKnot(knot._GetData(), knot.GetCustomData());

    // De-regress.
    if (TsEditBehaviorBlock::GetStack().empty()
        && _data->curveType == TsCurveTypeBezier)
    {
        // Find indices of knots bounding segments that the knot is part of.
        const size_t first = (idx == 0 ? idx : idx - 1);
        const size_t last = (idx == _data->times.size() - 1 ? idx : idx + 1);

        // Process zero, one, or two segments.
        for (size_t i = first; i < last; i++)
        {
            Ts_KnotData* const startKnot = _data->GetKnotPtrAtIndex(i);
            Ts_KnotData* const endKnot = _data->GetKnotPtrAtIndex(i + 1);
            Ts_RegressionPreventerBatchAccess::ProcessSegment(
                startKnot, endKnot, GetAntiRegressionAuthoringMode());
        }
    }

    return true;
}

void TsSpline::_SetKnotUnchecked(
    const TsKnot &knot)
{
    _PrepareForWrite(knot.GetValueType());
    _data->SetKnot(knot._GetData(), knot.GetCustomData());
}

TsKnotMap TsSpline::GetKnots() const
{
    return TsKnotMap(_GetData());
}

bool TsSpline::GetKnot(
    const TsTime time,
    TsKnot* const knotOut) const
{
    if (!_data)
    {
        return false;
    }

    // Look up and clone knot data.
    Ts_KnotData* const knotData = _data->CloneKnotAtTime(time);
    if (!knotData)
    {
        return false;
    }

    // Look up custom data.
    VtDictionary customData;
    TfMapLookup(_data->customData, time, &customData);

    // Bundle into TsKnot.
    *knotOut = TsKnot(knotData, GetValueType(), std::move(customData));
    return true;
}

////////////////////////////////////////////////////////////////////////////////
// Removing knots

void TsSpline::ClearKnots()
{
    _PrepareForWrite();
    _data->ClearKnots();
}

void TsSpline::RemoveKnot(
    const TsTime time,
    GfInterval* const affectedIntervalOut)
{
    _PrepareForWrite();
    _data->RemoveKnotAtTime(time);

    // XXX TODO: compute affected interval
}

////////////////////////////////////////////////////////////////////////////////
// Evaluation

bool TsSpline::DoSidesDiffer(
    const TsTime time) const
{
    // Simple implementation.  Could probably be faster.
    double preValue = 0, value = 0;
    EvalPreValue(time, &preValue);
    Eval(time, &value);
    return (value == preValue);
}

////////////////////////////////////////////////////////////////////////////////
// Whole-Spline Queries

bool TsSpline::IsEmpty() const
{
    return _GetData()->times.empty();
}

bool TsSpline::HasValueBlocks() const
{
    return _GetData()->HasValueBlocks();
}

bool TsSpline::HasLoops() const
{
    return HasInnerLoops() || HasExtrapolatingLoops();
}

bool TsSpline::HasInnerLoops() const
{
    return _GetData()->HasInnerLoops();
}

bool TsSpline::HasExtrapolatingLoops() const
{
    return (
        _GetData()->preExtrapolation.IsLooping()
        || _GetData()->postExtrapolation.IsLooping());
}

////////////////////////////////////////////////////////////////////////////////
// Within-Spline Queries

bool TsSpline::HasValueBlockAtTime(const TsTime time) const
{
    return _GetData()->HasValueBlockAtTime(time);
}

////////////////////////////////////////////////////////////////////////////////
// Human-readable dump

static std::string _ExtrapDesc(const TsExtrapolation &extrap)
{
    std::ostringstream ss;

    ss << TfEnum::GetName(extrap.mode).substr(8);

    if (extrap.mode == TsExtrapSloped)
    {
        ss << " " << TfStringify(extrap.slope);
    }

    return ss.str();
}

std::ostream& operator<<(std::ostream& out, const TsSpline &spline)
{
    out << "Spline:" << std::endl
        << "  value type " << spline.GetValueType().GetTypeName() << std::endl
        << "  time valued " << spline.IsTimeValued() << std::endl
        << "  curve type "
        << TfEnum::GetName(spline.GetCurveType()).substr(11) << std::endl
        << "  pre extrap "
        << _ExtrapDesc(spline.GetPreExtrapolation()) << std::endl
        << "  post extrap "
        << _ExtrapDesc(spline.GetPostExtrapolation()) << std::endl;

    if (spline.HasInnerLoops())
    {
        const TsLoopParams lp = spline.GetInnerLoopParams();
        out << "Loop:" << std::endl
            << "  start " << TfStringify(lp.protoStart)
            << ", end " << TfStringify(lp.protoEnd)
            << ", numPreLoops " << lp.numPreLoops
            << ", numPostLoops " << lp.numPostLoops
            << ", valueOffset " << TfStringify(lp.valueOffset)
            << std::endl;
    }

    for (const TsKnot &knot : spline.GetKnots())
        out << knot;

    return out;
}

////////////////////////////////////////////////////////////////////////////////
// Applying layer offsets

void Ts_SplineOffsetAccess::ApplyOffsetAndScale(
    TsSpline *spline,
    const TsTime offset,
    const double scale)
{
    spline->_PrepareForWrite();
    spline->_data->ApplyOffsetAndScale(offset, scale);
}

////////////////////////////////////////////////////////////////////////////////
// Helpers

const Ts_SplineData* TsSpline::_GetData() const
{
    // Function-static default data to use when _data is null.
    static const Ts_SplineData* const defaultData =
        Ts_SplineData::Create(TfType());

    return (_data ? _data.get() : defaultData);
}

void TsSpline::_PrepareForWrite(TfType valueType)
{
    // If we had default state, create storage now.  If no value type was
    // specified, the storage will be physically double-typed (anticipating the
    // most common case) but labeled untyped.
    if (!_data)
    {
        _data.reset(Ts_SplineData::Create(valueType));
    }

    // If we're adding our first knot(s), and we have untyped data, make sure we
    // have the correct typed data.
    else if (_data && !_data->isTyped && valueType)
    {
        // If we guessed correctly, upgrade to real storage by marking typed.
        if (valueType == Ts_GetType<double>())
        {
            _data->isTyped = true;
        }

        // Otherwise create new storage and transfer.  The second parameter to
        // Create serves as a copy source for overall spline parameters, which
        // are the purpose of untyped storage.
        else
        {
            _data.reset(Ts_SplineData::Create(valueType, _data.get()));
        }
    }

    // Copy-on-write: if we have shared data, make an independent copy so we can
    // modify it without affecting other TsSpline instances.
    else if (_data && _data.use_count() > 1)
    {
        _data.reset(_data->Clone());
    }
}

////////////////////////////////////////////////////////////////////////////////
// Anti-Regression

#ifndef PXR_TS_DEFAULT_ANTI_REGRESSION_AUTHORING_MODE
#define PXR_TS_DEFAULT_ANTI_REGRESSION_AUTHORING_MODE TsAntiRegressionKeepRatio
#endif

// static
TsAntiRegressionMode
TsSpline::GetAntiRegressionAuthoringMode()
{
    const TsAntiRegressionAuthoringSelector* const selector =
        TsAntiRegressionAuthoringSelector::GetStackTop();
    if (selector)
    {
        return selector->mode;
    }

    return PXR_TS_DEFAULT_ANTI_REGRESSION_AUTHORING_MODE;
}

bool TsSpline::HasRegressiveTangents() const
{
    if (!_data)
    {
        return false;
    }

    if (_data->curveType != TsCurveTypeBezier)
    {
        return false;
    }

    const size_t size = _data->times.size();
    if (size < 2)
    {
        return false;
    }

    for (size_t i = 0; i < size - 1; i++)
    {
        const Ts_KnotData* const startKnot = _data->GetKnotPtrAtIndex(i);
        const Ts_KnotData* const endKnot = _data->GetKnotPtrAtIndex(i + 1);

        if (Ts_RegressionPreventerBatchAccess::IsSegmentRegressive(
                startKnot, endKnot, GetAntiRegressionAuthoringMode()))
        {
            return true;
        }
    }

    return false;
}

bool TsSpline::AdjustRegressiveTangents()
{
    if (!_data)
    {
        return false;
    }

    if (_data->curveType != TsCurveTypeBezier)
    {
        return false;
    }

    const size_t size = _data->times.size();
    if (size < 2)
    {
        return false;
    }

    size_t i = 0;
    bool splineChanged = false;

    // If we're sharing data, start by only querying for regression.
    if (_data.use_count() > 1)
    {
        for (; i < size - 1; i++)
        {
            const Ts_KnotData* const startKnot = _data->GetKnotPtrAtIndex(i);
            const Ts_KnotData* const endKnot = _data->GetKnotPtrAtIndex(i + 1);

            // After we break here, 'i' will still identify the regression, and
            // we'll reuse that index in the modifying loop below.
            if (Ts_RegressionPreventerBatchAccess::IsSegmentRegressive(
                    startKnot, endKnot, GetAntiRegressionAuthoringMode()))
            {
                break;
            }
        }

        // If we didn't get all the way through, then there is regression in the
        // i'th segment.  Copy the data in preparation for modification.
        if (i < size - 1)
        {
            _PrepareForWrite();
        }
    }

    // Iterate over the data, possibly modifying it.  This may occur after we've
    // already examined some of the segments in read-only mode above; it may be
    // skipped because we found no regression above; or it may be the only thing
    // we do, because we were not sharing data.
    for (; i < size - 1; i++)
    {
        Ts_KnotData* const startKnot = _data->GetKnotPtrAtIndex(i);
        Ts_KnotData* const endKnot = _data->GetKnotPtrAtIndex(i + 1);

        if (Ts_RegressionPreventerBatchAccess::ProcessSegment(
                startKnot, endKnot, GetAntiRegressionAuthoringMode()))
        {
            splineChanged = true;
        }
    }

    return splineChanged;
}

////////////////////////////////////////////////////////////////////////////////
// Misc

// XXX: see comment in header.
void swap(TsSpline &lhs, TsSpline &rhs)
{
    std::swap(lhs, rhs);
}


PXR_NAMESPACE_CLOSE_SCOPE
