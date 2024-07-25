//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/tsTest_SampleTimes.h"
#include "pxr/base/ts/tsTest_SplineData.h"
#include "pxr/base/tf/diagnostic.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

using SData = TsTest_SplineData;

////////////////////////////////////////////////////////////////////////////////

TsTest_SampleTimes::SampleTime::SampleTime() = default;

TsTest_SampleTimes::SampleTime::SampleTime(
    double timeIn)
    : time(timeIn) {}

TsTest_SampleTimes::SampleTime::SampleTime(
    double timeIn, bool preIn)
    : time(timeIn), pre(preIn) {}

TsTest_SampleTimes::SampleTime&
TsTest_SampleTimes::SampleTime::operator=(
    double timeIn)
{
    *this = SampleTime(timeIn);
    return *this;
}

bool TsTest_SampleTimes::SampleTime::operator<(
    const SampleTime &other) const
{
    return time < other.time
        || (time == other.time && pre && !other.pre);
}

bool TsTest_SampleTimes::SampleTime::operator==(
    const SampleTime &other) const
{
    return time == other.time && pre == other.pre;
}

bool TsTest_SampleTimes::SampleTime::operator!=(
    const SampleTime &other) const
{
    return !(*this == other);
}

////////////////////////////////////////////////////////////////////////////////

TsTest_SampleTimes::SampleTimeSet
TsTest_SampleTimes::_GetKnotTimes() const
{
    SampleTimeSet result;

    // Examine all knots.
    bool held = false;
    for (const SData::Knot &knot : _splineData.GetKnots())
    {
        if (held || knot.isDualValued)
        {
            result.insert(SampleTime(knot.time, /* pre = */ true));
        }

        result.insert(SampleTime(knot.time));

        held = (knot.nextSegInterpMethod == SData::InterpHeld);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////

TsTest_SampleTimes::TsTest_SampleTimes()
    : _haveSplineData(false)
{
}

TsTest_SampleTimes::TsTest_SampleTimes(
    const TsTest_SplineData &splineData)
    : _haveSplineData(true),
      _splineData(splineData)
{
}

void TsTest_SampleTimes::AddTimes(
    const std::vector<double> &times)
{
    for (const double time : times)
        _times.insert(SampleTime(time));
}

void TsTest_SampleTimes::AddTimes(
    const std::vector<SampleTime> &times)
{
    _times.insert(times.begin(), times.end());
}

void TsTest_SampleTimes::AddKnotTimes()
{
    if (!_haveSplineData)
    {
        TF_CODING_ERROR("AddKnotTimes: no spline data");
        return;
    }

    const SampleTimeSet knotTimes = _GetKnotTimes();
    _times.insert(knotTimes.begin(), knotTimes.end());
}

void TsTest_SampleTimes::AddUniformInterpolationTimes(
    const int numSamples)
{
    if (!_haveSplineData)
    {
        TF_CODING_ERROR("AddUniformInterpolationTimes: no spline data");
        return;
    }

    if (numSamples < 1)
    {
        TF_CODING_ERROR("AddUniformInterpolationTimes: Too few samples");
        return;
    }

    const SampleTimeSet knotTimes = _GetKnotTimes();
    if (knotTimes.size() < 2)
    {
        TF_CODING_ERROR("AddUniformInterpolationTimes: Too few knots");
        return;
    }

    const double firstTime = knotTimes.begin()->time;
    const double lastTime = knotTimes.rbegin()->time;
    const double knotRange = lastTime - firstTime;
    const double step = knotRange / (numSamples + 1);

    for (int i = 0; i < numSamples - 1; i++)
        _times.insert(SampleTime(firstTime + i * step));
}

void TsTest_SampleTimes::AddExtrapolationTimes(
    const double extrapolationFactor)
{
    if (!_haveSplineData)
    {
        TF_CODING_ERROR("AddExtrapolationTimes: no spline data");
        return;
    }

    if (extrapolationFactor <= 0.0)
    {
        TF_CODING_ERROR("AddExtrapolationTimes: invalid factor");
        return;
    }

    // Use this simplistic technique for non-looped extrapolation, which always
    // produces a straight line, and so can be illustrated with a single sample.
    const bool preLoop =
        (_splineData.GetPreExtrapolation().method == SData::ExtrapLoop);
    const bool postLoop =
        (_splineData.GetPostExtrapolation().method == SData::ExtrapLoop);
    if (preLoop && postLoop)
    {
        return;
    }

    const SampleTimeSet knotTimes = _GetKnotTimes();
    if (knotTimes.size() < 2)
    {
        TF_CODING_ERROR("AddExtrapolationTimes: too few knots");
        return;
    }

    const double firstTime = knotTimes.begin()->time;
    const double lastTime = knotTimes.rbegin()->time;
    const double knotRange = lastTime - firstTime;
    const double extrap = extrapolationFactor * knotRange;

    if (!preLoop)
    {
        _times.insert(SampleTime(firstTime - extrap));
    }
    if (!postLoop)
    {
        _times.insert(SampleTime(lastTime + extrap));
    }
}

void TsTest_SampleTimes::AddExtrapolatingLoopTimes(
    const int numIterations,
    const int numSamplesPerIteration)
{
    if (!_haveSplineData)
    {
        TF_CODING_ERROR("AddExtrapolatingLoopTimes: no spline data");
        return;
    }

    if (numIterations <= 0)
    {
        TF_CODING_ERROR("AddExtrapolatingLoopTimes: invalid iteration count");
        return;
    }

    // Use this technique for looped extrapolation, which requires copies of the
    // sample times from the knot range to illustrate properly.
    const bool preLoop =
        (_splineData.GetPreExtrapolation().method == SData::ExtrapLoop);
    const bool postLoop =
        (_splineData.GetPostExtrapolation().method == SData::ExtrapLoop);
    if (!preLoop && !postLoop)
    {
        return;
    }

    const SampleTimeSet knotTimes = _GetKnotTimes();
    if (knotTimes.size() < 2)
    {
        TF_CODING_ERROR("AddExtrapolatingLoopTimes: too few knots");
        return;
    }

    const double firstTime = knotTimes.begin()->time;
    const double lastTime = knotTimes.rbegin()->time;
    const double knotRange = lastTime - firstTime;

    // Create standard times for the knot range using a sub-instance.
    TsTest_SampleTimes knotRangeTimes(_splineData);
    knotRangeTimes.AddKnotTimes();
    knotRangeTimes.AddUniformInterpolationTimes(numSamplesPerIteration);
    const SampleTimeSet &knotRangeSet = knotRangeTimes.GetTimes();

    if (preLoop)
    {
        // Make a copy of the knot-range times for each pre-iteration.
        for (int i = -numIterations; i < 0; i++)
        {
            for (const SampleTime &sample : knotRangeSet)
            {
                _times.insert(
                    SampleTime(
                        sample.time + knotRange * i,
                        sample.pre));
            }
        }

        // Add a pre-time at each pre-loop join.
        for (int i = -numIterations; i <= 0; i++)
        {
            _times.insert(
                SampleTime(
                    firstTime + knotRange * i,
                    /* pre = */ true));
        }
    }

    if (postLoop)
    {
        // Make a copy of the knot-range times for each post-iteration.
        for (int i = 1; i <= numIterations; i++)
        {
            for (const SampleTime &sample : knotRangeSet)
            {
                _times.insert(
                    SampleTime(
                        sample.time + knotRange * i,
                        sample.pre));
            }
        }

        // Add a pre-time at each post-loop join.
        for (int i = 0; i <= numIterations; i++)
        {
            _times.insert(
                SampleTime(
                    lastTime + knotRange * i,
                    /* pre = */ true));
        }
    }
}

void TsTest_SampleTimes::AddStandardTimes()
{
    AddKnotTimes();
    AddUniformInterpolationTimes(200);
    AddExtrapolationTimes(0.2);
    AddExtrapolatingLoopTimes(3, 200);
}

const TsTest_SampleTimes::SampleTimeSet&
TsTest_SampleTimes::GetTimes() const
{
    return _times;
}

PXR_NAMESPACE_CLOSE_SCOPE
