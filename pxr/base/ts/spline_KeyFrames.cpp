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
#include "pxr/base/ts/spline_KeyFrames.h"
#include "pxr/base/ts/evalUtils.h"
#include "pxr/base/ts/keyFrameUtils.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/trace/trace.h"
#include <limits>

PXR_NAMESPACE_OPEN_SCOPE

using std::string;
using std::vector;

TsSpline_KeyFrames::TsSpline_KeyFrames() :
    _extrapolation(TsExtrapolationHeld, TsExtrapolationHeld)
{
}

TsSpline_KeyFrames
::TsSpline_KeyFrames(TsSpline_KeyFrames const &other,
                       TsKeyFrameMap const *keyFrames)
    : _extrapolation(other._extrapolation)
    , _loopParams(other._loopParams)
{
    if (keyFrames) {
        if (_loopParams.GetLooping()) {
            // If looping, there might be knots hidden under the echos of the
            // loop that we need to preserve.
            _normalKeyFrames = other._normalKeyFrames;
        }

        SetKeyFrames(*keyFrames);
    } else {
        _loopedKeyFrames = other._loopedKeyFrames;
        _normalKeyFrames = other._normalKeyFrames;
    }
}

TsSpline_KeyFrames::~TsSpline_KeyFrames()
{
}

const TsKeyFrameMap & 
TsSpline_KeyFrames::GetKeyFrames() const
{
    if (_loopParams.GetLooping()) {
        return _loopedKeyFrames;
    }
    else {
        return _normalKeyFrames;
    }
}

const TsKeyFrameMap & 
TsSpline_KeyFrames::GetNormalKeyFrames() const
{
    return _normalKeyFrames;
}

void
TsSpline_KeyFrames::SetKeyFrames(const TsKeyFrameMap &keyFrames)
{
    TfAutoMallocTag2 tag("Ts", "TsSpline_KeyFrames::SetKeyFrames");
    TRACE_FUNCTION();

    if (_loopParams.GetLooping()) {
        _loopedKeyFrames = keyFrames;
        _UnrollMaster();
        // Keep the normal keys in sync; this is so we can write out scene
        // description (which only refects the normal keys) at any time.
        // Note we don't update the eval cache for the normal keys; we'll do
        // this if/when we switch back to normal mode.
        _SetNormalFromLooped();
    }
    else {
        _normalKeyFrames = keyFrames;
    }
}

void
TsSpline_KeyFrames::SwapKeyFrames(std::vector<TsKeyFrame>* keyFrames)
{
    TRACE_FUNCTION();

    if (_loopParams.GetLooping()) {
        _loopedKeyFrames.swap(*keyFrames);
        _UnrollMaster();
        // Keep the normal keys in sync; this is so we can write out scene
        // description (which only refects the normal keys) at any time.
        // Note we don't update the eval cache for the normal keys; we'll do
        // this if/when we switch back to normal mode.
        _SetNormalFromLooped();
    }
    else {
        _normalKeyFrames.swap(*keyFrames);
    }
}

void
TsSpline_KeyFrames::SetKeyFrame(
    TsKeyFrame kf, 
    GfInterval *intervalAffected)
{
    TfAutoMallocTag2 tag("Ts", "TsSpline_KeyFrames::SetKeyFrame");
    TsTime t = kf.GetTime();

    if (_loopParams.GetLooping()) {
        // Get Loop domain intervals
        GfInterval loopedInterval =
            _loopParams.GetLoopedInterval();
        GfInterval masterInterval =
            _loopParams.GetMasterInterval();

        bool inMaster = masterInterval.Contains(t);
        // Punt if not in the writable range
        if (loopedInterval.Contains(t) && !inMaster) {
            return;
        }

        _loopedKeyFrames[t] = kf;

        // Keep the normal keys in sync; this is so we can write out scene
        // description (which only refelcts the normal keys) at any time
        // Note we don't update the eval cache for the normal keys; we'll do
        // this if/when we swtich back to normal mode.
        _normalKeyFrames[t] = kf;

        // The times that we added, including the one passed to us.  Note these
        // will not necessarily be in time order
        vector<TsTime> times(1, t);

        if (inMaster) {
            // Iterators for the key to propagate
            TsKeyFrameMap::iterator k = _loopedKeyFrames.find(t);
            if (k == _loopedKeyFrames.end())
                return;  // Yikes; just inserted it

            TsKeyFrameMap::iterator k0 = k++;
            _UnrollKeyFrameRange(&_loopedKeyFrames, k0, k,
                    _loopParams, &times);
        }
        
        // Set intervalAffected
        if (intervalAffected) {
            TF_FOR_ALL(ti, times) {
                // For non-looping splines, we already computed the interval
                // changed, before the key was instered.  For looping splines
                // this is too hard (and not worth it) so we compute here,
                // afterwards.
                *intervalAffected |= _GetTimeInterval(*ti);
            }
        }
    } else {
        // Non-looping
        if (intervalAffected) {
            // Optimize the case where the param is empty
            if (intervalAffected->IsEmpty())
                *intervalAffected = _FindSetKeyFrameChangedInterval(kf);
            else {
                *intervalAffected |= _FindSetKeyFrameChangedInterval(kf);
            }
        }

        _normalKeyFrames[t] = kf;
    }
}

void
TsSpline_KeyFrames::RemoveKeyFrame( TsTime t,
        GfInterval *intervalAffected )
{
    TfAutoMallocTag2 tag("Ts", "TsSpline_KeyFrames::RemoveKeyFrame");
    // Assume none removed
    if (intervalAffected)
        *intervalAffected = GfInterval();

    if (_loopParams.GetLooping()) {
        // Get Loop domain intervals
        GfInterval loopedInterval =
            _loopParams.GetLoopedInterval();
        GfInterval masterInterval =
            _loopParams.GetMasterInterval();

        bool inMaster = masterInterval.Contains(t);
        // Punt if not in the writable range
        if (loopedInterval.Contains(t) && !inMaster) {
            return;
        }

        // Error if we've been asked to remove a keyframe that doesn't exist
        if (_loopedKeyFrames.find( t ) == _loopedKeyFrames.end()) {
            TF_CODING_ERROR("keyframe does not exist; not removing");
            return;
        }

        // Remove the requested time.  This will either be in the master
        // interval, or outside the looped interval
        if (intervalAffected) {
            *intervalAffected |= 
                _FindRemoveKeyFrameChangedInterval(t);
        }
        _loopedKeyFrames.erase(t);

        // If we removed it from the master interval we now how to iterate 
        // over all the echos and remove it from them too
        if (inMaster) {
            // Number of whole iterations to cover the prepeat range
            int numPrepeats = ceil((masterInterval.GetMin() -
                        loopedInterval.GetMin()) / masterInterval.GetSize());
            // Number of whole iterations to cover the repeat range
            int numRepeats = ceil((loopedInterval.GetMax() -
                        masterInterval.GetMax()) / masterInterval.GetSize());

            // Iterate from the first prepeat to the last repeat
            for (int i = -numPrepeats; i <= numRepeats; i++) {
                TsTime timeOffset = i * masterInterval.GetSize();
                // Already removed it from the master interval
                if (i == 0)
                    continue;

                // Shift time
                TsTime time = t + timeOffset;
                // In case the pre/repeat ranges were not multiples of the
                // period, the first and last iterations may have some knots
                // outside the range
                if (!loopedInterval.Contains(time))
                    continue;

                if (intervalAffected) {
                    *intervalAffected |= 
                        _FindRemoveKeyFrameChangedInterval(time);
                }
                _loopedKeyFrames.erase(time);
            }
        }

    } else {
        // Non-looping

        // Error if we've been asked to remove a keyframe that doesn't exist
        if (_normalKeyFrames.find( t ) == _normalKeyFrames.end()) {
            TF_CODING_ERROR("keyframe does not exist; not removing");
            return;
        }
        if (intervalAffected) {
            *intervalAffected |= _FindRemoveKeyFrameChangedInterval(t);
        }
        // Actual removal below
    }

    // Whether looping or not, remove it from the normal keys to keep them in
    // sync.
    _normalKeyFrames.erase(t);
}

void
TsSpline_KeyFrames::Clear()
{
    TfAutoMallocTag2 tag("Ts", "TsSpline_KeyFrames::Clear");
    TfReset(_normalKeyFrames);
    TfReset(_loopedKeyFrames);
}

void
TsSpline_KeyFrames::_LoopParamsChanged(bool loopingChanged, 
        bool valueOffsetChanged, bool domainChanged) 
{
    // Punt if nothing changed
    if (!(loopingChanged | valueOffsetChanged | domainChanged))
        return;

    // If we're now looping, then whatever the change was, re-generate the
    // looped keys from the normal ones
    if (_loopParams.GetLooping()) {
        _SetLoopedFromNormal();
    }
}

void
TsSpline_KeyFrames::_SetNormalFromLooped()
{
    TfAutoMallocTag2 tag("Ts", "TsSpline_KeyFrames::_SetNormalFromLooped");

    // Get Loop domain intervals
    GfInterval loopedInterval =
        _loopParams.GetLoopedInterval();
    GfInterval masterInterval =
        _loopParams.GetMasterInterval();

    // Clear the region before the prepeat and copy the corresponding from
    // the looped keys
    _normalKeyFrames.erase(
            _normalKeyFrames.begin(),
            _normalKeyFrames.lower_bound(loopedInterval.GetMin()));
    _normalKeyFrames.insert(
            _loopedKeyFrames.begin(),
            _loopedKeyFrames.lower_bound(loopedInterval.GetMin()));

    // Clear the masterInterval region and copy the corresponding from the
    // looped keys
    _normalKeyFrames.erase(
            _normalKeyFrames.lower_bound(masterInterval.GetMin()),
            _normalKeyFrames.lower_bound(masterInterval.GetMax()));
    _normalKeyFrames.insert(
            _loopedKeyFrames.lower_bound(masterInterval.GetMin()),
            _loopedKeyFrames.lower_bound(masterInterval.GetMax()));

    // Clear the region after the repeat and copy the corresponding from
    // the looped keys
    _normalKeyFrames.erase(
            _normalKeyFrames.lower_bound(loopedInterval.GetMax()),
            _normalKeyFrames.end());
    _normalKeyFrames.insert(
            _loopedKeyFrames.lower_bound(loopedInterval.GetMax()),
            _loopedKeyFrames.end());
}

void
TsSpline_KeyFrames::_SetLoopedFromNormal()
{
    TfAutoMallocTag2 tag("Ts", "TsSpline_KeyFrames::_SetLoopedFromNormal");

    _loopedKeyFrames = _normalKeyFrames;
    _UnrollMaster();
}

bool 
TsSpline_KeyFrames::operator==(const TsSpline_KeyFrames &rhs) const
{
    TRACE_FUNCTION();
    
    if (_extrapolation != rhs._extrapolation ||
        _loopParams != rhs._loopParams) {
        return false;
    }
    
    // If looping, compare both maps, else just the normal ones
    bool normalEqual = _normalKeyFrames == rhs._normalKeyFrames;
    if (!_loopParams.GetLooping()) {
        return normalEqual;
    }
    else {
        return normalEqual && _loopedKeyFrames == rhs._loopedKeyFrames;
    }
}

void
TsSpline_KeyFrames::BakeSplineLoops()
{
    _loopParams.SetLooping(false);
    _UnrollKeyFrames(&_normalKeyFrames, _loopParams);
    // Clear the loop params after baking
    _loopParams = TsLoopParams();
}

void 
TsSpline_KeyFrames::_UnrollMaster()
{
    TfAutoMallocTag2 tag("Ts", "TsSpline_KeyFrames::_UnrollMaster");

    _UnrollKeyFrames(&_loopedKeyFrames, _loopParams);
}

void
TsSpline_KeyFrames::_UnrollKeyFrames(TsKeyFrameMap *keyFrames,
    const TsLoopParams &params)
{
    // Get Loop domain intervals
    GfInterval loopedInterval = params.GetLoopedInterval();
    GfInterval masterInterval = params.GetMasterInterval();

    // Clear the keys in the prepeat range
    keyFrames->erase(
            keyFrames->lower_bound(loopedInterval.GetMin()), 
            keyFrames->lower_bound(masterInterval.GetMin()));

    // Clear the keys in the repeat range
    keyFrames->erase(
            keyFrames->lower_bound(masterInterval.GetMax()), 
            keyFrames->lower_bound(loopedInterval.GetMax()));

    // Iterators for the masterInterval keys to propagate
    TsKeyFrameMap::iterator k0 = 
        keyFrames->lower_bound(masterInterval.GetMin());
    TsKeyFrameMap::iterator k1 = 
        keyFrames->lower_bound(masterInterval.GetMax());

    _UnrollKeyFrameRange(keyFrames, k0, k1, params);
}

void
TsSpline_KeyFrames::_UnrollKeyFrameRange(
        TsKeyFrameMap *keyFrames,
        const TsKeyFrameMap::iterator &k0,
        const TsKeyFrameMap::iterator &k1,
        const TsLoopParams &params,
        std::vector<TsTime> *times)
{
    // Get Loop domain intervals
    GfInterval loopedInterval = params.GetLoopedInterval();
    GfInterval masterInterval = params.GetMasterInterval();

    // Propagate the master keys inside the 'loopedInterval'
    //
    // Number of whole iterations to cover the prepeat range; we trim down
    // below
    int numPrepeats = ceil((masterInterval.GetMin() - loopedInterval.GetMin()) /
            masterInterval.GetSize());
    // Number of whole iterations to cover the repeat range; we trim down
    // below
    int numRepeats = ceil((loopedInterval.GetMax() - masterInterval.GetMax()) /
            masterInterval.GetSize());

    // Iterate from the first prepeat to the last repeat, copying from the 
    // masterInterval and shifting in time and possibly value
    TsKeyFrameMap newKeyFrames = *keyFrames;
    for (int i = -numPrepeats; i <= numRepeats; i++) {
        if (i == 0)
            continue; // The masterInterval frames are already in place
        TsTime timeOffset = i * masterInterval.GetSize();
        double valueOffset = i * params.GetValueOffset();
        for (TsKeyFrameMap::iterator k = k0; k != k1; k++) {
            TsKeyFrame key = *k;

            // Shift time
            TsTime t = key.GetTime() + timeOffset;
            // In case the pre/repeat ranges were not multiples of the period,
            // the first and last iterations may have some knots outside the
            // range
            if (!loopedInterval.Contains(t))
                continue;
            key.SetTime(t);

            // Shift value if a double
            VtValue v = key.GetValue();
            if (v.IsHolding<double>()) {
                key.SetValue(VtValue(v.Get<double>() + valueOffset));
                // Handle dual valued
                if (key.GetIsDualValued()) {
                    key.SetLeftValue(
                       VtValue(key.GetLeftValue().Get<double>() + valueOffset));
                }
            }

            // Clobber existing
            newKeyFrames[t] = key;

            // Remember times we changed
            if (times)
                times->push_back(t);
        }
    }
    *keyFrames = newKeyFrames;
}

TsKeyFrameMap *
TsSpline_KeyFrames::_GetKeyFramesMutable()
{
    return const_cast<TsKeyFrameMap *>(&GetKeyFrames());
}

TsSpline_KeyFrames::_KeyFrameRange
TsSpline_KeyFrames::_GetKeyFrameRange( TsTime time )
{
    // Get the keyframe after time
    TsKeyFrameMap::iterator i = _GetKeyFramesMutable()->upper_bound( time );

    // Get the keyframe before time
    TsKeyFrameMap::iterator j = i;
    if (j != _GetKeyFramesMutable()->begin()) {
        --j;
        if (j->GetTime() == time && j != _GetKeyFramesMutable()->begin()) {
            // There's a keyframe at time so go to the previous keyframe.
            --j;
        }
    }

    return std::make_pair(j, i);
}

TsSpline_KeyFrames::_KeyFrameRange
TsSpline_KeyFrames::_GetKeyFrameRange( TsTime leftTime, TsTime rightTime )
{
    // Get the keyframe before leftTime
    TsKeyFrameMap::iterator i = _GetKeyFramesMutable()->lower_bound(
            leftTime );
    if (i != _GetKeyFramesMutable()->begin()) {
        --i;
    }

    // Get the keyframe after rightTime
    TsKeyFrameMap::iterator j = _GetKeyFramesMutable()->upper_bound(
            rightTime );

    return std::make_pair(i, j);
}

GfInterval
TsSpline_KeyFrames::_GetTimeInterval( TsTime t )
{
    GfInterval result = GfInterval::GetFullInterval();

    if (GetKeyFrames().empty())
        return result;

    TsKeyFrameMap::const_iterator lower = GetKeyFrames().lower_bound(t);
    TsKeyFrameMap::const_iterator upper = lower;
    if (upper != GetKeyFrames().end()) {
        ++upper;
    }

    std::pair<TsKeyFrameMap::const_iterator,
        TsKeyFrameMap::const_iterator> range(
            GetKeyFrames().lower_bound(t),
            GetKeyFrames().upper_bound(t));
    
    // Tighten min bound.
    if (range.first != GetKeyFrames().begin()) {
        // Start at previous knot.
        TsKeyFrameMap::const_iterator prev = range.first;
        --prev;
        result.SetMin( prev->GetTime(), prev->GetTime() == t );
    } else {
        // No previous knots -- therefore min is unbounded.
    }

    // Tighten max bound.
    if (range.second != GetKeyFrames().end()) {
        result.SetMax( range.second->GetTime(), range.second->GetTime() == t );
    } else {
        // No subsequent knots -- therefore max is unbounded.
    }

    return result;
}

GfInterval 
TsSpline_KeyFrames::_FindRemoveKeyFrameChangedInterval(TsTime time)
{
    // No change if there's no keyframe at the given time.
    TsKeyFrameMap::const_iterator iter = GetKeyFrames().find(time);
    if (iter == GetKeyFrames().end()) {
        return GfInterval();
    }

    // If the keyframe is redundant, then there's no change
    const TsKeyFrame &keyFrame = *iter;
    if (Ts_IsKeyFrameRedundant(GetKeyFrames(), keyFrame)) {
        return GfInterval();
    }

    // First assume everything from the previous keyframe to the next keyframe
    // has changed.
    GfInterval r = _GetTimeInterval(time);

    _KeyFrameRange keyFrameRange = _GetKeyFrameRange(time);

    // If it's the only key frame and the key frame was not redundant, we
    // just invalidate the entire interval.
    if (GetKeyFrames().size() == 1) {
        return GfInterval::GetFullInterval();
    }

    // If there is no keyframe to the left, then we do an extrapolation
    // comparison.
    if (r.GetMin() == -std::numeric_limits<TsTime>::infinity()) {
        const TsKeyFrame & nextKeyFrame = *(keyFrameRange.second);
        // Get the effective extrapolations of each spline on the left side
        TsExtrapolationType aExtrapLeft = 
            _GetEffectiveExtrapolationType(nextKeyFrame, TsLeft);
        TsExtrapolationType bExtrapLeft = 
            _GetEffectiveExtrapolationType(keyFrame, TsLeft);

        // We can tighten if the extrapolations of both knots are held and
        // their left values are the same
        if (aExtrapLeft == TsExtrapolationHeld && 
            bExtrapLeft == TsExtrapolationHeld && 
            nextKeyFrame.GetLeftValue() == keyFrame.GetLeftValue()) {

            r.SetMin(time, /* closed */ false);
        }
    } else {
        // If there is a keyframe to the left that is held, the changed 
        // interval starts at the removed key frame.
        TsKeyFrameMap::const_iterator
            it = GetKeyFrames().find(r.GetMin());
        if (it != GetKeyFrames().end() && 
            it->GetKnotType() == TsKnotHeld) {
            r.SetMin(time, /* closed */ true);
        }
    }
    // If there is no keyframe to the right, then we do an extrapolation
    // comparison.
    if (r.GetMax() == std::numeric_limits<TsTime>::infinity()) {
        const TsKeyFrame & prevKeyFrame = *(keyFrameRange.first);
        // Get the effective extrapolations of each spline on the right side
        TsExtrapolationType aExtrapRight = 
            _GetEffectiveExtrapolationType(prevKeyFrame, TsRight);
        TsExtrapolationType bExtrapRight = 
            _GetEffectiveExtrapolationType(keyFrame, TsRight);

        // We can tighten if the extrapolations are the same
        if (aExtrapRight == TsExtrapolationHeld &&
            bExtrapRight == TsExtrapolationHeld &&
            prevKeyFrame.GetValue() == keyFrame.GetValue()) {

            r.SetMax(time, /* closed */ false);
        }
    }

    if (r.IsEmpty()) {
        return GfInterval();
    }
    return r;
}

GfInterval 
TsSpline_KeyFrames::_FindSetKeyFrameChangedInterval(
    const TsKeyFrame &keyFrame)
{
    const TsKeyFrameMap &keyFrames = GetKeyFrames();
    TsTime time = keyFrame.GetTime();

    // If adding a new key frame that is redundant, nothing changed, just
    // return an empty interval.
    if (Ts_IsKeyFrameRedundant(keyFrames, keyFrame)) {
        if (keyFrames.find(time) == keyFrames.end() || 
            Ts_IsKeyFrameRedundant(keyFrames, *keyFrames.find(time))) {
            return GfInterval();
        }
    }

    // First assume everything from the previous keyframe to the next keyframe
    // has changed.
    GfInterval r = _GetTimeInterval(time);

    // If the spline is empty then just return the entire interval.
    if (keyFrames.empty()) {
        return r;
    }

    // If there is no keyframe to the left, then we do an extrapolation
    // comparison.
    if (r.GetMin() == -std::numeric_limits<TsTime>::infinity()) {
        const TsKeyFrame & firstKeyFrame = *(keyFrames.begin());
        // Get the effective extrapolations of each spline on the left side
        TsExtrapolationType aExtrapLeft = 
            _GetEffectiveExtrapolationType(firstKeyFrame, TsLeft);
        TsExtrapolationType bExtrapLeft = 
            _GetEffectiveExtrapolationType(keyFrame, TsLeft);

        // We can tighten if the extrapolations are the same
        if (aExtrapLeft == bExtrapLeft) {
            // if the first keyframes of both splines are the same, then we may
            // not have any changes to left of the first keyframes
            if (firstKeyFrame.GetLeftValue() == keyFrame.GetLeftValue()) {
                // If the extrapolation is held to the left, then there are no 
                // changes before the minimum of the first keyframe times
                if (aExtrapLeft == TsExtrapolationHeld) {
                    r.SetMin(time, /* closed */ false);
                }
                // Otherwise the extrapolation is linear so only if the time and
                // slopes match, do we not have a change before the first
                // keyframes
                else if (firstKeyFrame.GetTime() == time &&
                         firstKeyFrame.GetLeftTangentSlope() == 
                         keyFrame.GetLeftTangentSlope()) {
                    r.SetMin(time, /* closed */ false);
                }
            }
        }
    } else {
        // If there is a keyframe to the left that is held, the changed 
        // interval starts at the added key frame.
        TsKeyFrameMap::const_iterator
            it = keyFrames.find(r.GetMin());
        if (it != keyFrames.end() && it->GetKnotType() == TsKnotHeld) {
            r.SetMin(time, /* closed */ it->GetValue() != keyFrame.GetValue());
        }
    }
    // If there is no keyframe to the right, then we do an extrapolation
    // comparison.
    if (r.GetMax() == std::numeric_limits<TsTime>::infinity()) {
        const TsKeyFrame & lastKeyFrame = *(keyFrames.rbegin());
        // Get the effective extrapolations of each spline on the right side
        TsExtrapolationType aExtrapRight = 
            _GetEffectiveExtrapolationType(lastKeyFrame, TsRight);
        TsExtrapolationType bExtrapRight = 
            _GetEffectiveExtrapolationType(keyFrame, TsRight);

        // We can tighten if the extrapolations are the same
        if (aExtrapRight == bExtrapRight) {
            // if the last keyframes of both splines are the same, then we may
            // not have any changes to right of the last keyframes
            if (lastKeyFrame.GetValue() == keyFrame.GetValue()) {
                // If the extrapolation is held to the right, then there are no 
                // changes after the maximum of the last keyframe times
                if (aExtrapRight == TsExtrapolationHeld) {
                    r.SetMax(time, /* closed */ false);
                }
                // Otherwise the extrapolation is linear so only if the time and
                // slopes match, do we not have a change after the last keyframes
                else if (lastKeyFrame.GetTime() == time &&
                         lastKeyFrame.GetRightTangentSlope() == 
                         keyFrame.GetRightTangentSlope()) {
                    r.SetMax(time, /* closed */ false);
                }
            }
        }
    }
    // If we're replacing an existing keyframe.
    TsKeyFrameMap::const_iterator it = keyFrames.find(time);
    if (it != keyFrames.end()) {
        const TsKeyFrame &k = *(it);
        _KeyFrameRange keyFrameRange = _GetKeyFrameRange(time);
        if (k.IsEquivalentAtSide(keyFrame, TsLeft)) {
            r.SetMin(time, k.GetValue() != keyFrame.GetValue());
        } else if (keyFrameRange.first->GetTime() != time &&
            (keyFrameRange.first->GetKnotType() == TsKnotHeld ||
                (Ts_IsSegmentFlat(*(keyFrameRange.first), k) &&
                    Ts_IsSegmentFlat(*(keyFrameRange.first), keyFrame)))) {
            r.SetMin(time, k.GetValue() != keyFrame.GetValue());
        }

        if (k.IsEquivalentAtSide(keyFrame, TsRight)) {
            // Note that the value *at* this time will not change since the
            // right values are the same, but since we produce intervals
            // that contain changed knots, we want an interval that is
            // closed on the right if the left values are different.
            r.SetMax(time, k.GetLeftValue() != keyFrame.GetLeftValue());
        } else if (keyFrameRange.second != keyFrames.end() &&
            Ts_IsSegmentFlat(k, *(keyFrameRange.second)) &&
            Ts_IsSegmentFlat(keyFrame, *(keyFrameRange.second))) {
            r.SetMax(time, k.GetLeftValue() != keyFrame.GetLeftValue());
        }
    }

    if (r.IsEmpty()) {
        return GfInterval();
    }
    return r;
}

TsExtrapolationType
TsSpline_KeyFrames::_GetEffectiveExtrapolationType(
    const TsKeyFrame &keyFrame,
    const TsSide &side) const
{
    return Ts_GetEffectiveExtrapolationType(keyFrame, GetExtrapolation(),
        GetKeyFrames().size() == 1, side);
}

const TsLoopParams &
TsSpline_KeyFrames::GetLoopParams() const
{
    return _loopParams;
}

void
TsSpline_KeyFrames::SetLoopParams(const TsLoopParams &params)
{
    TfAutoMallocTag2 tag("Ts", "TsSpline_KeyFrames::SetLoopParams");

    // Note what's changing, to inform _keyframes (don't care about the group)
    bool loopingChanged = params.GetLooping() != _loopParams.GetLooping();
    bool valueOffsetChanged = 
        params.GetValueOffset() != _loopParams.GetValueOffset();
    bool domainChanged = params != _loopParams;

    // Make the change
    _loopParams = params;

    // Tell _keyframes
    _LoopParamsChanged(loopingChanged, valueOffsetChanged, domainChanged);
}

void
TsSpline_KeyFrames::SetExtrapolation(
    const TsExtrapolationPair &extrapolation)
{
    _extrapolation = extrapolation;
}

const TsExtrapolationPair &
TsSpline_KeyFrames::GetExtrapolation() const
{
    return _extrapolation;
}

PXR_NAMESPACE_CLOSE_SCOPE
