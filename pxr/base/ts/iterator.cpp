//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/iterator.h"

#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

constexpr double inf = std::numeric_limits<double>::infinity();

// Constants for constructing GfInterval objects
constexpr bool OPEN = false;
constexpr bool CLOSED = true;

// Get a looped interval that's open on the right-hand end. The one returned by
// TsLoopParams::GetLoopedInterval() is closed on the right and changing that
// breaks a couple of existing tests.
// XXX: Revisit the possibility of changing TsLoopParams::GetLoopedInterval() once
// our baking and sampling code has been converted to use these iterators.
GfInterval _GetOpenLoopedInterval(const TsLoopParams& lp)
{
    GfInterval result = lp.GetLoopedInterval();
    result.SetMax(result.GetMax(), OPEN);
    return result;
}

bool _SegmentAtEnd(
    const GfInterval& interval,
    const Ts_Segment& segment,
    const bool reversed)
{
    if (reversed) {
        const double t0 = segment.p0[0];
        // When reversed, don't go back in time if we have the earliest
        // post-value needed by the interval.
        if (!interval.Contains(t0) || t0 == interval.GetMin()) {
            return true;
        }
    } else {
        const double t1 = segment.p1[0];
        if (!interval.Contains(t1)) {
            return true;
        }
    }
    return false;
}

bool _SegmentOverlapsInterval(
    const GfInterval& interval,
    const Ts_Segment& segment)
{
    const GfInterval segInterval(segment.p0[0], segment.p1[0], CLOSED, OPEN);
    return !((segInterval & interval).IsEmpty());
}

}  // end anonymous namespace


////////////////////////////////////////////////////////////////
// Ts_SegmentPrototypeIterator

Ts_SegmentPrototypeIterator::Ts_SegmentPrototypeIterator(
    const TsSpline& spline,
    const GfInterval& interval,
    bool reversed /*= false*/)
: Ts_SegmentPrototypeIterator(Ts_GetSplineData(spline),
                              interval,
                              reversed)
{ }

Ts_SegmentPrototypeIterator::Ts_SegmentPrototypeIterator(
    const Ts_SplineData* data,
    const GfInterval& interval,
    bool reversed /*= false*/)
: _data(data)
, _interval(interval)
, _reversed(reversed)
, _atEnd(false)
{
    if (!_data || !_data->HasInnerLoops(&_firstProtoKnotIndex)) {
        _atEnd = true;
        return;
    }

    const GfInterval protoInterval = _data->loopParams.GetPrototypeInterval();
    GfInterval iterInterval = interval & protoInterval;

    if (iterInterval.IsEmpty()) {
        _atEnd = true;
        _interval = iterInterval;
        return;
    }

    // Save some typing
    const auto& times = _data->times;

    // XXX: There are slight variations on this if/else block in the prototype,
    // the knot, and the segment iterators. It might be nice to unify them one
    // day.
    if (_reversed) {
        // Find the beginning of the segment containing max time.
        _timesIt = std::lower_bound(times.begin(), times.end(),
                                    iterInterval.GetMax());
        if (_timesIt != times.end() &&
            _interval.IsMaxClosed() &&
            *_timesIt == _interval.GetMax())
        {
            // do nothing, this knot is the beginning of the segment.
        } else if (_timesIt != times.begin()) {
            --_timesIt;
        }
    } else {
        // Find the beginning of the segment containing min time.
        _timesIt = std::upper_bound(times.begin(), times.end(),
                                    iterInterval.GetMin());

        if (_timesIt != times.begin()) {
            --_timesIt;
        }
    }

    _UpdateSegment();
    _atEnd = false;
}

void
Ts_SegmentPrototypeIterator::_UpdateSegment()
{
    const auto& times = _data->times;
    const auto& lp = _data->loopParams;

    ptrdiff_t prevIndex = std::distance(times.begin(), _timesIt);
    Ts_TypedKnotData<double> prevKnot, nextKnot;
    prevKnot = _data->GetKnotDataAsDouble(prevIndex);

    ptrdiff_t nextIndex = prevIndex + 1;
    if (nextIndex < ptrdiff_t(times.size()) && times[nextIndex] < lp.protoEnd) {
        // Still iterating within the prototype knots.
        nextKnot = _data->GetKnotDataAsDouble(nextIndex);
    } else {
        // We've gone past the last knot in the prototype region and need to use
        // a virtual copy of the first knot (with appropriate offsets)
        nextKnot = _data->GetKnotDataAsDouble(_firstProtoKnotIndex);
        nextKnot.time += (lp.protoEnd - lp.protoStart);
        nextKnot.preValue += lp.valueOffset;
        nextKnot.value += lp.valueOffset;
    }

    _segment.p0.Set(prevKnot.time, prevKnot.value);
    _segment.p1.Set(nextKnot.time, nextKnot.GetPreValue());
    _segment.t0 = _segment.p0 +
                  GfVec2d(prevKnot.postTanWidth,
                          prevKnot.postTanWidth * prevKnot.postTanSlope);
    _segment.t1 = _segment.p1 +
                  GfVec2d(-nextKnot.preTanWidth,
                          nextKnot.GetPreTanHeight());
    _segment.SetInterp(prevKnot.nextInterp, _data->curveType);
}

Ts_Segment Ts_SegmentPrototypeIterator::operator *() const
{
    return _segment;
}

Ts_SegmentPrototypeIterator& Ts_SegmentPrototypeIterator::operator ++()
{
    if (_SegmentAtEnd(_interval, _segment, _reversed)) {
        _atEnd = true;
        return *this;
    }

    StepForward();
    return *this;
}

bool
Ts_SegmentPrototypeIterator::StepForward()
{
    // We can't move if we don't have an initialization anchor time.
    if (_interval.IsEmpty()) {
        _atEnd = true;
        return false;
    }

    // Ensure _timesIt is within range.
    const auto& times = _data->times;
    if (_timesIt < times.begin() || _timesIt >= times.end()) {
        _atEnd = true;
        return false;
    }

    // We can't step beyond the prototype basis.
    const auto& lp = _data->loopParams;
    if ((_reversed && *_timesIt <= lp.protoStart) ||
        (!_reversed && *_timesIt >= lp.protoEnd))
    {
        _atEnd = true;
        return false;
    }

    if (_reversed) {
        // If we cannot decrement the iterator or if it already points outside
        // the times region, set _atEnd to true.
        if (_timesIt == times.begin()) {
            _atEnd = true;
            return false;
        }
        --_timesIt;
    } else {
        if (_timesIt + 1 == times.end() ||
            *(_timesIt + 1) >= lp.protoEnd)
        {
            _atEnd = true;
            return false;
        }
        ++_timesIt;
    }

    _UpdateSegment();

    _atEnd = !_SegmentOverlapsInterval(_interval, _segment);
    return true;
}

bool
Ts_SegmentPrototypeIterator::StepBackward()
{
    _reversed = !_reversed;
    const bool success = StepForward();
    _reversed = !_reversed;
    return success;
}

////////////////////////////////////////////////////////////////
// Ts_SegmentLoopIterator

Ts_SegmentLoopIterator::Ts_SegmentLoopIterator(
    const TsSpline& spline,
    const GfInterval& interval,
    bool reversed /*= false*/)
: Ts_SegmentLoopIterator(Ts_GetSplineData(spline),
                         interval,
                         reversed)
{ }

Ts_SegmentLoopIterator::Ts_SegmentLoopIterator(
    const Ts_SplineData* data,
    const GfInterval& interval,
    bool reversed /*= false*/)
: _data(data)
, _interval(interval)
, _reversed(reversed)
, _atEnd(false)
{
    if (!_data || !_data->HasInnerLoops()) {
        _atEnd = true;
        return;
    }

    const TsLoopParams& lp = _data->loopParams;

    const GfInterval loopedInterval = _GetOpenLoopedInterval(lp);
    GfInterval iterInterval = _interval & loopedInterval;

    if (iterInterval.IsEmpty()) {
        _atEnd = true;
        _interval = iterInterval;
        return;
    }

    const double protoStart = lp.protoStart;
    const double protoEnd = lp.protoEnd;
    _protoSpan = protoEnd - protoStart;
    _valueOffset = lp.valueOffset;

    _minIteration = -lp.numPreLoops;
    _maxIteration = lp.numPostLoops;

    double initialTimeOffset;
    if (_reversed) {
        initialTimeOffset = iterInterval.GetMax() - protoStart;
        _curIteration = int32_t(std::floor(initialTimeOffset / _protoSpan));
        if (_curIteration * _protoSpan == initialTimeOffset) {
            // initialTimeOffset is exactly a multiple of _protoSpan.
            // Since we're iterating backwards, skip this iteration because
            // it is zero width and go to the previous iteration where
            // this time will be the end of the segment rather than the
            // beginning.
            --_curIteration;
        }
    } else {
        initialTimeOffset = iterInterval.GetMin() - protoStart;
        _curIteration = int32_t(std::floor(initialTimeOffset / _protoSpan));
    }
    _curIteration = std::clamp(_curIteration, _minIteration, _maxIteration);

    _UpdatePrototypeIterator();
    _UpdateSegment();
    _atEnd = _protoIt.AtEnd();
}

void
Ts_SegmentLoopIterator::_UpdateSegment()
{
    _segment = *_protoIt;
    GfVec2d delta(_curIteration * _protoSpan, _curIteration * _valueOffset);
    _segment += delta;
}

void
Ts_SegmentLoopIterator::_UpdatePrototypeIterator()
{
    const auto& lp = _data->loopParams;
    const double timeDelta = _curIteration * _protoSpan;

    // Use the full proto interval unless the starting _interval boundary
    // falls within the current iteration.
    GfInterval protoInterval(lp.protoStart, lp.protoEnd, CLOSED, OPEN);
    if (_reversed) {
        const double startTime = _interval.GetMax() - timeDelta;
        if (protoInterval.Contains(startTime)) {
            protoInterval.SetMax(startTime, OPEN);
        }
    } else {
        const double startTime = _interval.GetMin() - timeDelta;
        if (protoInterval.Contains(startTime)) {
            protoInterval.SetMin(startTime, CLOSED);
        }
    }

    _protoIt = Ts_SegmentPrototypeIterator(_data,
                                           protoInterval,
                                           _reversed);
}

Ts_Segment
Ts_SegmentLoopIterator::operator *() const
{
    return _segment;
}

Ts_SegmentLoopIterator&
Ts_SegmentLoopIterator::operator ++()
{
    if (_SegmentAtEnd(_interval, _segment, _reversed)) {
        _atEnd = true;
        return *this;
    }

    StepForward();
    return *this;
}

bool
Ts_SegmentLoopIterator::StepForward()
{
    // We can't move if we don't have an initialization anchor time.
    if (_interval.IsEmpty()) {
        _atEnd = true;
        return false;
    }

    // Ensure we're operating within unrolled inner loop region
    GfInterval loopedInterval = _GetOpenLoopedInterval(_data->loopParams);
    if (_SegmentAtEnd(loopedInterval, _segment, _reversed)) {
        _atEnd = true;
        return false;
    }

    const bool stepBackward =
        _steppingBackward ^ _initializedPrototypeItBackward;
    bool success = stepBackward ? _protoIt.StepBackward()
                                : _protoIt.StepForward();
    if (!success) {
        // Hit end of a prototype loop.
        _curIteration += (_reversed ? -1 : 1);

        if ((_reversed && _curIteration < _minIteration) ||
            (!_reversed && _curIteration > _maxIteration))
        {
            _atEnd = true;
            return false;
        }

        _initializedPrototypeItBackward = _steppingBackward;
        _UpdatePrototypeIterator();
    }

    _UpdateSegment();
    _atEnd = !_SegmentOverlapsInterval(_interval, _segment);
    return true;
}

bool
Ts_SegmentLoopIterator::StepBackward()
{
    _reversed = !_reversed;
    _steppingBackward = true;
    const bool success = StepForward();
    _steppingBackward = false;
    _reversed = !_reversed;
    return success;
}

////////////////////////////////////////////////////////////////
// Ts_SegmentKnotIterator

Ts_SegmentKnotIterator::Ts_SegmentKnotIterator(
    const TsSpline& spline,
    const GfInterval& interval,
    bool reversed /*= false*/)
: Ts_SegmentKnotIterator(Ts_GetSplineData(spline),
                         interval,
                         reversed)
{ }

Ts_SegmentKnotIterator::Ts_SegmentKnotIterator(
    const Ts_SplineData* data,
    const GfInterval& interval,
    bool reversed /*= false*/)
: _data(data)
, _interval(interval)
, _reversed(reversed)
, _atEnd(false)
{
    if (!_data) {
        _atEnd = true;
        return;
    }

    // Save some typing
    const auto& times = _data->times;

    double firstTime = times.front();
    double lastTime = times.back();

    _hasInnerLoops = _data->HasInnerLoops(&_firstProtoKnotIndex);
    if (_hasInnerLoops) {
        _loopedInterval = _GetOpenLoopedInterval(_data->loopParams);
        firstTime = std::min(firstTime, _loopedInterval.GetMin());
        lastTime = std::max(lastTime, _loopedInterval.GetMax());
    }

    if (times.size() < 2 && !_hasInnerLoops) {
        // There are no segments between knots
        _atEnd = true;
        return;
    }

    _interval &= GfInterval(firstTime, lastTime, CLOSED, OPEN);
    if (_interval.IsEmpty()) {
        _atEnd = true;
        return;
    }

    if (_hasInnerLoops) {
        if (_reversed) {
            if (times.back() > _loopedInterval.GetMax() &&
                _interval.GetMax() > _loopedInterval.GetMax())
            {
                // We're iterating (in reverse) after the end of inner looping.
                _knotSection = PostInnerLooping;
            } else if (_interval.GetMax() > _loopedInterval.GetMin()) {
                // We're starting iterations in the middle of the inner looped
                // section.
                _knotSection = InnerLooping;
            } else if (_interval.GetMax() > times.front()) {
                // We're starting iterations (in reverse) before the inner looping.
                _knotSection = PreInnerLooping;
            } else {
                // We're starting (in reverse) iterations before the start of the knots.
                _knotSection = PreInnerLooping;
                _atEnd = true;
                return;
            }
        } else {
            if (times.front() < _loopedInterval.GetMin() &&
                _interval.GetMin() < _loopedInterval.GetMin())
            {
                // We're iterating before the start of inner looping.
                _knotSection = PreInnerLooping;
            } else if (_interval.GetMin() < _loopedInterval.GetMax()) {
                // We're starting iterations in the middle of the inner looped
                // section
                _knotSection = InnerLooping;
            } else if (_interval.GetMin() < times.back()) {
                // We're starting iterations after the inner looping
                _knotSection = PostInnerLooping;
            } else {
                // We're starting iterations after the end of the knots.
                _knotSection = PostInnerLooping;
                _atEnd = true;
                return;
            }
        }
    } else {
        // No inner looping. Set _knotSection to the section that would be
        // iterated after inner looping.
        _knotSection = PostInnerLooping;
    }

    if (_knotSection == InnerLooping) {
        _loopIt = Ts_SegmentLoopIterator(_data, _interval, _reversed);
    } else {
        // XXX: There are slight variations on this if/else block in the
        // prototype, the knot, and the segment iterators. It might be nice to
        // unify them one day.
        if (_reversed) {
            // Find the start of the segment we're interested in.
            _timesIt = std::lower_bound(times.begin(), times.end(),
                                        _interval.GetMax());
            if (_timesIt != times.end() &&
                _interval.IsMaxClosed() &&
                *_timesIt == _interval.GetMax())
            {
                // do nothing, this knot is the beginning of the segment
            } else if (_timesIt != times.begin()) {
                --_timesIt;
            }
        } else {
            // Find the start of the segment we're interested in.
            _timesIt = std::upper_bound(times.begin(), times.end(),
                                        _interval.GetMin());
            if (_timesIt != times.begin()) {
                --_timesIt;
            }
        }
    }

    _UpdateSegment();
}

void
Ts_SegmentKnotIterator::_UpdateSegment()
{
    // If we're in the inner looping section, just let _loopIt do the work.
    if (_knotSection == InnerLooping) {
        _segment = *_loopIt;
        return;
    }

    // Almost the same algorithm as in Ts_SegmentPrototypeIterator. The
    // difference is that we have to check to see if we're iterating up against
    // the inner looped section in which case we need to update the knot that's
    // running into the looping.
    const auto& times = _data->times;
    const auto& lp = _data->loopParams;

    Ts_TypedKnotData<double> prevKnot, nextKnot;
    ptrdiff_t prevIndex = std::distance(times.begin(), _timesIt);

    // Note that nextIndex is safe to use. _atEnd is set to true when _timesIt
    // points to the last knot and _UpdateSegment would have exited early and
    // never reached this point. So we know that there's at least one more knot
    // that nextIndex can safely access.
    ptrdiff_t nextIndex = prevIndex + 1;

    // Check to see if we've run into the back end of _loopedInterval
    if (_hasInnerLoops && _knotSection == PostInnerLooping &&
        times[prevIndex] <= _loopedInterval.GetMax())
    {
        prevKnot = _data->GetKnotDataAsDouble(_firstProtoKnotIndex);
        // Note: use numPostLoops + 1 because at the end of the 0th loop we
        // use 1 times the offset and increase from there.
        prevKnot.time += (lp.protoEnd - lp.protoStart) * (lp.numPostLoops + 1);
        prevKnot.value += lp.valueOffset * (lp.numPostLoops + 1);
    } else {
        prevKnot = _data->GetKnotDataAsDouble(prevIndex);
    }

    // Check to see if we've run into the front end of _loopedInterval
    if (_hasInnerLoops && _knotSection == PreInnerLooping &&
        times[nextIndex] >= _loopedInterval.GetMin())
    {
        nextKnot = _data->GetKnotDataAsDouble(_firstProtoKnotIndex);
        // Note: when at the start of the prototype, we have 0 times the offset
        // at the start of the 0th loop.
        nextKnot.time -= (lp.protoEnd - lp.protoStart) * lp.numPreLoops;
        nextKnot.value -= lp.valueOffset * lp.numPreLoops;
    } else {
        nextKnot = _data->GetKnotDataAsDouble(nextIndex);
    }

    _segment.p0.Set(prevKnot.time, prevKnot.value);
    _segment.p1.Set(nextKnot.time, nextKnot.GetPreValue());
    _segment.t0 = _segment.p0 +
                  GfVec2d(prevKnot.postTanWidth,
                          prevKnot.postTanWidth * prevKnot.postTanSlope);
    _segment.t1 = _segment.p1 +
                  GfVec2d(-nextKnot.preTanWidth,
                          nextKnot.GetPreTanHeight());
    _segment.SetInterp(prevKnot.nextInterp, _data->curveType);
}

Ts_Segment
Ts_SegmentKnotIterator::operator *() const
{
    return _segment;
}

Ts_SegmentKnotIterator&
Ts_SegmentKnotIterator::operator ++()
{
    if (_SegmentAtEnd(_interval, _segment, _reversed)) {
        _atEnd = true;
        return *this;
    }

    StepForward();
    return *this;
}

bool
Ts_SegmentKnotIterator::StepForward()
{
    // We can't move if we don't have an initialization anchor time.
    if (_interval.IsEmpty()) {
        _atEnd = true;
        return false;
    }

    const auto& times = _data->times;

    if (_knotSection == InnerLooping) {
        const bool stepBackward =
            _steppingBackward ^ _initializedLoopItBackward;

        const bool success = stepBackward ? _loopIt.StepBackward()
                                          : _loopIt.StepForward();
        if (!success) {
            // _loopIt is exhausted. Move to the next knot section and set
            // _timesIt correctly. This is essentially the same algorithm
            // used in the constructor.
            _knotSection = _reversed ? PreInnerLooping : PostInnerLooping;

            // XXX: There are slight variations on this if/else block in the
            // prototype, the knot, and the segment iterators. It might be
            // nice to unify them one day.
            if (_reversed) {
                // Find the start of the segment we're interested in.
                _timesIt = std::lower_bound(times.begin(), times.end(),
                                            _loopedInterval.GetMin());
                if (_timesIt == times.begin()) {
                    _atEnd = true;
                    return false;
                }
                --_timesIt;
            } else {
                _timesIt = std::upper_bound(times.begin(), times.end(),
                                            _loopedInterval.GetMax());
                if (_timesIt == times.end() ||
                    _timesIt == times.begin())
                {
                    _atEnd = true;
                    return false;
                }
                --_timesIt;
            }
        }
    } else {
        if (_reversed) {
            if (_timesIt == times.begin() ||
                _timesIt + 1 >= times.end())
            {
                _atEnd = true;
                return false;
            }
            --_timesIt;
            if (_knotSection == PostInnerLooping &&
                *_timesIt < _loopedInterval.GetMax())
            {
                // We have just entered the inner looping section.
                _knotSection = InnerLooping;
                _initializedLoopItBackward = _steppingBackward;
                _loopIt = Ts_SegmentLoopIterator(_data,
                                                 _interval,
                                                 _reversed);
            }
        } else {
            if (_timesIt == times.end() ||
                (_knotSection == PostInnerLooping &&
                 _timesIt + 2 >= times.end()))
            {
                _atEnd = true;
                return false;
            }
            ++_timesIt;

            if (_knotSection == PreInnerLooping &&
                *_timesIt >= _loopedInterval.GetMin())
            {
                // We have just entered the inner looping section.
                _knotSection = InnerLooping;
                _initializedLoopItBackward = _steppingBackward;
                _loopIt = Ts_SegmentLoopIterator(_data,
                                                 _interval,
                                                 _reversed);
            }
        }
    }

    _UpdateSegment();
    _atEnd = !_SegmentOverlapsInterval(_interval, _segment);
    return true;
}

bool
Ts_SegmentKnotIterator::StepBackward()
{
    _reversed = !_reversed;
    _steppingBackward = true;
    const bool success = StepForward();
    _steppingBackward = false;
    _reversed = !_reversed;
    return success;
}

////////////////////////////////////////////////////////////////
// Ts_SegmentIterator

Ts_SegmentIterator::Ts_SegmentIterator(
    const TsSpline& spline,
    const GfInterval& interval)
: Ts_SegmentIterator(Ts_GetSplineData(spline),
                     interval)
{ }

Ts_SegmentIterator::Ts_SegmentIterator(
    const Ts_SplineData* data,
    const GfInterval& interval)
: _data(data)
, _interval(interval)
, _atEnd(false)
{
    if (_interval.IsEmpty() ||
        !_data ||
        _data->times.empty()) {
        // We're done.
        _atEnd = true;
        return;
    }

    const auto& times = _data->times;
    const auto& lp = _data->loopParams;

    _firstKnotTime = times.front();
    _lastKnotTime = times.back();

    _firstKnotPreValue = _data->GetKnotPreValueAsDouble(0);
    _firstKnotValue = _data->GetKnotValueAsDouble(0);
    _lastKnotPreValue = _data->GetKnotPreValueAsDouble(times.size() - 1);
    _lastKnotValue = _data->GetKnotValueAsDouble(times.size() - 1);


    size_t firstProtoIndex;
    if (_data->HasInnerLoops(&firstProtoIndex)) {
        GfInterval loopedInterval = _GetOpenLoopedInterval(lp);

        if (loopedInterval.GetMin() <= _firstKnotTime) {
            // The start of the knots is looped.
            _firstKnotTime = loopedInterval.GetMin();
            _firstKnotPreValue =
                _data->GetKnotPreValueAsDouble(firstProtoIndex);
            _firstKnotPreValue -= lp.valueOffset * lp.numPreLoops;
            _firstKnotValue = _data->GetKnotValueAsDouble(firstProtoIndex);
            _firstKnotValue -= lp.valueOffset * lp.numPreLoops;
        }

        if (loopedInterval.GetMax() >= _lastKnotTime) {
            // The end of the knots is looped.
            _lastKnotTime = loopedInterval.GetMax();
            _lastKnotPreValue = _data->GetKnotPreValueAsDouble(firstProtoIndex);
            _lastKnotPreValue += lp.valueOffset * (lp.numPostLoops + 1);
            _lastKnotValue = _data->GetKnotValueAsDouble(firstProtoIndex);
            _lastKnotValue += lp.valueOffset * (lp.numPostLoops + 1);
        }
    }

    _preExtrapLooped = _InitExtrapLooping(true);
    _postExtrapLooped = _InitExtrapLooping(false);
    if ((_preExtrapLooped && !_interval.IsMinFinite()) ||
        (_postExtrapLooped && !_interval.IsMaxFinite()))
    {
        TF_CODING_ERROR("Cannot iterate an infinitely looping spline across"
                        " an infinite time interval.");
        _atEnd = true;
        return;
    }

    // Compute the starting iteration.
    _curIteration = 0;
    if (_preExtrapLooped && _interval.GetMin() < _firstKnotTime) {
        const double knotSpan = _lastPreExtrapKnotTime - _firstKnotTime;
        const double offset = _interval.GetMin() - _firstKnotTime;
        _curIteration = int32_t(std::floor(offset / knotSpan));
    } else if (_postExtrapLooped && _interval.GetMin() >= _lastKnotTime) {
        const double knotSpan = _lastKnotTime - _firstPostExtrapKnotTime;
        const double offset = _interval.GetMin() - _lastKnotTime;
        _curIteration = 1 + int32_t(std::floor(offset / knotSpan));
    }

    _minIteration = _maxIteration = 0;
    if (_preExtrapLooped) {
        _minIteration = std::numeric_limits<int32_t>::min();
    }
    if (_postExtrapLooped) {
        _maxIteration = std::numeric_limits<int32_t>::max();
    }

    if (_interval.GetMin() < _firstKnotTime) {
        _splineRegion = (_preExtrapLooped
                         ? TsSourcePreExtrapLoop
                         : TsSourcePreExtrap);
    } else if (_interval.GetMin() < _lastKnotTime) {
        _splineRegion = TsSourceKnotInterp;
    } else {
        _splineRegion = (_postExtrapLooped
                         ? TsSourcePostExtrapLoop
                         : TsSourcePostExtrap);
    }

    _atEnd = false;
    _UpdateKnotIterator();
    _UpdateSegment();
}

bool
Ts_SegmentIterator::_InitExtrapLooping(bool isPre)
{
    // Extrapolation looping behaves as if it were held when there is only
    // one knot in the spline.
    if (_firstKnotTime >= _lastKnotTime) {
        return false;
    }

    const auto &times = _data->times;
    const TsExtrapolation& extrap = isPre ? _data->preExtrapolation
                                          : _data->postExtrapolation;
    if (!extrap.IsLooping()) {
        return false;
    }
    
    // When loopBoundaryTime isn't specified, looping extrap loops over the
    // whole knot region.
    if (!extrap.loopBoundaryTime.has_value()) {
        if (isPre) {
            _lastPreExtrapKnotTime = _lastKnotTime;
            _lastPreExtrapKnotPreValue = _lastKnotPreValue;
        } else {
            _firstPostExtrapKnotTime = _firstKnotTime;
            _firstPostExtrapKnotValue = _firstKnotValue;
        }
        return true;
    }

    const TsTime boundary = extrap.loopBoundaryTime.value();

    // Extrapolation has "held" behavior when loopBoundaryTime is
    // specified and the resulting looped region has size zero.
    if ((isPre && boundary == _firstKnotTime) ||
        (!isPre && boundary == _lastKnotTime))
    {
        return false;
    }

    const auto it = std::lower_bound(times.begin(), times.end(), boundary);
    if (it == times.end() || *it != boundary) {
        // Boundary time doesn't match a knot time, the extrap region
        // will be value block.
        return false;
    }

    // loopBoundaryTime specifies a valid subregion. Set values accordingly.
    const size_t index = std::distance(times.begin(), it);
    if (isPre) {
        _lastPreExtrapKnotTime = boundary;
        _lastPreExtrapKnotPreValue = _data->GetKnotPreValueAsDouble(index);
    } else {
        _firstPostExtrapKnotTime = boundary;
        _firstPostExtrapKnotValue = _data->GetKnotValueAsDouble(index);
    }
    return true;
}

Ts_Segment
Ts_SegmentIterator::operator *() const
{
    return _segment;
}

Ts_SegmentIterator&
Ts_SegmentIterator::operator ++()
{
    if (_SegmentAtEnd(_interval, _segment, /* reversed */ false)) {
        _atEnd = true;
        return *this;
    }

    StepForward();
    return *this;
}

bool
Ts_SegmentIterator::StepForward()
{
    // We can't move if we don't have an initialization anchor time.
    if (_interval.IsEmpty()) {
        return false;
    }

    // If we're advancing out of post-extrapolation, we're done.
    if (_splineRegion == TsSourcePostExtrap) {
        _atEnd = true;
        return false;
    }

    bool initializedKnotIt = false;
    if (_splineRegion == TsSourcePreExtrap) {
        // Advancing out of pre-extrapolation. Assume we're advancing
        // to knot interpolation until proven otherwise.
        _splineRegion = TsSourceKnotInterp;
        _curIteration = 0;
        _initializedKnotItBackward = false;
        initializedKnotIt = true;
        // It's possible that there's only one knot and we're going to
        // advance straight to post-extrap. Update the iterator here, if it
        // starts AtEnd() then there are no knot segments.
        _UpdateKnotIterator();
    } else {
        const bool success =
            _initializedKnotItBackward ? _knotIt.StepBackward()
                                        : _knotIt.StepForward();
        if (!success ||
            !_SegmentOverlapsInterval(_fullKnotRegion, *_knotIt))
        {
            // We don't need to consult _initializedKnotItBackward
            // since this is the top-level iterator, and stepping
            // forward here always means stepping forward in time.
            ++_curIteration;
            _initializedKnotItBackward = false;
            initializedKnotIt = true;
            _UpdateKnotIterator();
        }
    }

    if ((_curIteration > _maxIteration ||
        (initializedKnotIt && _knotIt.AtEnd())) && !_postExtrapLooped)
    {
        _splineRegion = TsSourcePostExtrap;
    }

    _UpdateSegment();
    _atEnd = !_SegmentOverlapsInterval(_interval, _segment);
    return true;
}

bool
Ts_SegmentIterator::StepBackward()
{
    // We can't move if we don't have an initialization anchor time.
    if (_interval.IsEmpty()) {
        return false;
    }

    // If we're advancing out of pre-extrapolation, we're done.
    if (_splineRegion == TsSourcePreExtrap) {
        _atEnd = true;
        return false;
    }

    bool initializedKnotIt = false;
    if (_splineRegion == TsSourcePostExtrap) {
        _splineRegion = TsSourceKnotInterp;
        _curIteration = 0;
        _initializedKnotItBackward = true;
        initializedKnotIt = true;
        _UpdateKnotIterator();
    } else {
        const bool success =
            _initializedKnotItBackward ? _knotIt.StepForward()
                                        : _knotIt.StepBackward();
        if (!success ||
            !_SegmentOverlapsInterval(_fullKnotRegion, *_knotIt))
        {
            // Stepping backward here always means stepping backward
            // in time.
            --_curIteration;
            _initializedKnotItBackward = true;
            initializedKnotIt = true;
            _UpdateKnotIterator();
        }
    }

    if ((_curIteration < _minIteration ||
        (initializedKnotIt && _knotIt.AtEnd())) && !_preExtrapLooped)
    {
        _splineRegion = TsSourcePreExtrap;
    }

    _UpdateSegment();
    _atEnd = !_SegmentOverlapsInterval(_interval, _segment);
    return true;
}

/// Update the knot iterator appropriately.
void
Ts_SegmentIterator::_UpdateKnotIterator()
{
    if (_splineRegion == TsSourcePreExtrap ||
        _splineRegion == TsSourcePostExtrap)
    {
        // This region does not use _knotIt.
        return;
    }

    if (_curIteration > _maxIteration || _curIteration < _minIteration) {
        // Off the end. Reset _knotIt so it will be AtEnd().
        _knotIt = Ts_SegmentKnotIterator();
        return;
    }

    // Figure out the offset to shift the time interval for our
    // current iteration.
    double firstKnotTime = _firstKnotTime;
    double lastKnotTime = _lastKnotTime;
    if (_curIteration < 0) {
        lastKnotTime = _lastPreExtrapKnotTime;
    } else if (_curIteration > 0) {
        firstKnotTime = _firstPostExtrapKnotTime;
    }
    const double knotTimeSpan = lastKnotTime - firstKnotTime;
    const double timeDelta = _curIteration * knotTimeSpan;

    _fullKnotRegion = GfInterval(firstKnotTime, lastKnotTime, CLOSED, OPEN);

    // If we're reversing, we want to shift and flop the interval. Try to do this
    // with a minimal amount of operations to minimize round off error
    // accumulation.
    if (_curIteration < 0) {
        _oscillating = (_data->preExtrapolation.mode == TsExtrapLoopOscillate);
        _valueShift = (_data->preExtrapolation.mode == TsExtrapLoopRepeat
                       ? _curIteration *
                            (_lastPreExtrapKnotPreValue - _firstKnotPreValue)
                       : 0.0);
    } else if (_curIteration > 0) {
        _oscillating = (_data->postExtrapolation.mode == TsExtrapLoopOscillate);
        _valueShift = (_data->postExtrapolation.mode == TsExtrapLoopRepeat
                       ? _curIteration *
                            (_lastKnotValue - _firstPostExtrapKnotValue)
                       : 0.0);
    } else {
        _oscillating = false;
        _valueShift = 0.0;
    }

    const bool oscillateReversing = _oscillating && (_curIteration % 2 != 0);
    _reversing = _initializedKnotItBackward ^ oscillateReversing;
    if (oscillateReversing) {
        // We're reversed, flop things around.
        //
        // The math for the interval is the same as it would be for a simple
        // time value. Let T represent the input time interval.
        //
        // Shift the interval by timeDelta to map times in the current iteration
        // to [firstKnotTime .. lastKnotTime)
        //
        //    shifted_T = T - timeDelta;
        //
        // Shift it further by firstKnotTime to map it to [0 .. knotTimeSpan)
        //
        //    zeroed_T = shiftedT - firstKnotTime;
        //
        // Invert it to get (-knotTimeSpan .. 0]
        //
        //    inverted_T = -zeroedT;
        //
        // Now add knotTimeSpan to shift it to (0 .. knotTimeSpan] again but now
        // it is inverted. Further shift it by firstKnotTime to get the range
        // (firstKnotTime .. lastKnotTime] but now it's inverted.
        //
        // That last shift, (knotTimeSpan + firstKnotTime), simplifies to just
        // lastKnotTime, giving us:
        //
        //    unzeroed_T = invertedT + lastKnotTime;
        //
        // Finally, fix up the open/closed state of the interval to get:
        //
        //    new_T = [unzeroed_T.min .. unzeroedT.max);
        //
        // Putting this all together, we get
        //
        //    new_T = -(T - shift1) + shift2
        // where
        //    shift1 = timeDelta + firstKnotTime
        //    shift2 = lastKnotTime
        //
        // This gives us an interval that includes the knots we should iterate
        // over to generate our segments. Each segment will need to be mapped
        // back through this transformation to get the result segment.
        //
        _shift1 = timeDelta + firstKnotTime;
        _shift2 = lastKnotTime;
        _valueShift = 0.0;  // Oscillating loops never have a value offset
    } else {
        // For the non-reversed case, we can just subtract timeDelta.
        _shift1 = timeDelta;
        _shift2 = 0;
    }

    // Use the loop subregion as the knot iterator interval, with the end
    // open [closed, open) to avoid double-counting at loop boundaries.
    GfInterval knotIterInterval(firstKnotTime, lastKnotTime, CLOSED, OPEN);

    // Rewrite the "min" side of the interval with the min translated to
    // the valid basis of the segment knot iterator if the current knot
    // time span contains _interval.GetMin(). This ensures that the iterator
    // starts at the expected segment. All other sections use the full knot
    // interval, and _atEnd is calculated in _UpdateSegment. Note that we
    // don't need to rewrite the "min" side if the knot iterator is constructed
    // because of StepBackward().
    if (_interval.GetMin() >= (timeDelta + firstKnotTime) &&
        _interval.GetMin() < (timeDelta + firstKnotTime + knotTimeSpan) &&
        !_initializedKnotItBackward)
    {
        if (oscillateReversing) {
            const double t1 = -(_interval.GetMin() - _shift1) + _shift2;
            knotIterInterval.SetMax(t1, OPEN);
        } else {
            knotIterInterval.SetMin(_interval.GetMin() - _shift1);
        }
    }

    _knotIt = Ts_SegmentKnotIterator(_data, knotIterInterval, _reversing);
}

/// Update the segment after the iterators change.
void
Ts_SegmentIterator::_UpdateSegment()
{
    if (_splineRegion == TsSourcePreExtrap) {
        _UpdatePreExtrapSegment();
    } else if (_splineRegion == TsSourcePostExtrap) {
        _UpdatePostExtrapSegment();
    } else {
        _segment = *_knotIt;
        if (_oscillating && (_curIteration % 2 != 0)) {
            // Don't bother shifting the value, reversed loops never have a
            // value offset.
            _segment = -(_segment - _shift2) + _shift1;
        } else {
            // Shift both time and value.
            _segment += GfVec2d(_shift1, _valueShift);
        }
    }
}

void
Ts_SegmentIterator::_UpdatePreExtrapSegment()
{
    // We can only get here for non-looping extrapolation. All non-looping
    // extrapolation is a straight ray to infinity, we just need to determine
    // the slope and the end point.
    const TsExtrapolation& extrap = _data->preExtrapolation;
    TsExtrapMode mode = extrap.mode;

    // We have a case of extrapolation looping that's resolved to an extrap
    // region of held or value block. Set the mode accordingly.
    if (extrap.IsLooping()) {
        if (_firstKnotTime == _lastKnotTime) {
            mode = TsExtrapHeld;
        } else {
            if (TF_VERIFY(extrap.loopBoundaryTime.has_value(),
                          "_UpdatePreExtrapSegment should trigger with "
                          "looping extrap only when loopBoundaryTime has "
                          "a value."))
            {   
                mode = extrap.loopBoundaryTime.value() == _firstKnotTime
                    ? TsExtrapHeld
                    : TsExtrapValueBlock;
            } else {
                mode = TsExtrapValueBlock;
            }
        }
    }

    // The end point is easy.
    GfVec2d endPt(_firstKnotTime, _firstKnotPreValue);

    // The slope depends on the pre-extrapolation mode.
    double slope = 0.0;  // Covers value-block and held extrapolation
    Ts_SegmentInterp interp = (mode == TsExtrapValueBlock
                               ? Ts_SegmentInterp::ValueBlock
                               : Ts_SegmentInterp::PreExtrap);

    if (mode == TsExtrapSloped) {
        slope = _data->preExtrapolation.slope;
    }

    // Short circuit the simple cases here. If this is not linear, or if it
    // is but there's a discontinuity, then we're done.
    if (mode != TsExtrapLinear ||
        _firstKnotPreValue != _firstKnotValue)
    {
        _segment = Ts_Segment{{-inf, slope},
                              {0.0, 0.0},
                              {0.0, 0.0},
                              endPt,
                              interp};
        return;
    }

    // Assuming there's no discontinuity, linear extrapolation uses the same
    // slope as the spline at the first knot. Determining that derivative can be
    // tricky, but we've got a Ts_SegmentKnotIterator that can give us the first
    // knot segment.
    Ts_SegmentKnotIterator tmpKnotIt(_data,
                                     GfInterval(_firstKnotTime, _lastKnotTime),
                                     /*reverse = */false);
    Ts_Segment firstSeg;
    if (tmpKnotIt.AtEnd()) {
        // There was no first knot segment. Slope is 0.0;
        slope = 0.0;
    } else {
        firstSeg= *tmpKnotIt;
        slope = firstSeg._ComputeDerivative(/*u = */0.0);
    }

    _segment = Ts_Segment{{-inf, slope},
                          {0.0, 0.0},
                          {0.0, 0.0},
                          endPt,
                          Ts_SegmentInterp::PreExtrap};
}

void
Ts_SegmentIterator::_UpdatePostExtrapSegment()
{
    // We can only get here for non-looping extrapolation. All non-looping
    // extrapolation is a straight ray to infinity, we just need to determine
    // the slope and the end point.
    const TsExtrapolation& extrap = _data->postExtrapolation;
    TsExtrapMode mode = extrap.mode;

    // We have a case of extrapolation looping that's resolved to an extrap
    // region of held or value block. Set the mode accordingly.
    if (extrap.IsLooping()) {
        if (_firstKnotTime == _lastKnotTime) {
            mode = TsExtrapHeld;
        } else {
            if (TF_VERIFY(extrap.loopBoundaryTime.has_value(),
                          "_UpdatePostExtrapSegment should trigger with "
                          "looping extrap only when loopBoundaryTime has "
                          "a value."))
            {   
                mode = extrap.loopBoundaryTime.value() == _lastKnotTime
                    ? TsExtrapHeld
                    : TsExtrapValueBlock;
            } else {
                mode = TsExtrapValueBlock;
            }
        }
    }

    // The end point is easy.
    GfVec2d endPt(_lastKnotTime, _lastKnotValue);

    // The slope depends on the post-extrapolation mode.
    double slope = 0.0;  // Covers value-block and held extrapolation
    Ts_SegmentInterp interp = (mode == TsExtrapValueBlock
                               ? Ts_SegmentInterp::ValueBlock
                               : Ts_SegmentInterp::PostExtrap);

    if (mode == TsExtrapSloped) {
        slope = _data->postExtrapolation.slope;
    }

    // Short circuit the simple cases here. If this is not linear, or if it
    // is but there's a discontinuity, then we're done.
    if (mode != TsExtrapLinear ||
        _lastKnotPreValue != _lastKnotValue)
    {
        _segment = Ts_Segment{endPt,
                              {0.0, 0.0},
                              {0.0, 0.0},
                              {+inf, slope},
                              interp};
        return;
    }

    // Assuming there's no discontinuity, linear extrpolation uses the same
    // slope as the spline at the last knot. Determining that derivative can be
    // tricky, but we've got a Ts_SegmentKnotIterator that can give us the last
    // knot segment.
    Ts_SegmentKnotIterator tmpKnotIt(_data,
                                     GfInterval(_firstKnotTime, _lastKnotTime),
                                     /*reverse = */true);
    Ts_Segment lastSeg;
    if (tmpKnotIt.AtEnd()) {
        // There was no last knot segment. Slope is 0.0;
        slope = 0.0;
    } else {
        lastSeg= *tmpKnotIt;
        slope = lastSeg._ComputeDerivative(/*u = */1.0);
    }

    _segment = Ts_Segment{endPt,
                          {0.0, 0.0},
                          {0.0, 0.0},
                          {+inf, slope},
                          Ts_SegmentInterp::PostExtrap};
}

PXR_NAMESPACE_CLOSE_SCOPE
