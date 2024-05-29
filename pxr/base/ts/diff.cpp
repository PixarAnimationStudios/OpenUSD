//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/diff.h"

#include "pxr/base/ts/keyFrameUtils.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/types.h"

#include "evalUtils.h"

#include "pxr/base/gf/interval.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/vt/value.h"

#include <limits>

PXR_NAMESPACE_OPEN_SCOPE

//
// FindChangedInterval
//

namespace {

// Helper class for taking two splines and computing the GfInterval in which
// they differ from each other evaluatively.
class Ts_SplineChangedIntervalHelper
{
    typedef TsSpline::const_iterator KeyFrameIterator;
    typedef TsSpline::const_reverse_iterator KeyFrameReverseIterator;

public:
    Ts_SplineChangedIntervalHelper(
        const TsSpline *s1, const TsSpline *s2);
    GfInterval ComputeChangedInterval();

private:
    // Helper functions for tightening the changed interval from the left
    static KeyFrameReverseIterator _GetFirstKeyFrame(
        const TsSpline& spline, KeyFrameReverseIterator kf);
    KeyFrameIterator _GetNextNonFlatKnot(
        const TsSpline &spline, const KeyFrameIterator &startKeyFrame);
    bool _TightenToNextKeyFrame(bool extrapolateHeldLeft = false);
    void _TightenFromLeft();

    // Helper functions for tightening the changed interval from the right
    static KeyFrameIterator _GetLastKeyFrame(
        const TsSpline& spline, KeyFrameIterator kf);
    KeyFrameReverseIterator _GetPreviousNonFlatKnot(
        const TsSpline &spline, const KeyFrameReverseIterator &startKeyFrame);
    bool _TightenToPreviousKeyFrame(bool extrapolateHeldRight = false);
    void _TightenFromRight();

    const TsSpline *_s1;
    const TsSpline *_s2;
    KeyFrameIterator _s1Iter;
    KeyFrameIterator _s2Iter;
    KeyFrameReverseIterator _s1ReverseIter;
    KeyFrameReverseIterator _s2ReverseIter;

    GfInterval _changedInterval;
};

}


Ts_SplineChangedIntervalHelper::Ts_SplineChangedIntervalHelper(
    const TsSpline *s1, const TsSpline *s2):
    _s1(s1),
    _s2(s2)
{
}

GfInterval
Ts_SplineChangedIntervalHelper::ComputeChangedInterval()
{
    TRACE_FUNCTION();

    // First assume everything changed.
    _changedInterval = GfInterval::GetFullInterval();

    // If both splines are empty then splines aren't different so just return
    // the empty interval.
    if (_s1->empty() && _s2->empty()) {
        _changedInterval = GfInterval();
        return _changedInterval;
    }
    // If either spline is empty then just return the entire interval.
    if (_s1->empty() || _s2->empty()) {
        return _changedInterval;
    }

    // Try to tighten interval from right side
    _TightenFromRight();

    if (!_changedInterval.IsEmpty()) {
        // Try to tighten interval from left side
        _TightenFromLeft();
    }

    if (_changedInterval.IsEmpty()) {
        _changedInterval = GfInterval();
    }

    return _changedInterval;
}

// Return the iterator representing the last keyframe: if extrapolating, return 
// end(); otherwise, return the last keyframe
Ts_SplineChangedIntervalHelper::KeyFrameIterator 
Ts_SplineChangedIntervalHelper::_GetLastKeyFrame(
    const TsSpline &spline,
    KeyFrameIterator kf)
{
    TF_VERIFY(kf+1 == spline.end());

    if (Ts_GetEffectiveExtrapolationType(*kf, spline, TsRight)
        == TsExtrapolationHeld) {
        return spline.end();
    } else {
        return kf;
    }
}

// This function finds the first key frame after or including startKeyFrame 
// that is not part of a constant flat spline segment starting at 
// startKeyFrame's right side value.  
Ts_SplineChangedIntervalHelper::KeyFrameIterator
Ts_SplineChangedIntervalHelper::_GetNextNonFlatKnot(
    const TsSpline &spline, 
    const KeyFrameIterator &startKeyFrame)
{
    TRACE_FUNCTION();

    // Start off assuming the next non flat key frame is the one we passed in.
    KeyFrameIterator kf = startKeyFrame;
    VtValue prevHeldValue;

    // For array-valued spline, assume the next knot is non-flat. 
    // This is primarily an optimization to address expensive comparisons 
    // required by the loop below for large arrays with large identical prefixes
    // between adjacent knots (which is often the case for animation data).
    // The outer loop calling this function will still iterate over each knot 
    // to tighten the invalidation interval.
    if (kf == spline.end()) {
        return kf;
    } else if (kf->GetValue().IsArrayValued()) {
        ++kf;

        // If startKeyFrame is the last key frame, check the extrapolation to
        // the right
        if (kf == spline.end()) {
            return _GetLastKeyFrame(spline, startKeyFrame);
        } else {
            return kf;
        }
    }

    while (kf != spline.end()) {
        // With the exception of the key frame we're starting with, check for
        // for value consistency from the left side to the right side
        if (kf != startKeyFrame) {
            // A dual valued not with different values means this key frame
            // is the next non flat knot
            if (kf->GetIsDualValued() && 
                kf->GetLeftValue() != kf->GetValue()) {
                return kf;
            }
            // If the previous knot was held and this knot's right value is 
            // different than the held value, then this key frame is the next
            // non-flat one.
            if (!prevHeldValue.IsEmpty() && 
                kf->GetValue() != prevHeldValue) {
                return kf;
            }
        }
        // If this key frame is held, then we're automatically flat until the
        // next key frame so skip to it.  We specifically check the held case
        // instead of using Ts_IsSegmentFlat as Ts_IsSegmentFlat requires
        // that the next knot's left value be the same for flatness while this
        // function does not.
        if (kf->GetKnotType() == TsKnotHeld) {
            // Store the held value so we can compare it.
            prevHeldValue = kf->GetValue();
            ++kf;
            continue;
        } else {
            // Clear the previous held value if we're not held.
            prevHeldValue = VtValue();
        }

        // Get the next key frame
        KeyFrameIterator nextKeyFrameIt = kf;
        ++nextKeyFrameIt;

        // If we're looking at the last key frame, then check the extrapolation
        // to the right
        if (nextKeyFrameIt == spline.end()) {
            return _GetLastKeyFrame(spline, kf);
        }

        // If the segment from this key frame to the next one is not flat,
        // then this key frame is the next non flat one.
        if (!Ts_IsSegmentFlat(*kf, *nextKeyFrameIt)) {
            return kf;
        }

        // We passed all the flatness conditions so move to the next key frame.
        ++kf;
    }

    return kf;
}

// Tightens the left side of the changed interval up to the next key frame if 
// possible and returns whether the interval can potentially be tightened 
// any more.
bool 
Ts_SplineChangedIntervalHelper::_TightenToNextKeyFrame(
    bool extrapolateHeldLeft)
{
    TRACE_FUNCTION();
    const TsTime infinity = std::numeric_limits<TsTime>::infinity();

    // By default assume we can't tighten the interval beyond the next 
    // key frame.
    bool canTightenMore = false;

    // If we're holding extrapolation for the left, then we can only tighten
    // if the left side values of the current key frames are equal.
    if (extrapolateHeldLeft) {
        if (_s1Iter->GetLeftValue() != _s2Iter->GetLeftValue()) {
            return false;
        }
    }

    // First find the next non flat knots from the current key frames.
    KeyFrameIterator s1NextNonFlat = _s1Iter;
    KeyFrameIterator s2NextNonFlat = _s2Iter;

    // If we're held extrapolating from the left (meaning this is the first 
    // left side tightening) but the key frame is dual valued and sides don't 
    // match in value, then we know that the next flat knot is the first knot
    // in this case.  In this instance we don't want to get the next non flat
    // knot as that function ignores anything on the left side of the initial
    // knot.  For all other cases, we just get the next non flat knot as normal
    // since we will have already covered the left side of the current knot.
    if (!extrapolateHeldLeft ||
        !_s1Iter->GetIsDualValued() || 
        _s1Iter->GetValue() == _s1Iter->GetLeftValue()) {
        s1NextNonFlat = _GetNextNonFlatKnot(*_s1, _s1Iter);
    }
    if (!extrapolateHeldLeft ||
        !_s2Iter->GetIsDualValued() ||
        _s2Iter->GetValue() == _s2Iter->GetLeftValue()) {
        s2NextNonFlat = _GetNextNonFlatKnot(*_s2, _s2Iter);
    }

    // If we're extrapolating held from the left or we found flat segments of
    // the same value on both splines, then we can do the flat segment
    // interval tightening.
    if (extrapolateHeldLeft ||
        (s1NextNonFlat != _s1Iter && s2NextNonFlat != _s2Iter &&
         _s1Iter->GetValue() == _s2Iter->GetValue())) {

        // Get the times of the end of the flat segment (could be infinity if
        // the spline is flat all the way past the last key frame).
        TsTime s1NextKfTime = 
            (s1NextNonFlat == _s1->end()) ? infinity : s1NextNonFlat->GetTime();
        TsTime s2NextKfTime = 
            (s2NextNonFlat == _s2->end()) ? infinity : s2NextNonFlat->GetTime();

        // At this point we know we're tightening the interval from the left,
        // we still need to determine if beginning of the interval should be
        // closed or open and whether we can potentially continue tightening
        // from the left.
        bool closed = false;
        if (s1NextKfTime < s2NextKfTime)
        {
            // If s1's flat segment ends before s2's then the interval is 
            // closed if either side of s1's key frame differs from the held
            // segment's value.
            closed = s1NextNonFlat->GetValue() != _s2Iter->GetValue() ||
                (s1NextNonFlat->GetIsDualValued() &&
                 s1NextNonFlat->GetValue() != s1NextNonFlat->GetLeftValue());
        } 
        else if (s2NextKfTime < s1NextKfTime) 
        {
            // If s2's flat segment ends before s1's then the interval is 
            // closed if either side of s2's key frame differs from the held
            // segment's value.
            closed = s2NextNonFlat->GetValue() != _s1Iter->GetValue() ||
                (s2NextNonFlat->GetIsDualValued() &&
                 s2NextNonFlat->GetValue() != s2NextNonFlat->GetLeftValue());
        } 
        else // (s2NextKfTime == s1NextKfTime)
        {   
            // Otherwise both spline's flat segments end at the same time.

            // If the splines are flat to the end, then there is no 
            // evaluative difference between the two.  The changed interval
            // is empty.
            if (s1NextKfTime == infinity) {
                _changedInterval = GfInterval();
                return false;
            }
            // The interval is closed if either the splines don't match values
            // on either side of the keyframe.
            closed = s1NextNonFlat->GetValue() != s2NextNonFlat->GetValue() || 
                s1NextNonFlat->GetLeftValue() != s2NextNonFlat->GetLeftValue();
            // We can only potentially tighten more if the key frames have
            // equivalent values on both sides.
            canTightenMore = !closed;
        }

        // Update the changed interval with the new min value.
        _changedInterval.SetMin(GfMin(s1NextKfTime, s2NextKfTime), closed);

        // Update the forward iterators to the end of the flat segments we
        // just checked.
        _s1Iter = s1NextNonFlat;
        _s2Iter = s2NextNonFlat;
    }

    // Otherwise we're not looking at a flat segment so just do a standard
    // segment equivalence check.
    else { 
        // First make sure the right sides of the current key frames are 
        // equivalent.
        if (_s1Iter->IsEquivalentAtSide(*_s2Iter, TsRight)) {
            // Move to the next key frames and check if they're left equivalent.
            ++_s1Iter;
            ++_s2Iter;
            if (_s1Iter != _s1->end() && _s2Iter != _s2->end() &&
                _s1Iter->IsEquivalentAtSide(*_s2Iter, TsLeft)) {

                // Compare the right side values to determine if the interval
                // should be closed
                bool closed = (_s1Iter->GetValue() != _s2Iter->GetValue());
                _changedInterval.SetMin(_s1Iter->GetTime(), closed);
                // We can continue tightening if the knots are right equivalent.
                canTightenMore = !closed;
            } 
        }
    }

    return canTightenMore;
}

void
Ts_SplineChangedIntervalHelper::_TightenFromLeft()
{
    TRACE_FUNCTION();

    // Initialize the iterators to the first key frame in each spline
    _s1Iter = _s1->begin();
    _s2Iter = _s2->begin();

    // Get the effective extrapolations of each spline on the left side
    const TsExtrapolationType aExtrapLeft = 
        Ts_GetEffectiveExtrapolationType(*_s1Iter, *_s1, TsLeft);
    const TsExtrapolationType bExtrapLeft = 
        Ts_GetEffectiveExtrapolationType(*_s2Iter, *_s2, TsLeft);

    // We can't tighten if the extrapolations or the extrapolated values are
    // different.
    if (aExtrapLeft != bExtrapLeft ||
        _s1Iter->GetLeftValue() != _s2Iter->GetLeftValue()) {
        return;
    }

    // If the extrapolation is held then tighten to the next key frame
    // with left held extrapolation.
    if (aExtrapLeft == TsExtrapolationHeld) {
        if (!_TightenToNextKeyFrame(true /*extrapolateHeldLeft*/)) {
            // If we can't continue tightening then return.
            return;
        }
    }
    // Otherwise the extrapolation is linear so only if the time and
    // slopes match, do we not have a change before the first keyframes
    // XXX: We could potentially improve upon how much we invalidate in
    // the linear extrapolation case but it may not be worth it at this
    // time.
    else if (_s1Iter->GetTime() == _s2Iter->GetTime() &&
             _s1Iter->GetLeftTangentSlope() == 
             _s2Iter->GetLeftTangentSlope()) {
        bool closed = _s1Iter->GetValue() != _s2Iter->GetValue();
        _changedInterval.SetMin(_s1Iter->GetTime(), closed );
        // If the interval is closed, then we can't tighten any more so 
        // just return.
        if (closed) {
            return;
        }
    } else {
        // Otherwise we our extrapolations are not tightenable so just
        // return.
        return;
    }

    // Now just continue tightening the interval to the next key frame
    // until we can no longer do so.
    while(_TightenToNextKeyFrame());
}

// Return the iterator representing the last keyframe: if extrapolating, return 
// end(); otherwise, return the last keyframe
Ts_SplineChangedIntervalHelper::KeyFrameReverseIterator 
Ts_SplineChangedIntervalHelper::_GetFirstKeyFrame(
    const TsSpline &spline, 
    KeyFrameReverseIterator kf)
{
    TF_VERIFY(kf+1 == spline.rend());

    if (Ts_GetEffectiveExtrapolationType(*kf, spline, TsLeft)
        == TsExtrapolationHeld) {
        return spline.rend();
    } else {
        return kf;
    }
}

// This function finds the left most key frame before startKeyFrame that begins
// a constant flat spline segment that continues up to but does not include 
// startKeyFrame.
Ts_SplineChangedIntervalHelper::KeyFrameReverseIterator
Ts_SplineChangedIntervalHelper::_GetPreviousNonFlatKnot(
    const TsSpline &spline,
    const KeyFrameReverseIterator &startKeyFrame)
{
    TRACE_FUNCTION();

    // Start off assuming the previous non flat key frame is the one we
    // passed in.
    KeyFrameReverseIterator kf = startKeyFrame;

    // For array-valued spline, assume the previous knot is non-flat.
    // This is primarily an optimization to address expensive comparisons
    // required by the loop below for large arrays with large identical prefixes
    // between adjacent knots (which is often the case for animation data).
    // The outer loop calling this function will still iterate over each knot
    // to tighten the invalidation interval.
    if (kf == spline.rend()) {
        return kf;
    } else if (startKeyFrame->GetValue().IsArrayValued()) {
        ++kf;

        // If startKeyFrame is the first key frame, check the extrapolation to
        // the left
        if (kf == spline.rend()) {
            return _GetFirstKeyFrame(spline, startKeyFrame);
        } else {
            return kf;
        }
    }

    while (kf != spline.rend()) {
        // With the exception of the key frame we're starting with, check for
        // for value consistency from the left side to the right side
        if (kf != startKeyFrame) {
            // A dual valued not with different values means this key frame
            // is the next non flat knot
            if (kf->GetIsDualValued() && kf->GetLeftValue() != kf->GetValue()) {
                return kf;
            }
        }

        // Get the previous key frame
        KeyFrameReverseIterator prevKeyFrameIt = kf;
        ++prevKeyFrameIt;

        // If we're looking at the first key frame, then check the extrapolation
        // to the left
        if (prevKeyFrameIt == spline.rend()) {
            return _GetFirstKeyFrame(spline, kf);
        }

        // If the previous key frame is held, then we're automatically flat
        // up to the current key frame as long as the current key frame's left
        // value matches the previous key frame's held value or is the current
        // key frame is the starting key frame.
        if (prevKeyFrameIt->GetKnotType() == TsKnotHeld) {
            if (kf == startKeyFrame ||
                kf->GetLeftValue() == prevKeyFrameIt->GetValue()) {
                ++kf;
                continue;
            }
        }

        // If the segment from the previous key frame to the current one is not
        // flat, then this key frame is the next non flat one.
        if (!Ts_IsSegmentFlat(*prevKeyFrameIt, *kf)) {
            return kf;
        }

        // We passed all the flatness conditions so move to the next key frame.
        ++kf;
   }
   return kf;
}

// Tightens the right side of the changed interval up to the previous key frame 
// if possible and returns whether the interval can potentially be tightened 
// any more.
bool 
Ts_SplineChangedIntervalHelper::_TightenToPreviousKeyFrame(
    bool extrapolateHeldRight)
{
    TRACE_FUNCTION();
    const TsTime infinity = std::numeric_limits<TsTime>::infinity();

    // By default assume we won't be able to tighten any more beyond the 
    // previous key frame.
    bool canTightenMore = false;

    // If we're holding extrapolation for the right, then we can only tighten
    // if the right side values of the current key frames are equal.
    if (extrapolateHeldRight) {
        if (_s1ReverseIter->GetValue() != 
            _s2ReverseIter->GetValue()) {
            return false;
        }
    }

    // First find the previous non flat knots from the current key frames.
    KeyFrameReverseIterator s1PrevNonFlat = 
        _GetPreviousNonFlatKnot(*_s1, _s1ReverseIter);
    KeyFrameReverseIterator s2PrevNonFlat = 
        _GetPreviousNonFlatKnot(*_s2, _s2ReverseIter);

    // Store the values of the previous key frames (if the previous key frame
    // is past the left end of the spline, then we use the left value of the
    // spline's first key frame).
    const VtValue s1PrevValue = s1PrevNonFlat == _s1->rend() ? 
        _s1->begin()->GetLeftValue() : s1PrevNonFlat->GetValue();
    const VtValue s2PrevValue = s2PrevNonFlat == _s2->rend() ? 
        _s2->begin()->GetLeftValue() : s2PrevNonFlat->GetValue();

    // We have to do some extra checks if we're extrapolating held to the
    // right of our current key frames as _GetPreviousNonFlatKnot doesn't
    // look at the current key frame at all.
    if (extrapolateHeldRight) {
        // If the previous non flat knot is different then the current knot,
        // then we verify that the held value of the previous knot is the same
        // as the value of both sides of the current knot to ensure that 
        // segment is completely flat from the previous knot to infinity 
        // extrapolated beyond the current knot.  If this check fails, then
        // we have to roll the previous knot back to being the current knot.
        if (s1PrevNonFlat != _s1ReverseIter) {
            if (s1PrevValue != _s1ReverseIter->GetValue() ||
                (_s1ReverseIter->GetIsDualValued() && 
                 _s1ReverseIter->GetValue() != 
                 _s1ReverseIter->GetLeftValue())) {
                s1PrevNonFlat = _s1ReverseIter;
            }
        }
        if (s2PrevNonFlat != _s2ReverseIter) {
            if (s2PrevValue != _s2ReverseIter->GetValue() ||
                (_s2ReverseIter->GetIsDualValued() && 
                 _s2ReverseIter->GetValue() != 
                 _s2ReverseIter->GetLeftValue())) {
                s2PrevNonFlat = _s2ReverseIter;
            }
        }
    }

    // If we're extrapolating held from the right or we found flat segments of
    // the same value on both splines, then we can do the flat segment
    // interval tightening.
    if (extrapolateHeldRight ||
        (s1PrevNonFlat != _s1ReverseIter && s2PrevNonFlat != _s2ReverseIter &&
         s1PrevValue == s2PrevValue)) {

        // If the splines are flat to the end, then there is no 
        // evaluative difference between the two.  Return an empty
        // interval.
        const TsTime s1PrevKfTime = 
            (s1PrevNonFlat == _s1->rend()) ? -infinity : s1PrevNonFlat->GetTime();
        const TsTime s2PrevKfTime = 
            (s2PrevNonFlat == _s2->rend()) ? -infinity : s2PrevNonFlat->GetTime();

        // At this point we know we're tightening the interval from the right,
        // we still need to determine if end of the interval should be
        // closed or open and whether we can potentially continue tightening
        // from the right.
        bool closed = false;
        if (s1PrevKfTime > s2PrevKfTime)
        {
            // If s1's flat segment begins after s2's then the interval is 
            // closed only if s1's key frame has differing left and right side
            // values.
            closed = (s1PrevNonFlat->GetIsDualValued() &&
                s1PrevNonFlat->GetValue() != s1PrevNonFlat->GetLeftValue());
        } 
        else if (s2PrevKfTime > s1PrevKfTime) 
        {
            // If s2's flat segment begins after s1's then the interval is 
            // closed only if s2's key frame has differing left and right side
            // values.
            closed = (s2PrevNonFlat->GetIsDualValued() &&
                s2PrevNonFlat->GetValue() != s2PrevNonFlat->GetLeftValue());
        } 
        else // (s2PrevKfTime == s1PrevKfTime)
        {   
            // Otherwise both spline's flat segments begin at the same time.

            // If the splines are flat to the end, then there is no 
            // evaluative difference between the two.  Return an empty
            // interval.
            if (s1PrevKfTime == -infinity) {
                _changedInterval = GfInterval();
                return false;
            }
            // The interval is closed if the left values of the previous key
            // frames don't match (we've already guaranteed that the right
            // values match above).
            //
            // Note that the value *at* this time will not change, but since
            // we produce intervals that contain changed knots, we want an
            // interval that is closed on the right if the left values are
            // different.
            closed = 
                s1PrevNonFlat->GetLeftValue() != s2PrevNonFlat->GetLeftValue();

            // We can only potentially tighten more if the key frames have
            // equivalent values on both sides.
            canTightenMore = !closed;
        }

        // Update the changed interval with the new max value.
        _changedInterval.SetMax(GfMax(s1PrevKfTime, s2PrevKfTime), closed);

        // Update the reverse iterators to the beginning of the flat segments we
        // just checked.
        _s1ReverseIter = s1PrevNonFlat;
        _s2ReverseIter = s2PrevNonFlat;
    }

    // Otherwise we're not looking at a flat segment so just do a standard
    // segment equivalence check.
    else {
        // First make sure the left sides of the current key frames are 
        // equivalent.
        if (_s1ReverseIter->IsEquivalentAtSide(*_s2ReverseIter, TsLeft)) {
            // Move to the previous key frames and check if they're right 
            // equivalent.
            ++_s1ReverseIter;
            ++_s2ReverseIter;
            if (_s1ReverseIter != _s1->rend() &&
                _s2ReverseIter != _s2->rend() &&
                _s1ReverseIter->IsEquivalentAtSide(*_s2ReverseIter, TsRight)) {
                // Compare the left side values to determine if the interval
                // should be closed.
                //
                // Note that the value *at* this time will not change since
                // the right values are the same, but since we produce
                // intervals that contain changed knots, we want an interval
                // that is closed on the right if the left values are
                // different.
                const bool closed = (_s1ReverseIter->GetLeftValue() != 
                                     _s2ReverseIter->GetLeftValue());
                _changedInterval.SetMax(_s1ReverseIter->GetTime(), closed);

                // We can continue tightening if the knots are left equivalent.
                canTightenMore = !closed;
            } 
        }
    }

    return canTightenMore;
}

void
Ts_SplineChangedIntervalHelper::_TightenFromRight()
{
    TRACE_FUNCTION();

    // Initialize the reverse iterators to the last key frame in each spline
    _s1ReverseIter = _s1->rbegin();
    _s2ReverseIter = _s2->rbegin();

    // Get the effective extrapolations of each spline on the right side
    const TsExtrapolationType aExtrapRight = 
        Ts_GetEffectiveExtrapolationType(*_s1ReverseIter, *_s1, TsRight);
    const TsExtrapolationType bExtrapRight = 
        Ts_GetEffectiveExtrapolationType(*_s2ReverseIter, *_s2, TsRight);

    // We can't tighten if the extrapolations or the extrapolated values are 
    // different.
    if (aExtrapRight != bExtrapRight ||
        _s1ReverseIter->GetValue() != _s2ReverseIter->GetValue()) {
        return;
    }

    // If the extrapolation is held then tighten to the previous key frame
    // with right held extrapolation.
    if (aExtrapRight == TsExtrapolationHeld) {
        if (!_TightenToPreviousKeyFrame(true /*extrapolateHeldRight*/)) {
            // If we can't continue tightening then return.
            return;
        }
    }
    // Otherwise the extrapolation is linear so only if the time and
    // slopes match, do we not have a change after the last keyframes
    else if (_s1ReverseIter->GetTime() == _s2ReverseIter->GetTime() &&
             _s1ReverseIter->GetRightTangentSlope() == 
             _s2ReverseIter->GetRightTangentSlope()) {
        // Note that the value *at* this time will not change since the
        // right values are the same, but since we produce intervals
        // that contain changed knots, we want an interval that is
        // closed on the right if the left values are different.
        const bool closed = (_s1ReverseIter->GetLeftValue() != 
                             _s2ReverseIter->GetLeftValue());
        _changedInterval.SetMax(_s1ReverseIter->GetTime(), closed);
        // If the interval is closed, then we can't tighten any more so 
        // just return.
        if (closed) {
            return;
        }
    } else {
        // Otherwise we our extrapolations are not tightenable so just
        // return.
        return;
    }

    // Now just continue tightening the interval to the previous key frame
    // until we can no longer do so.
    while(_TightenToPreviousKeyFrame());
}

GfInterval
TsFindChangedInterval(const TsSpline &s1, const TsSpline &s2)
{
    TRACE_FUNCTION();
    return Ts_SplineChangedIntervalHelper(&s1, &s2).ComputeChangedInterval();
}

PXR_NAMESPACE_CLOSE_SCOPE
