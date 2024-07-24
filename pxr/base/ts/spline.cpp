//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/spline.h"

#include "pxr/base/ts/spline_KeyFrames.h"
#include "pxr/base/ts/loopParams.h"
#include "pxr/base/ts/data.h"
#include "pxr/base/ts/keyFrameUtils.h"
#include "pxr/base/ts/evalUtils.h"
#include "pxr/base/tf/enum.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/gf/math.h" 
#include <limits>

PXR_NAMESPACE_OPEN_SCOPE

using std::string;

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<TsSpline>();
}

TsSpline::TsSpline() :
    _data(new TsSpline_KeyFrames())
{
}

TsSpline::TsSpline(const TsSpline &other) :
    _data(other._data)
{
}

TsSpline::TsSpline( const TsKeyFrameMap & kf,
                        TsExtrapolationType left,
                        TsExtrapolationType right,
                        const TsLoopParams &loopParams) :
    _data(new TsSpline_KeyFrames())
{
    _data->SetExtrapolation(TsExtrapolationPair(left, right));
    _data->SetLoopParams(loopParams);
    _data->SetKeyFrames(kf);
}

TsSpline::TsSpline( const std::vector<TsKeyFrame> & kf,
                        TsExtrapolationType left,
                        TsExtrapolationType right,
                        const TsLoopParams &loopParams) :
    _data(new TsSpline_KeyFrames())
{
    _data->SetExtrapolation(TsExtrapolationPair(left, right));
    _data->SetLoopParams(loopParams);

    // We can't construct a std::map from a std::vector directly.
    // This also performs type checks for every keyframe coming in.
    TF_FOR_ALL(it, kf)
        SetKeyFrame(*it);
}

bool TsSpline::operator==(const TsSpline &rhs) const
{
    // The splines are equal if the data are the same object
    // or if they contain the same information.
    return (_data == rhs._data || *_data == *rhs._data);
}

bool TsSpline::operator!=(const TsSpline &rhs) const
{
    return !(*this == rhs);
}

bool TsSpline::IsEmpty() const
{
    return GetKeyFrames().empty();
}

void
TsSpline::_Detach()
{
    TfAutoMallocTag2 tag( "Ts", "TsSpline::_Detach" );
    
    if (!_data.unique()) {
        std::shared_ptr<TsSpline_KeyFrames> newData(
            new TsSpline_KeyFrames(*_data));
        _data.swap(newData);
    }
}

bool
TsSpline
::ClearRedundantKeyFrames( const VtValue &defaultValue /* = VtValue() */,
                           const GfMultiInterval &intervals /* = infinity */)
{
    // Retrieve a copy of the key frames;
    // we'll be deleting from the spline as we iterate.
    TsKeyFrameMap keyFrames = GetKeyFrames();

    bool changed = false;

    // For performance, skip testing inclusion if we have the infinite range.
    bool needToTestIntervals = 
        intervals != GfMultiInterval(GfInterval(
                                    -std::numeric_limits<double>::infinity(),
                                    std::numeric_limits<double>::infinity()));
    // Iterate in reverse. In many cases, this doesn't matter -- but for a
    // sequence of contiguous redundant knots, this means that the first
    // one will remain instead of the last one.
    TF_REVERSE_FOR_ALL(i, keyFrames) {
        const TsKeyFrame &kf = *i;
        // Passing an invalid VtValue here has the effect that the final
        // knot will not ever be deleted.
        if (IsKeyFrameRedundant(kf, defaultValue)) {
            // Punt if this knot is not in 'intervals'
            if (needToTestIntervals && !intervals.Contains(kf.GetTime()))
                continue;

            // Immediately delete redundant key frames; this may affect
            // whether subsequent key frames are redundant or not.
            RemoveKeyFrame(i->GetTime());
            changed = true;
        }
    }

    return changed;
}

const TsKeyFrameMap &
TsSpline::GetKeyFrames() const
{
    return _data->GetKeyFrames();
}

const TsKeyFrameMap &
TsSpline::GetRawKeyFrames() const
{
    return _data->GetNormalKeyFrames();
}

std::vector<TsKeyFrame>
TsSpline::GetKeyFramesInMultiInterval(
    const GfMultiInterval &intervals) const
{
    TRACE_FUNCTION();
    std::vector<TsKeyFrame> result;
    TF_FOR_ALL(kf, GetKeyFrames()) {
        if (intervals.Contains(kf->GetTime()))
            result.push_back(*kf);
    }
    return result;
}

GfInterval
TsSpline::GetFrameRange() const
{
    if (IsEmpty())
        return GfInterval();

    TsKeyFrameMap const & keyframes = GetKeyFrames();

    return GfInterval( keyframes.begin()->GetTime(),
        keyframes.rbegin()->GetTime() );
}

void
TsSpline::SwapKeyFrames(std::vector<TsKeyFrame>* swapInto)
{
    _Detach();
    _data->SwapKeyFrames(swapInto);
}

bool
TsSpline::CanSetKeyFrame( const TsKeyFrame & kf, std::string *reason ) const
{
    if (IsEmpty())
        return true;

    const VtValue kfValue = kf.GetValue();

    if (ARCH_UNLIKELY(!TfSafeTypeCompare(GetTypeid(), kfValue.GetTypeid())))
    {
        // Values cannot have keyframes of different types
        if (reason) {
            *reason = TfStringPrintf(
                "cannot mix keyframes of different value types; "
                "(adding %s to existing keyframes of type %s)",
                ArchGetDemangled(kfValue.GetTypeid()).c_str(),
                ArchGetDemangled(GetTypeid()).c_str() );
        }
        return false;
    }

    return true;
}

bool
TsSpline::KeyFrameIsInLoopedRange(const TsKeyFrame & kf)
{
    TsLoopParams loopParams = GetLoopParams();
    if (loopParams.GetLooping()) {
        GfInterval loopedInterval =
            loopParams.GetLoopedInterval();
        GfInterval masterInterval =
            loopParams.GetMasterInterval();

        bool inMaster = masterInterval.Contains(kf.GetTime());
        if (loopedInterval.Contains(kf.GetTime()) && !inMaster) {
            return true;
        }
    }

    return false;
}

void
TsSpline::SetKeyFrame( TsKeyFrame kf, GfInterval *intervalAffected )
{
    // Assume none affected
    if (intervalAffected)
        *intervalAffected = GfInterval();

    std::string reason;
    if (!CanSetKeyFrame(kf, &reason)) {
        TF_CODING_ERROR(reason);
        return;
    }

    _Detach();
    _data->SetKeyFrame(kf, intervalAffected);
}

void
TsSpline::RemoveKeyFrame( TsTime time, GfInterval *intervalAffected )
{
    _Detach();
    _data->RemoveKeyFrame(time, intervalAffected);
}

std::optional<TsKeyFrame>
TsSpline::Breakdown(
    double x, TsKnotType type,
    bool flatTangents, double tangentLength,
    const VtValue &value,
    GfInterval *intervalAffected )
{
    // It's not an error to try to beakdown in the unrolled region of a looped
    // spline, but at present it's not supported either.
    if (IsTimeLooped(x))
        return std::optional<TsKeyFrame>();

    TsKeyFrameMap newKeyframes;
    _GetBreakdown( &newKeyframes, x, type, flatTangents, tangentLength, value );

    if (!newKeyframes.empty()) {
        std::string reason;
        TF_FOR_ALL(i, newKeyframes) {
            if (!CanSetKeyFrame(*i, &reason)) {
                TF_CODING_ERROR(reason);
                return std::optional<TsKeyFrame>();
            }
        }

        // Assume none affected
        if (intervalAffected)
            *intervalAffected = GfInterval();

        // Set the keyframes; this will stomp existing
        TF_FOR_ALL(i, newKeyframes) {
            // union together the intervals from each call to SetKeyFrame,
            // which inits is interval arg
            GfInterval interval;
            SetKeyFrame(*i, intervalAffected ? &interval : NULL);
            if (intervalAffected)
                *intervalAffected |= interval;
        }
    }
    
    // Return the newly created keyframe, or the existing one.
    const_iterator i = find(x);
    if (i != end()) {
        return *i;
    } else {
        TF_RUNTIME_ERROR("Failed to find keyframe: %f", x);
        return std::optional<TsKeyFrame>();
    }
}

void
TsSpline::_GetBreakdown(
    TsKeyFrameMap* newKeyframes,
    double x, TsKnotType type,
    bool flatTangents, double tangentLength,
    const VtValue &value) const
{
    newKeyframes->clear();

    // Check for existing key frame
    TsKeyFrameMap const & keyframes = GetKeyFrames();
    TsKeyFrameMap::const_iterator i = keyframes.find(x);
    if (i != keyframes.end()) {
        return;
    }


    // If there are no key frames then we're just inserting.
    if (keyframes.empty()) {
        // XXX: we don't know the value type so we assume double
        VtValue kfValue = value.IsEmpty() ? VtValue( 0.0 ) : value;

        (*newKeyframes)[x] = TsKeyFrame( x, kfValue, type,
                                           VtValue(), VtValue(),
                                           tangentLength, tangentLength);
        return;
    }

    // If we have a valid value, use it for the new keyframe; otherwise,
    // evaluate the spline at the given time.
    VtValue kfValue = value.IsEmpty() ? Eval( x ) : value;

    // Non-Bezier types have no tangents so we have enough to return a key
    // frame.  We can also return if the value type doesn't support tangents.
    if (type != TsKnotBezier ||
        !keyframes.begin()->SupportsTangents()) {
        (*newKeyframes)[x] = TsKeyFrame( x, kfValue, type );
        return;
    }

    // Fallback to flat tangents
    VtValue slope = keyframes.begin()->GetZero();

    // Whether we are breaking down on a frame that is before or after
    // all knots.
    const bool leftOfAllKnots  = (x < keyframes. begin()->GetTime());
    const bool rightOfAllKnots = (x > keyframes.rbegin()->GetTime());

    if (!flatTangents) {
        // If we are breaking down before all knots and we have linear
        // interpolation, use slope of the extrapolation.
        if (leftOfAllKnots) {
            if (GetExtrapolation().first == TsExtrapolationLinear) {
                slope = EvalDerivative(x);
            }
        }
        // Analogously if after all knots.
        if (rightOfAllKnots) {
            if (GetExtrapolation().second == TsExtrapolationLinear) {
                slope = EvalDerivative(x);
            }
        }                
    }

    // Insert the knot, possibly subsequently amending tangents to preserve
    // shape
    (*newKeyframes)[x] = TsKeyFrame( x, kfValue, type,
                                       slope, slope,
                                       tangentLength, tangentLength );

    // If we want flat tangents then we're done. Similarly, if we are
    // breaking down before or after all knots.
    if (flatTangents || leftOfAllKnots || rightOfAllKnots) {
        return;
    }
    
    // Copy the neighboring key frames into newKeyframes
    i = keyframes.upper_bound(x);
    newKeyframes->insert(*i);
    --i;
    newKeyframes->insert(*i);

    // We now have the three key frames of interest in newKeyframes
    // They're correct except we may need to change the length of
    // the right side tangent of the first, the length of the left
    // side tangent of the third and the slope and length on both
    // sides of the middle.  Ts_Breakdown() does that.
    Ts_Breakdown(newKeyframes);
}

void
TsSpline::Breakdown(
    const std::set<double> &times,
    TsKnotType type,
    bool flatTangents,
    double tangentLength,
    const VtValue &value,
    GfInterval *intervalAffected,
    TsKeyFrameMap *keyFramesAtTimes)
{
    std::vector<double> timesVec(times.begin(), times.end());
    std::vector<VtValue> values(times.size(), value);

    _BreakdownMultipleValues(timesVec, type, flatTangents,
        tangentLength, values, intervalAffected, keyFramesAtTimes);
}

void
TsSpline::Breakdown(
    const std::vector<double> &times,
    TsKnotType type,
    bool flatTangents,
    double tangentLength,
    const std::vector<VtValue> &values,
    GfInterval *intervalAffected,
    TsKeyFrameMap *keyFramesAtTimes)
{
    _BreakdownMultipleValues(times, type, flatTangents,
        tangentLength, values, intervalAffected, keyFramesAtTimes);
}

void
TsSpline::Breakdown(
    const std::vector<double> &times,
    const std::vector<TsKnotType> &types,
    bool flatTangents,
    double tangentLength,
    const std::vector<VtValue> &values,
    GfInterval *intervalAffected,
    TsKeyFrameMap *keyFramesAtTimes)
{
    _BreakdownMultipleKnotTypes(times, types, flatTangents,
        tangentLength, values, intervalAffected, keyFramesAtTimes);
}

void
TsSpline::_BreakdownMultipleValues(
    const std::vector<double> &times,
    TsKnotType type,
    bool flatTangents,
    double tangentLength,
    const std::vector<VtValue> &values,
    GfInterval *intervalAffected,
    TsKeyFrameMap *keyFramesAtTimes)
{
    if (times.size() != values.size()) {
        TF_CODING_ERROR("Number of times and values do not match");
        return;
    }

    std::vector<TsKnotType> types(times.size(), type);

    _BreakdownMultipleKnotTypes(times, types, flatTangents,
        tangentLength, values, intervalAffected, keyFramesAtTimes);
}

void
TsSpline::_BreakdownMultipleKnotTypes(
    const std::vector<double> &times,
    const std::vector<TsKnotType> &types,
    bool flatTangents,
    double tangentLength,
    const std::vector<VtValue> &values,
    GfInterval *intervalAffected,
    TsKeyFrameMap *keyFramesAtTimes)
{
    if (!(times.size() == types.size() && times.size() == values.size())) {
        TF_CODING_ERROR("Numbers of times, values and knot types do not match");
        return;
    }

    // We need to separate the key frames we're going to breakdown by knot type
    // so that we can breakdown the beziers first. Beziers need to be broken
    // down first because, when flatTangents aren't enabled, a bezier tries to
    // preserve the shape of the spline as much as possible (see Ts_Breakdown in
    // Ts/EvalUtils). So, we need to "lock down" the shape of the spline with
    // beziers first then breakdown the other types. Additionally, only bezier
    // knots have authored info that can change the shape of the spline at
    // breakdown, i.e. flatTangents and tangentLength. For the other knot types
    // those values are either computed or irrelevant. Thus, we must treat
    // beziers separately and can treat the other types the same.

    // Sample the spline or look up the given value at the given times before 
    // making any modifications, and store the samples per knot type. 
    std::map<TsKnotType, _Samples> samplesPerKnotType;

    const TsKeyFrameMap &keyFrames = GetKeyFrames();

    for (size_t index = 0; index < times.size(); ++index) {
        const double x = times[index];
        const VtValue &value = values[index];
        const TsKnotType type = types[index];

        TsKeyFrameMap::const_iterator i = keyFrames.find(x);
        if (i != keyFrames.end()) {
            // Save the existing keyframe in the result.
            if (keyFramesAtTimes) {
                (*keyFramesAtTimes)[x] = *i;
            }
        } else {
            // Only need to sample where keyframes don't already exist.
            if (value.IsEmpty()) {
                samplesPerKnotType[type].push_back(std::make_pair(x, Eval(x)));
            } else {
                samplesPerKnotType[type].push_back(std::make_pair(x, value));
            }
        }
    }

    // Reset the affected interval, if applicable.
    if (intervalAffected) {
        *intervalAffected = GfInterval();
    }

    // Breakdown any bezier knots first. 
    if (samplesPerKnotType.count(TsKnotBezier) > 0) {
        _BreakdownSamples(samplesPerKnotType[TsKnotBezier], TsKnotBezier, 
            flatTangents, tangentLength, intervalAffected, keyFramesAtTimes);
    } 

    // Breakdown the remaining knot types. 
    for (const auto &mapIt : samplesPerKnotType) {
        TsKnotType type = mapIt.first;
        if (type != TsKnotBezier) {
            _BreakdownSamples(mapIt.second, type, flatTangents, tangentLength,
                intervalAffected, keyFramesAtTimes);
        }
    }
}

void
TsSpline::_BreakdownSamples(
    const _Samples &samples, 
    TsKnotType type, 
    bool flatTangents,
    double tangentLength,
    GfInterval *intervalAffected,
    TsKeyFrameMap *keyFramesAtTimes)
{
    // Perform the Breakdown on the given samples.
    for (const auto &sample : samples) {
        GfInterval interval;
        std::optional<TsKeyFrame> kf = Breakdown(
            sample.first, type, flatTangents, tangentLength, sample.second,
            intervalAffected ? &interval : NULL);

        if (keyFramesAtTimes && kf) {
            // Save the new keyframe in the result.
            (*keyFramesAtTimes)[sample.first] = *kf;
        }

        if (intervalAffected) {
            *intervalAffected |= interval;
        }
    }
}

void
TsSpline::Clear()
{
    TsKeyFrameMap empty;
    // We implement a specialized version of _Detach here.  We're setting
    // the keyframes here which is the heaviest part of the spline.  A normal
    // detach copies the whole spline including the keyframes.  We'd like to
    // avoid that copy since we're going to overwite the keyframes anyway.
    if (!_data.unique()) {
        // Invoke TsSpline_KeyFrames's generalized copy ctor that lets us
        // specify keyframes to copy.
        std::shared_ptr<TsSpline_KeyFrames> newData(
            new TsSpline_KeyFrames(*_data, &empty));
        _data.swap(newData);
    } else {
        // Our data is already unique, just set keyframes.
        _data->SetKeyFrames(empty);
    }
}

std::optional<TsKeyFrame>
TsSpline::GetClosestKeyFrame( TsTime targetTime ) const
{
    const TsKeyFrame *k = Ts_GetClosestKeyFrame(GetKeyFrames(), targetTime);
    if (k) {
        return *k;
    }
    return std::optional<TsKeyFrame>();
}

std::optional<TsKeyFrame>
TsSpline::GetClosestKeyFrameBefore( TsTime targetTime ) const
{
    const TsKeyFrame *k =
        Ts_GetClosestKeyFrameBefore(GetKeyFrames(), targetTime);
    if (k) {
        return *k;
    }
    return std::optional<TsKeyFrame>();
}

std::optional<TsKeyFrame>
TsSpline::GetClosestKeyFrameAfter( TsTime targetTime ) const
{
    const TsKeyFrame *k =
        Ts_GetClosestKeyFrameAfter(GetKeyFrames(), targetTime);
    if (k) {
        return *k;
    }
    return std::optional<TsKeyFrame>();
}

// Note: In the future this could be extended to evaluate the spline, and
//       by doing so we could support removing key frames that are redundant
//       but are not on flat sections of the spline.  Also, doing so would
//       avoid problems where such frames invalidate the frame cache.  If
//       all splines are cubic polynomials, then evaluating the spline at
//       four points, two before the key frame and two after, would be 
//       sufficient to tell if a particular key frame was redundant.
bool
TsSpline::IsKeyFrameRedundant( 
    const TsKeyFrame &keyFrame,
    const VtValue& defaultValue /* = VtValue() */) const
{
    return Ts_IsKeyFrameRedundant(GetKeyFrames(), keyFrame, 
            GetLoopParams(), defaultValue);
}

bool
TsSpline::IsKeyFrameRedundant(
    TsTime keyFrameTime,
    const VtValue &defaultValue /* = VtValue() */ ) const
{
    TsKeyFrameMap const & keyFrames = GetKeyFrames();

    TsKeyFrameMap::const_iterator it = keyFrames.find( keyFrameTime );
    if (it == keyFrames.end()) {
        TF_CODING_ERROR("Time %0.02f doesn't correspond to a key frame!",
                        static_cast<double>(keyFrameTime));
        return false;
    }

    return IsKeyFrameRedundant(*it, defaultValue );
}

bool
TsSpline::HasRedundantKeyFrames(
    const VtValue &defaultValue /* = VtValue() */ ) const
{
    const TsKeyFrameMap& allKeyFrames = GetKeyFrames();

    TF_FOR_ALL(i, allKeyFrames) {
        if (IsKeyFrameRedundant(*i, defaultValue)) {
            return true;
        }
    }

    return false;
}

bool
TsSpline::IsSegmentFlat(
    const TsKeyFrame &kf1,
    const TsKeyFrame &kf2 ) const
{
    return Ts_IsSegmentFlat(kf1, kf2);
}

bool
TsSpline::IsSegmentFlat( TsTime startTime, TsTime endTime ) const
{
    TsKeyFrameMap const & keyFrames = GetKeyFrames();

    TsKeyFrameMap::const_iterator startFrame = keyFrames.find( startTime );
    if (startFrame == keyFrames.end()) {
        TF_CODING_ERROR("Start time %0.02f doesn't correspond to a key frame!",
                        static_cast<double>(startTime));
        return false;
    }

    TsKeyFrameMap::const_iterator endFrame = keyFrames.find( endTime );
    if (endFrame == keyFrames.end()) {
        TF_CODING_ERROR("End time %0.02f doesn't correspond to a key frame!",
                        static_cast<double>(endTime));
        return false;
    }

    return IsSegmentFlat( *startFrame, *endFrame);
}

bool 
TsSpline::IsSegmentValueMonotonic(
    const TsKeyFrame &kf1,
    const TsKeyFrame &kf2 ) const
{
    return Ts_IsSegmentValueMonotonic(kf1, kf2);
}

bool
TsSpline::IsSegmentValueMonotonic(
    TsTime startTime,
    TsTime endTime ) const
{
    TsKeyFrameMap const& keyFrames = GetKeyFrames();

    TsKeyFrameMap::const_iterator startFrame = keyFrames.find( startTime );
    if (startFrame == keyFrames.end()) {
        TF_CODING_ERROR("Start time %0.02f doesn't correspond to a key frame!",
                static_cast<double>(startTime));
        return false;
    }

    TsKeyFrameMap::const_iterator endFrame = keyFrames.find( endTime );
    if (endFrame == keyFrames.end()) {
        TF_CODING_ERROR("End time %0.02f doesn't correspond to a key frame!",
                static_cast<double>(endTime));
        return false;
    }

    return Ts_IsSegmentValueMonotonic(*startFrame, *endFrame);
}

bool 
TsSpline::IsVarying() const
{
    return _IsVarying(/* tolerance */ 0.0);
}

bool 
TsSpline::IsVaryingSignificantly() const
{
    return _IsVarying(/* tolerance */ 1e-6);
}

bool 
TsSpline::_IsVarying(double tolerance) const
{
    const TsKeyFrameMap& keyFrames = GetKeyFrames();

    // Empty splines do not vary
    if (keyFrames.empty()) {
        return false;
    }

    bool isDouble = keyFrames.begin()->GetValue().IsHolding<double>();

    TRACE_FUNCTION();

    // Get the extrapolation settings for the whole spline
    std::pair<TsExtrapolationType, TsExtrapolationType> extrap = 
        GetExtrapolation();

    // Iterate through key frames comparing values.
    // To account for dual-valued knots we need to compare values
    // on each side of a key frame in addition to values across
    // keyframes.
    //
    TsKeyFrameMap::const_iterator prev = keyFrames.end();
    TsKeyFrameMap::const_iterator cur;

    // If this is a double spline, the min/max we've seen so far
    double minDouble = std::numeric_limits<double>::infinity();
    double maxDouble = -std::numeric_limits<double>::infinity();

    // If this is not a double spline, the first value
    VtValue firstValueIfNotDouble;
    if (!isDouble)
        firstValueIfNotDouble = keyFrames.begin()->GetLeftValue();

    for (cur = keyFrames.begin(); cur != keyFrames.end(); ++cur) {
        const TsKeyFrame& curKeyFrame = *cur;

        if (isDouble) {
            // Double case: update min/max and then see if they're too far
            // apart
            double v = curKeyFrame.GetValue().Get<double>();
            minDouble = TfMin(v, minDouble);
            maxDouble = TfMax(v, maxDouble);

            // Check other side if dual valued
            if (curKeyFrame.GetIsDualValued()) {
                double v = curKeyFrame.GetLeftValue().Get<double>();
                minDouble = TfMin(v, minDouble);
                maxDouble = TfMax(v, maxDouble);
            }

            if (maxDouble - minDouble > tolerance)
                return true;

        } else {
            // Non-double, just compare with the first
            if (curKeyFrame.GetValue() != firstValueIfNotDouble)
                return true;

            if (curKeyFrame.GetIsDualValued()) {
                if (curKeyFrame.GetLeftValue() != firstValueIfNotDouble)
                    return true;
            }
        }

        // Check tangents
        if (curKeyFrame.HasTangents()) {
            bool isFirst = (cur == keyFrames.begin());
            bool isLast = (cur == keyFrames.end()-1);

            // Check the left tangent if:
            // - This is the first knot and we have non-held left extrapolation
            // - This is a subsequent knot and the previous knot is non-held
            bool checkLeft = 
                isFirst ? 
                (extrap.first != TsExtrapolationHeld) :
                (prev->GetKnotType() != TsKnotHeld);

            // Check the right tangent if:
            // - This is the last knot and we have non-held right extrapolation
            // - This is an earlier knot and is non-held
            bool checkRight = 
                isLast ? 
                (extrap.second != TsExtrapolationHeld) :
                (curKeyFrame.GetKnotType() != TsKnotHeld);

            // If a tangent that we check has a non-zero slope, 
            // the spline varies
            VtValue zero = curKeyFrame.GetZero();
            if ((checkLeft && curKeyFrame.GetLeftTangentLength() != 0 && 
                 curKeyFrame.GetLeftTangentSlope() != zero) ||
                (checkRight && curKeyFrame.GetRightTangentLength() != 0 && 
                 curKeyFrame.GetRightTangentSlope() != zero)) {
                return true;
            }
        }

        prev = cur;
    }

    return false;
}

void
TsSpline::SetExtrapolation(
    TsExtrapolationType left,
    TsExtrapolationType right)
{
    _Detach();
    _data->SetExtrapolation(TsExtrapolationPair(left, right));
}

std::pair<TsExtrapolationType, TsExtrapolationType>
TsSpline::GetExtrapolation() const
{
    return _data->GetExtrapolation();
}

const std::type_info &
TsSpline::GetTypeid() const
{
    TsKeyFrameMap const & keyframes = GetKeyFrames();
    if (keyframes.empty())
        return typeid(void);
    return keyframes.begin()->GetValue().GetTypeid();
}

TfType
TsSpline::GetType() const
{
    static TfStaticData<TfType> unknown;
    TsKeyFrameMap const & keyframes = GetKeyFrames();
    if (keyframes.empty())
        return *unknown;
    return keyframes.begin()->GetValue().GetType();
}

std::string
TsSpline::GetTypeName() const
{
    return ArchGetDemangled( GetTypeid() );
}

VtValue
TsSpline::Eval( TsTime time, TsSide side ) const
{
    return Ts_Eval(*this, time, side, Ts_EvalValue);
}

// Finds the first keyframe on or before the given time and side, or the first
// key after, if there are no keys on or before the time.
static std::optional<TsKeyFrame>
_FindHoldKey(const TsSpline &spline, TsTime time, TsSide side)
{
    if (spline.IsEmpty()) {
        return {};
    }

    const TsKeyFrameMap &keyframes = spline.GetKeyFrames();

    if (time <= keyframes.begin()->GetTime()) {
        // This is equivalent to held extrapolation for the first keyframe.
        return *keyframes.begin();
    } else {
        TsKeyFrameMap::const_iterator i = keyframes.find(time);

        if (side == TsRight && i != keyframes.end()) {
            // We have found an exact match.
            return *i;
        } else {
            // Otherwise, we are either evaluating the left side of the given
            // time, or there is no exact match. Find the closest prior
            // keyframe.
            return spline.GetClosestKeyFrameBefore(time);
        }
    }
    return {};
}

VtValue
TsSpline::EvalHeld(TsTime time, TsSide side) const
{
    if (IsEmpty()) {
        return VtValue();
    }

    const std::optional<TsKeyFrame> kf = _FindHoldKey(*this, time, side);
    if (!TF_VERIFY(kf)) {
        return VtValue();
    }
    return kf->GetValue();
}

VtValue
TsSpline::EvalDerivative( TsTime time, TsSide side ) const
{
    return Ts_Eval(*this, time, side, Ts_EvalDerivative);
}

bool
TsSpline::DoSidesDiffer(const TsTime time) const
{
    const TsKeyFrameMap &keyFrames = GetKeyFrames();

    // Find keyframe at specified time.  If none, the answer is false.
    const auto kfIt = keyFrames.find(time);
    if (kfIt == keyFrames.end())
        return false;

    // Check whether dual-valued with differing values.
    if (kfIt->GetIsDualValued()
        && kfIt->GetLeftValue() != kfIt->GetValue())
    {
        return true;
    }

    // Check whether preceding segment is held with differing value.
    if (kfIt != keyFrames.begin())
    {
        const auto prevKfIt = kfIt - 1;
        if (prevKfIt->GetKnotType() == TsKnotHeld
            && prevKfIt->GetValue() != kfIt->GetValue())
        {
            return true;
        }
    }

    // If we didn't hit any of the above cases, then this is a keyframe time
    // where our value is identical on left and right sides.
    return false;
}

TsSamples
TsSpline::Sample( TsTime startTime, TsTime endTime,
                   double timeScale, double valueScale,
                   double tolerance ) const
{
    return Ts_Sample( *this, startTime, endTime,
        timeScale, valueScale, tolerance );
}

std::pair<VtValue, VtValue>
TsSpline::GetRange( TsTime startTime, TsTime endTime ) const
{
    return Ts_GetRange( *this, startTime, endTime );
}

bool
TsSpline::IsLinear() const
{
    // If this spline is linear then it will only have two keyframes
    if ( empty() || size() != 2 )
        return false;

    // The two keyframes must also be single valued linear knots for the 
    // spline to be linear.  The output value must also be a double
    TF_FOR_ALL(it, *this) {
        if (it->GetKnotType() != TsKnotLinear || 
            !it->GetValue().IsHolding<double>() ||
            it->GetIsDualValued() ) {
            return false;
        }
    }

    // Extrapolation on both ends need to be linear
    if (GetExtrapolation().first != TsExtrapolationLinear || 
        GetExtrapolation().second != TsExtrapolationLinear) {
        return false;
    }

    // we passed all the linear tests!
    return true;
}

void
TsSpline::BakeSplineLoops()
{
    _Detach();
    _data->BakeSplineLoops();
}

void 
TsSpline::SetLoopParams(const TsLoopParams& params) 
{
    _Detach();
    _data->SetLoopParams(params);
}

TsLoopParams
TsSpline::GetLoopParams() const
{
    return _data->GetLoopParams();
}

bool 
TsSpline::IsTimeLooped(TsTime time) const
{
    const TsLoopParams &params = GetLoopParams();

    return params.GetLooping()
        && params.GetLoopedInterval().Contains(time)
        && !params.GetMasterInterval().Contains(time);
}

TsSpline::const_iterator
TsSpline::find(const TsTime &time) const
{
    return const_iterator(GetKeyFrames().find(time));
}

TsSpline::const_iterator
TsSpline::lower_bound(const TsTime &time) const
{
    return const_iterator(GetKeyFrames().lower_bound(time));
}

TsSpline::const_iterator
TsSpline::upper_bound(const TsTime &time) const
{
    return const_iterator(GetKeyFrames().upper_bound(time));
}

std::ostream& operator<<(std::ostream& out, const TsSpline & val)
{
    out << "Ts.Spline(";
    size_t counter = val.size();
    if (counter > 0) {
        out << "[";
        TF_FOR_ALL(it, val) {
            out << *it;
            counter--;
            out << (counter > 0 ? ", " : "]");
        }
    }
    out << ")";
    return out;
}

PXR_NAMESPACE_CLOSE_SCOPE
