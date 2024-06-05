//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/loopParams.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

TsLoopParams::TsLoopParams(
    bool looping,
    TsTime start,
    TsTime period,
    TsTime preRepeatFrames,
    TsTime repeatFrames,
    double valueOffset) :
    _looping(looping),
    _valueOffset(valueOffset)
{
    if (period <= 0 || preRepeatFrames < 0 || repeatFrames < 0)
        return; // We will be invalid

    _loopedInterval = GfInterval(
            start - preRepeatFrames,
            start + period + repeatFrames,
            true  /* min closed */,
            false /* max closed */);

    _masterInterval = GfInterval(
            start, start + period,
            true  /* min closed */,
            false /* max closed */);
}

TsLoopParams::TsLoopParams() :
    _looping(false),
    _valueOffset(0.0)
{
}

void
TsLoopParams::SetLooping(bool looping)
{
    _looping = looping;
}

bool
TsLoopParams::GetLooping() const
{
    return _looping;
}

TsTime
TsLoopParams::GetStart() const
{
    return _masterInterval.GetMin();
}

TsTime
TsLoopParams::GetPeriod() const
{
    return _masterInterval.GetMax() - _masterInterval.GetMin();
}

TsTime
TsLoopParams::GetPreRepeatFrames() const
{
    return _masterInterval.GetMin() - _loopedInterval.GetMin();
}

TsTime
TsLoopParams::GetRepeatFrames() const
{
    return _loopedInterval.GetMax() - _masterInterval.GetMax();
}

const GfInterval &
TsLoopParams::GetMasterInterval() const
{
    return _masterInterval;
}

const GfInterval &
TsLoopParams::GetLoopedInterval() const
{
    return _loopedInterval;
}

bool
TsLoopParams::IsValid() const
{
    return !_loopedInterval.IsEmpty() && !_masterInterval.IsEmpty();
}

void
TsLoopParams::SetValueOffset(double valueOffset)
{
    _valueOffset = valueOffset;
}

double
TsLoopParams::GetValueOffset() const
{
    return _valueOffset;
}

std::ostream& 
operator<<(std::ostream& out, const TsLoopParams& lp)
{
    out << "("
        << lp.GetLooping() << ", "
        << lp.GetStart() << ", "
        << lp.GetPeriod() << ", "
        << lp.GetPreRepeatFrames() << ", "
        << lp.GetRepeatFrames() << ", "
        << lp.GetValueOffset()
        << ")";
    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE
