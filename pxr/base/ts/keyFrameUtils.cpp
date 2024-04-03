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
#include "pxr/base/ts/keyFrameUtils.h"

#include "pxr/base/ts/data.h"
#include "pxr/base/ts/keyFrame.h"
#include "pxr/base/ts/keyFrameMap.h"

PXR_NAMESPACE_OPEN_SCOPE

const TsKeyFrame*
Ts_GetClosestKeyFrame(
    const TsKeyFrameMap &keyframes,
    const TsTime targetTime )
{
    if (keyframes.empty())
        return 0;

    const TsKeyFrame* closest;

    // Try the first frame with time >= targetTime
    TsKeyFrameMap::const_iterator it =
        keyframes.lower_bound( targetTime );

    // Nothing >=, so return the last element
    if (it == keyframes.end())
        return &(*keyframes.rbegin());

    closest = &(*it);

    // Now try the preceding frame
    if (it != keyframes.begin()) {
        --it;
        if ( (targetTime - it->GetTime()) < (closest->GetTime() - targetTime) )
            closest = &(*it);
    }

    return closest;
}

const TsKeyFrame*
Ts_GetClosestKeyFrameBefore(
    const TsKeyFrameMap &keyframes,
    const TsTime targetTime )
{

    if (keyframes.empty())
        return 0;
    
    TsKeyFrameMap::const_iterator it = 
        keyframes.lower_bound( targetTime );

    if (it == keyframes.end())
        return &(*keyframes.rbegin());

    if (it != keyframes.begin()) {
        --it;
        return &(*it);
    }

    return 0;
}

const TsKeyFrame*
Ts_GetClosestKeyFrameAfter(
    const TsKeyFrameMap &keyframes,
    const TsTime targetTime )
{
    if (keyframes.empty())
        return 0;

    TsKeyFrameMap::const_iterator it = 
        keyframes.lower_bound( targetTime );

    // Skip over keyframes that have the same time; we want the first
    // keyframe after them
    if (it != keyframes.end() && it->GetTime() == targetTime)
        ++it;
    if (it != keyframes.end())
        return &(*it);

    return 0;
}

std::pair<const TsKeyFrame *, const TsKeyFrame *>
Ts_GetClosestKeyFramesSurrounding(
    const TsKeyFrameMap &keyframes,
    const TsTime targetTime )
{
    std::pair<const TsKeyFrame *, const TsKeyFrame *> result;
    result.first = result.second = NULL;
    if (keyframes.empty())
        return result;

    // First the earliest at or after the targetTime
    TsKeyFrameMap::const_iterator itAtOrAfter = 
        keyframes.lower_bound( targetTime );

    // Set result.first
    if (itAtOrAfter == keyframes.end())
        result.first = &(*keyframes.rbegin());
    else {
        if (itAtOrAfter != keyframes.begin())
            result.first = &(*(itAtOrAfter - 1));
        // Else, result.first remains at NULL
    }

    // Set result.second
    // Skip over keyframes that have the same time; we want the first
    // keyframe after them
    if (itAtOrAfter != keyframes.end() && itAtOrAfter->GetTime() == targetTime)
        ++itAtOrAfter;
    if (itAtOrAfter != keyframes.end())
        result.second = &(*itAtOrAfter);
    // Else, result.second remains at NULL

    return result;
}

// Note: In the future this could be extended to evaluate the spline, and
//       by doing so we could support removing key frames that are redundant
//       but are not on flat sections of the spline.  Also, doing so would
//       avoid problems where such frames invalidate the frame cache.  If
//       all splines are cubic polynomials, then evaluating the spline at
//       four points, two before the key frame and two after, would be 
//       sufficient to tell if a particular key frame was redundant.
bool
Ts_IsKeyFrameRedundant( 
    const TsKeyFrameMap &keyframes,
    const TsKeyFrame &keyFrame,
    const TsLoopParams &loopParams,
    const VtValue& defaultValue)
{
    // If a knot is dual-valued, it can't possibly be redundant unless both of
    // its values are equal.
    if (keyFrame.GetIsDualValued()
            && !Ts_IsClose(keyFrame.GetLeftValue(), keyFrame.GetValue())){
        return false;
    }

    TsTime t = keyFrame.GetTime();
    const TsKeyFrame *prev = Ts_GetClosestKeyFrameBefore(keyframes,t);
    const TsKeyFrame *next = Ts_GetClosestKeyFrameAfter(keyframes,t);

    // For looping splines, the first and last knot in the master interval are
    // special as they interpolate, potentially, to multiple knots.  It's not
    // clear if the looping spline workflow calls for keeping these knots,
    // even if redundant, so we err on the side of conservatism and leave them
    // in.
    if (loopParams.IsValid()) {
        GfInterval master = loopParams.GetMasterInterval();
        if (master.Contains(t)) {
            // First in master interval?  Yes if there's no prev, or there is
            // a prev but it's not in the master interval
            if (!prev || !master.Contains(prev->GetTime()))
                return false;
            // Similar for last in master interval
            if (!next || !master.Contains(next->GetTime()))
                return false;
        }
    }

    if (prev && next) {
        if (keyFrame.GetKnotType() == TsKnotHeld && 
            prev->GetKnotType() == TsKnotHeld && 
            prev->GetValue() == keyFrame.GetValue()) {
            // If the both the previous key frame and the key frame we're 
            // checking are held with the same value, then the key frame is
            // redundant.
            return true;
        } else {
            // The key frame has two neighbors. If the spline is flat across all
            // three key frames, then the middle one is redundant.
            return Ts_IsSegmentFlat(*prev, keyFrame) && 
                Ts_IsSegmentFlat(keyFrame, *next);
        }
    } else if (!prev && next) {
        // This is the first key frame. If the spline is flat to the next
        // key frame, the first one is redundant.
        return Ts_IsSegmentFlat(keyFrame, *next);
    } else if (prev && !next) {
        // This is the last key frame. If the spline is flat to the previous
        // key frame, the last one is redundant.
        return Ts_IsSegmentFlat(*prev, keyFrame);
    } else if (!defaultValue.IsEmpty()) {
        // This is the only key frame. If its value is the same as the default 
        // value, it's redundant.
        return (Ts_IsClose(keyFrame.GetValue(), defaultValue));
    }

    return false;
}

// Note that this function is checking for flatness from the right side value
// of kf1 up to and including the left side value of kf2.
bool
Ts_IsSegmentFlat( const TsKeyFrame &kf1, const TsKeyFrame &kf2 )
{
    if (kf1.GetTime() >= kf2.GetTime()) {
        TF_CODING_ERROR("The first key frame must come before the second.");
        return false;
    }

    // If the second knot in the comparison is dual-valued, we should consider
    // its left value.
    const VtValue &v1 = kf1.GetValue();
    const VtValue &v2 =
        (kf2.GetIsDualValued()) ? kf2.GetLeftValue() : kf2.GetValue();

    // If the values differ, the segment cannot be flat.
    if (!Ts_IsClose(v1, v2)) {
        return false;
    }

    // Special case for held knots.  
    if (kf1.GetKnotType() == TsKnotHeld) {
        // Otherwise all segments starting with a held knot are flat until the 
        // the next key frame
        return true;
    }

    // Make sure the tangents are flat.
    //
    // XXX: TsKeyFrame::GetValueDerivative() returns the
    //      slope of the tangents, regardless of the knot type.
    //
    if (kf1.HasTangents()
            && !Ts_IsClose(kf1.GetValueDerivative(), kf1.GetZero())) {
        return false;
    }

    if (kf2.HasTangents()
            && !Ts_IsClose(kf2.GetLeftValueDerivative(), kf2.GetZero())) {
        return false;
    }

    return true;
}

Ts_Data* Ts_GetKeyFrameData(TsKeyFrame &kf)
{
    return kf._holder.GetMutable();
}

Ts_Data const* Ts_GetKeyFrameData(TsKeyFrame const& kf)
{
    return kf._holder.Get();
}

bool Ts_IsClose(const VtValue &v0, const VtValue &v1)
{
    static const double EPS = 1e-6;
    double v0dbl = 0.0; // Make compiler happy
    double v1dbl = 0.0;

    // Note that we don't use CanCast and Cast here because that would be
    // slower, and also, we don't want to cast int and bool.

    // Get out the v0 val if a float or double
    if ( v0.IsHolding<double>() )
        v0dbl = v0.UncheckedGet<double>();
    else if ( v0.IsHolding<float>() )
        v0dbl = v0.UncheckedGet<float>();
    else
        // Not either, so use ==
        return v0 == v1;

    // Get out the v1 val if a float or double
    if ( v1.IsHolding<double>() )
        v1dbl = v1.UncheckedGet<double>();
    else if ( v1.IsHolding<float>() )
        v1dbl = v1.UncheckedGet<float>();
    else
        // Not either, so use ==
        return v0 == v1;

    return TfAbs(v0dbl - v1dbl) < EPS;
}

PXR_NAMESPACE_CLOSE_SCOPE
