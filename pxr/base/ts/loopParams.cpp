//
// Copyright 2023 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
