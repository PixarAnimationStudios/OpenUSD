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

// Utility to convert a TsInterpMode value into a Ts_SegmentInterp. Because the
// TsInterpMode just includes a generic "curve" interpolation, we also need the
// spline's curve type to know if this segment is a bezier or hermite curve.
Ts_SegmentInterp _ConvertInterpMode(TsInterpMode interpMode,
                                    TsCurveType curveType)
{
    switch (interpMode)
    {
      case TsInterpValueBlock: return Ts_SegmentInterp::ValueBlock;
      case TsInterpHeld: return Ts_SegmentInterp::Held;
      case TsInterpLinear: return Ts_SegmentInterp::Linear;
      case TsInterpCurve:
        if (curveType == TsCurveTypeHermite) {
            return Ts_SegmentInterp::Hermite;
        } else {
            return Ts_SegmentInterp::Bezier;
        }
    }
    // Generate a nicer error message.
    constexpr bool invalid_TsInterpMode_value = false;
    TF_AXIOM(invalid_TsInterpMode_value);

    return Ts_SegmentInterp::ValueBlock;
}

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

}  // end anonymous namespace

////////////////////////////////////////////////////////////////
// Ts_Segment

// This computes a derivative, but only for u == 0 or u == 1 for curved
// segments. Fortunately, this is all that's required for linear extrapolation.
double
Ts_Segment::_ComputeDerivative(double u) const
{
    u = std::clamp(u, 0.0, 1.0);

    // It's possible to not have a derivative due to value blocks. But we're
    // going to return 0.0 in that case just to keep things simple. The result
    // will be a value block with slope of 0.0. :-)
    switch (interp)
    {
      case Ts_SegmentInterp::ValueBlock:
      case Ts_SegmentInterp::Held:
        return 0.0;

      case Ts_SegmentInterp::Linear:
        return (p1[1] - p0[1]) / (p1[0] - p0[0]);

      case Ts_SegmentInterp::PreExtrap:
        return p0[1];

      case Ts_SegmentInterp::PostExtrap:
        return p1[1];

      case Ts_SegmentInterp::Bezier:
      case Ts_SegmentInterp::Hermite:
        TF_VERIFY(u == 0.0 || u == 1.0,
                  "Cannot yet compute derivatives at arbitrary values. u = %g",
                  u);

        // Not a generalized solution right now, but it works for 0 and 1
        if (u <= 0.5) {
            // slope is equal to first tangent direction.
            GfVec2d tangent = t0 - p0;
            return tangent[1] / tangent[0];
        } else if (u > 0.5) {
            // slope is equal to the last tangent direction. Note that
            // this direction is measured from t1 to p1.
            GfVec2d tangent = p1 - t1;
            return tangent[1] / tangent[0];
        }

      default:
        TF_CODING_ERROR("Invalid segment interp (%d) in _ComputeDerivative",
                        int(interp));
    }

    return 0.0;
}

// Output operator for testing purposes.
std::ostream& operator <<(std::ostream& out, const Ts_SegmentInterp interp)
{
    switch (interp)
    {
      case Ts_SegmentInterp::ValueBlock:
        return out << "Ts_SegmentInterp::ValueBlock";

      case Ts_SegmentInterp::Held:
        return out << "Ts_SegmentInterp::Held";

      case Ts_SegmentInterp::Linear:
        return out << "Ts_SegmentInterp::Linear";

      case Ts_SegmentInterp::Bezier:
        return out << "Ts_SegmentInterp::Bezier";

      case Ts_SegmentInterp::Hermite:
        return out << "Ts_SegmentInterp::Hermite";

      case Ts_SegmentInterp::PreExtrap:
        return out << "Ts_SegmentInterp::PreExtrap";

      case Ts_SegmentInterp::PostExtrap:
        return out << "Ts_SegmentInterp::PostExtrap";
    }

    return out << "Ts_SegmentInterp(" << int(interp) << ")";
}

// Output operators for testing purposes.
std::ostream& operator <<(std::ostream& out, const Ts_Segment& seg)
{
    return out << "Ts_Segment{"
               << "{" << seg.p0[0] << ", " << seg.p0[1] << "}, "
               << "{" << seg.t0[0] << ", " << seg.t0[1] << "}, "
               << "{" << seg.t1[0] << ", " << seg.t1[1] << "}, "
               << "{" << seg.p1[0] << ", " << seg.p1[1] << "}, "
               << seg.interp << "}";
}

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
    if (_atEnd) {
        _segment = Ts_Segment();
        return;
    }

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
    _segment.interp = _ConvertInterpMode(prevKnot.nextInterp, _data->curveType);
}

Ts_Segment Ts_SegmentPrototypeIterator::operator *() const
{
    return _segment;
}

Ts_SegmentPrototypeIterator& Ts_SegmentPrototypeIterator::operator ++()
{
    if (_atEnd) {
        return *this;
    }

    const auto& times = _data->times;
    const auto& lp = _data->loopParams;

    if (_reversed) {
        // If we cannot decrement the iterator or if it already points outside
        // the prototype, set _atEnd to true.
        if (_timesIt == times.begin() ||
            _timesIt == times.end() ||
            *_timesIt <= lp.protoStart ||
            *_timesIt <= _interval.GetMin())
        {
            _atEnd = true;
        }

        if (!_atEnd) {
            --_timesIt;
        }
    } else {
        if (!_atEnd) {
            ++_timesIt;
            _atEnd = (_timesIt == times.end() ||
                      *_timesIt >= lp.protoEnd ||
                      !_interval.Contains(*_timesIt));
        }
    }

    _UpdateSegment();

    return *this;
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

    // Offset from the time this iterator is being asked for and the time
    // to pass to the Ts_SegmentPrototypeIterator.
    double timeDelta = _curIteration * _protoSpan;

    // Calculate the time to use for the protoType iterator;
    GfInterval protoIterInterval = iterInterval - GfInterval(timeDelta);

    _protoIt = Ts_SegmentPrototypeIterator(_data,
                                           protoIterInterval,
                                           _reversed);

    // Update _segment based on _protoIt
    _UpdateSegment();
    _atEnd = _protoIt.AtEnd();
}

void
Ts_SegmentLoopIterator::_UpdateSegment()
{
    if (_atEnd) {
        _segment = Ts_Segment();
    } else {
        _segment = *_protoIt;
        GfVec2d delta(_curIteration * _protoSpan, _curIteration * _valueOffset);
        _segment += delta;
    }
}

void
Ts_SegmentLoopIterator::_UpdatePrototypeIterator()
{
    GfInterval loopedInterval = _GetOpenLoopedInterval(_data->loopParams);

    const GfInterval iterInterval = _interval & loopedInterval;
    const double timeDelta = _curIteration * _protoSpan;
    const GfInterval protoIterInterval = iterInterval - GfInterval(timeDelta);

    _protoIt = Ts_SegmentPrototypeIterator(_data,
                                           protoIterInterval,
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
    if (_protoIt.AtEnd()) {
        _atEnd = true;
    } else {
        ++_protoIt;
        if (_protoIt.AtEnd()) {
            // Hit end of a prototype loop.
            _curIteration += (_reversed ? -1 : 1);

            _UpdatePrototypeIterator();

            if (_protoIt.AtEnd()) {
                // We really are at end.
                _atEnd = true;
            }
        }
    }

    _UpdateSegment();

    return *this;
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
                _knotSection = PreInnerLooping;
            } else if (_interval.GetMax() > _loopedInterval.GetMin()) {
                // We're starting iterations in the middle of the inner looped
                // section.
                _knotSection = InnerLooping;
            } else if (_interval.GetMax() > times.front()) {
                // We're starting iterations (in reverse) before the inner looping.
                _knotSection = PostInnerLooping;
            } else {
                // We're starting (in reverse) iterations before the start of the knots.
                _knotSection = PostInnerLooping;
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
    if (_atEnd) {
        _segment = Ts_Segment();
        return;
    }

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
    if (_hasInnerLoops &&
        ((_knotSection == PreInnerLooping && _reversed) ||
         (_knotSection == PostInnerLooping && !_reversed)) &&
        (times[prevIndex] <= _loopedInterval.GetMax()))
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
    if (_hasInnerLoops &&
        ((_knotSection == PreInnerLooping && !_reversed) ||
         (_knotSection == PostInnerLooping && _reversed)) &&
        (times[nextIndex] >= _loopedInterval.GetMin()))
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
    _segment.t1 = _segment.p1 -
                  GfVec2d(nextKnot.preTanWidth,
                          nextKnot.preTanWidth * nextKnot.preTanSlope);
    _segment.interp = _ConvertInterpMode(prevKnot.nextInterp, _data->curveType);
}

Ts_Segment
Ts_SegmentKnotIterator::operator *() const
{
    return _segment;
}

Ts_SegmentKnotIterator&
Ts_SegmentKnotIterator::operator ++()
{
    if (_atEnd) {
        // Don't go beyond the end.
        return *this;
    }

    // Save some typing
    const auto& times = _data->times;

    if (_knotSection == InnerLooping) {
        ++_loopIt;
        if (_loopIt.AtEnd()) {
            // We've run off the end of the inner looped section. Have we
            // also run off the iteration interval? Check _segment which
            // still has the last iteration in it.
            if ((_reversed && _segment.p0[0] <= _interval.GetMin()) ||
                (!_reversed && _segment.p1[0] >= _interval.GetMax()))
            {
                _atEnd = true;
            } else {
                // Move to PostInnerLooping and set _timesIt correctly. This is
                // essentially the same algorithm used in the constructor.
                _knotSection = PostInnerLooping;

                // XXX: There are slight variations on this if/else block in the
                // prototype, the knot, and the segment iterators. It might be
                // nice to unify them one day.
                if (_reversed) {
                    // Find the start of the segment we're interested in.
                    _timesIt = std::lower_bound(times.begin(), times.end(),
                                                _loopedInterval.GetMin());
                    if (_timesIt == times.begin()) {
                        _atEnd = true;
                    } else {
                        --_timesIt;
                    }
                } else {
                    _timesIt = std::upper_bound(times.begin(), times.end(),
                                                _loopedInterval.GetMax());
                    if (_timesIt == times.end()) {
                        _atEnd = true;
                    } else {
                        --_timesIt;
                    }
                }
            }
        }
    } else {
        if (_reversed) {
            if (_timesIt == times.begin() ||
                _timesIt == times.end() ||
                !_interval.Contains(*_timesIt))
            {
                _atEnd = true;
            }
            if (!_atEnd) {
                --_timesIt;
                if (_knotSection == PreInnerLooping &&
                    *_timesIt < _loopedInterval.GetMax())
                {
                    // We have just entered the inner looping section.
                    _knotSection = InnerLooping;
                    _loopIt = Ts_SegmentLoopIterator(_data,
                                                     _interval,
                                                     _reversed);
                }
            }
        } else {
            if (!_atEnd) {
                ++_timesIt;

                _atEnd = (_timesIt == times.end() ||
                          !_interval.Contains(*_timesIt));
            }
            if (!_atEnd &&
                _knotSection == PreInnerLooping &&
                *_timesIt >= _loopedInterval.GetMin())
            {
                // We have just entered the inner looping section.
                _knotSection = InnerLooping;
                _loopIt = Ts_SegmentLoopIterator(_data,
                                                 _interval,
                                                 _reversed);
            }
        }
    }

    _UpdateSegment();

    return *this;
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

    // The test (_firstKnotTime < _lastKnotTime) is true if there is more than
    // one knot (generated by inner looping or not) in the spline. We can only
    // have extrapolation looping if there is more than one knot to loop over.
    _preExtrapLooped = (_firstKnotTime < _lastKnotTime) &&
                       _data->preExtrapolation.IsLooping();
    _postExtrapLooped = (_firstKnotTime < _lastKnotTime) &&
                        _data->postExtrapolation.IsLooping();
    if ((_preExtrapLooped && !_interval.IsMinFinite()) ||
        (_postExtrapLooped && !_interval.IsMaxFinite()))
    {
        TF_CODING_ERROR("Cannot iterate an infinitely looping spline across"
                        " an infinite time interval.");
        _atEnd = true;
        return;
    }

    if (_preExtrapLooped || _postExtrapLooped) {
        const double knotSpan = _lastKnotTime - _firstKnotTime;
        _minIteration =
            int32_t(std::floor((_interval.GetMin() - _firstKnotTime) / knotSpan));
        _maxIteration =
            int32_t(std::floor((_interval.GetMax() - _firstKnotTime) / knotSpan));

        // If not pre-extrapolation looping, do not allow negative iterations.
        if (!_preExtrapLooped) {
            _minIteration = std::max(_minIteration, 0);
            _maxIteration = std::max(_maxIteration, 0);
        }

        // If not post-extrapolation looping, do not allow positive iterations.
        if (!_postExtrapLooped) {
            _minIteration = std::min(_minIteration, 0);
            _maxIteration = std::min(_maxIteration, 0);
        }
    } else {
        _minIteration = _maxIteration = 0;
    }

    _curIteration = _minIteration;

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

Ts_Segment
Ts_SegmentIterator::operator *() const
{
    return _segment;
}

Ts_SegmentIterator&
Ts_SegmentIterator::operator ++()
{
    if (_atEnd) {
        return *this;
    }

    if (_splineRegion == TsSourcePostExtrap) {
        // We're advancing out of post-extrapolation, we're done.
        _atEnd = true;
    } else {
        if (_splineRegion == TsSourcePreExtrap) {
            // Advancing out of pre-extrapolation. Assume we're advancing
            // to knot interpolation until proven otherwise.
            _splineRegion = TsSourceKnotInterp;
            _curIteration = 0;
            // It's possible that there's only one knot and we're going to
            // advance straight to post-extrap. Update the iterator here, if it
            // starts AtEnd() then there are no knot segments.
            _UpdateKnotIterator();
        } else {
            ++_knotIt;
        }

        if (_knotIt.AtEnd()) {
            // This is the end of a loop. Move to the next iteration if
            // there is one.
            ++_curIteration;
            _UpdateKnotIterator();
        }

        if (_knotIt.AtEnd()) {
            // Done with iterations, see if we need a postExtrap segment.
            if (!_postExtrapLooped && _interval.GetMax() > _lastKnotTime) {
                _splineRegion = TsSourcePostExtrap;
            } else {
                _atEnd = true;
            }
        }
    }

    _UpdateSegment();
    return *this;
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

    if (_curIteration > _maxIteration) {
        // Off the end. Reset _knotIt so it will be AtEnd().
        _knotIt = Ts_SegmentKnotIterator();
        return;
    }

    // Figure out the offset to shift the time interval for our
    // current iteration.
    const double knotTimeSpan = _lastKnotTime - _firstKnotTime;
    const double timeDelta = _curIteration * knotTimeSpan;

    // Shift the input _interval an appropriate amount for iterating over the
    // knots themselves. Normally this is shifting by timeDelta, but if we're
    // oscillating in reverse, we need to flop that knot interval.
    GfInterval knotIterInterval;

    // If we're reversing, we want to shift and flop the interval. Try to do this
    // with a minimal amount of operations to minimize round off error
    // accumulation.
    if (_curIteration < 0) {
        _oscillating = (_data->preExtrapolation.mode == TsExtrapLoopOscillate);
        _valueShift = (_data->preExtrapolation.mode == TsExtrapLoopRepeat
                       ? _curIteration * (_lastKnotValue - _firstKnotValue)
                       : 0.0);
    } else if (_curIteration > 0) {
        _oscillating = (_data->postExtrapolation.mode == TsExtrapLoopOscillate);
        _valueShift = (_data->postExtrapolation.mode == TsExtrapLoopRepeat
                       ? _curIteration * (_lastKnotValue - _firstKnotValue)
                       : 0.0);
    } else {
        _oscillating = false;
        _valueShift = 0.0;
    }
    _reversing = _oscillating && (_curIteration % 2 != 0);
    if (_reversing) {
        // We're reversed, flop things around.
        //
        // The math for the interval is the same as it would be for a simple
        // time value. Let T represent the input time interval.
        //
        // Shift the interval by timeDelta to map times in the current iteration
        // to [_firstKnotTime .. _lastKnotTime)
        //
        //    shifted_T = T - timeDelta;
        //
        // Shift it further by _firstKnotTime to map it to [0 .. knotTimeSpan)
        //
        //    zeroed_T = shiftedT - _firstKnotTime;
        //
        // Invert it to get (-knotTimeSpan .. 0]
        //
        //    inverted_T = -zeroedT;
        //
        // Now add knotTimeSpan to shift it to (0 .. knotTimeSpan] again but now
        // it is inverted. Further shift it by _firstKnotTime to get the range
        // (_firstKnotTime .. _lastKnotTime] but now it's inverted.
        //
        // That last shift, (knotTimeSpan + _firstKnotTime), simplifies to just
        // _lastKnotTime, giving us:
        //
        //    unzeroed_T = invertedT + _lastKnotTime;
        //
        // Finally, fix up the open/closed state of the interval to get:
        //
        //    new_T = [unzeroed_T.min .. unzeroedT.max);
        //
        // Putting this all together, we get
        //
        //    new_T = -(T - shift1) + shift2
        // where
        //    shift1 = timeDelta + _firstKnotTime
        //    shift2 = _lastKnotTime
        //
        // This gives us an interval that includes the knots we should iterate
        // over to generate our segments. Each segment will need to be mapped
        // back through this transformation to get the result segment.
        //
        _shift1 = timeDelta + _firstKnotTime;
        _shift2 = _lastKnotTime;
        _valueShift = 0.0;  // Oscillating loops never have a value offset
        const double t1 = -(_interval.GetMin() - _shift1) + _shift2;
        const double t0 = -(_interval.GetMax() - _shift1) + _shift2;
        knotIterInterval = GfInterval(t0, t1, CLOSED, OPEN);
    } else {
        // For the non-reversed case, we can just subtract timeDelta.
        _shift1 = timeDelta;
        _shift2 = 0;
        knotIterInterval = _interval - GfInterval(_shift1);
    }

    _knotIt = Ts_SegmentKnotIterator(_data, knotIterInterval, _reversing);
    _atEnd = _knotIt.AtEnd();
}

/// Update the segment after the iterators change.
void
Ts_SegmentIterator::_UpdateSegment()
{
    if (_atEnd) {
        _segment = Ts_Segment();
    } else if (_splineRegion == TsSourcePreExtrap) {
        _UpdatePreExtrapSegment();
    } else if (_splineRegion == TsSourcePostExtrap) {
        _UpdatePostExtrapSegment();
    } else {
        if (_knotIt.AtEnd()) {
            _atEnd = true;
            _segment = Ts_Segment();
        } else {
            _segment = *_knotIt;
            if (_reversing) {
                // Don't bother shifting the value, reversed loops never have a
                // value offset.
                _segment = -(_segment - _shift2) + _shift1;
            } else {
                // Shift both time and value.
                _segment += GfVec2d(_shift1, _valueShift);
            }
        }
    }
}

void
Ts_SegmentIterator::_UpdatePreExtrapSegment()
{
    // We can only get here for non-looping extrapolation. All non-looping
    // extrapolation is a straight ray to infinity, we just need to determine
    // the slope and the end point.
    TsExtrapMode mode = _data->preExtrapolation.mode;

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
    TsExtrapMode mode = _data->postExtrapolation.mode;

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
