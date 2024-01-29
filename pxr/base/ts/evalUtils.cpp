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
#include "evalUtils.h"

#include "pxr/base/ts/keyFrameUtils.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/gf/interval.h"

#include <limits>

PXR_NAMESPACE_OPEN_SCOPE

TsExtrapolationType
Ts_GetEffectiveExtrapolationType(
        const TsKeyFrame& kf,
        const TsExtrapolationPair &extrapolation,
        bool kfIsOnlyKeyFrame,
        TsSide side)
{
    // Check for held extrapolation
    if ((side == TsLeft  && extrapolation.first  == TsExtrapolationHeld) ||
        (side == TsRight && extrapolation.second == TsExtrapolationHeld)) {
        return TsExtrapolationHeld;
    }

    // Extrapolation is held if key frame is Held
    if (kf.GetKnotType() == TsKnotHeld) {
        return TsExtrapolationHeld;
    }

    // Extrapolation is held if key frame is dual valued and doesn't have
    // tangents (because there's no slope to extrapolate due to the dual
    // value discontinuity).
    if ((!kf.HasTangents() ) &&
            kf.GetIsDualValued()) {
        return TsExtrapolationHeld;
    }

    // Extrapolation is held if there's only one key frame and it
    // doesn't have tangents.
    if ((!kf.HasTangents()) && kfIsOnlyKeyFrame) {
        return TsExtrapolationHeld;
    }

    // Use extrapolation on spline
    return (side == TsLeft) ? extrapolation.first : extrapolation.second;
}

TsExtrapolationType
Ts_GetEffectiveExtrapolationType(
        const TsKeyFrame& kf,
        const TsSpline &spline,
        TsSide side)
{
    return Ts_GetEffectiveExtrapolationType(kf,spline.GetExtrapolation(),
        spline.size() == 1, side);
}

////////////////////////////////////////////////////////////////////////

static VtValue
_GetSlope(
        TsTime time,
        TsSpline::const_iterator i,
        const TsSpline & val,
        TsSide side)
{
    const TsKeyFrame &kf = *i;
    VtValue slope;
    switch (Ts_GetEffectiveExtrapolationType(kf, val, side)) {
        default:
        case TsExtrapolationHeld:
            slope = kf.GetZero();
            break;

        case TsExtrapolationLinear:
            if (kf.HasTangents() ) {
                slope = (side == TsLeft) ? kf.GetLeftTangentSlope() :
                    kf.GetRightTangentSlope();
            }
            else {
                // Set i and j to the left and right key frames of the segment
                // with the slope we want to extrapolate.
                TsSpline::const_iterator j = i;
                if (side == TsLeft) {
                    // i is on the left so move j to the right
                    ++j;
                }
                else {
                    // i is on the right so move it to the left
                    --i;
                }
                slope = Ts_GetKeyFrameData(*i)->GetSlope(*Ts_GetKeyFrameData(*j));
            }
            break;
    }

    return slope;
}

static VtValue
_Extrapolate(
        TsTime time,
        TsSpline::const_iterator i,
        const TsSpline &val,
        TsSide side)
{
    VtValue slope = _GetSlope(time, i, val, side);
    const TsKeyFrame& kf = *i;
    VtValue value = (side == TsLeft) ? kf.GetLeftValue() : kf.GetValue();
    TsTime dt   = time - kf.GetTime();

    return Ts_GetKeyFrameData(kf)->Extrapolate(value,dt,slope);
}

static VtValue
_ExtrapolateDerivative(
        TsTime time,
        TsSpline::const_iterator i,
        const TsSpline &val,
        TsSide side)
{
    return _GetSlope(time, i, val, side);
}

VtValue
Ts_Eval(
    const TsSpline &val,
    TsTime time, TsSide side,
    Ts_EvalType evalType)
{
    if (val.empty()) {
        return VtValue();
    }

    // XXX: do we want any snapping here?  divide-by-zero avoidance?
    // The following code was in Presto; not sure it belongs in Ts.  In
    // particular, we shouldn't assume integer knot times.
    //
    // If very close to a knot, eval at the knot.  Because of dual
    // valued knots, all sorts of trouble can result if you mean to
    // sample at a knot time and are actually epsilon off.
    //TsTime rounded = round(time);
    //if (fabs(rounded-time) < ARCH_MIN_FLOAT_EPS_SQR)
    //    time = rounded;

    // Get the keyframe after time
    TsSpline::const_iterator iAfterTime = val.upper_bound(time);
    TsSpline::const_iterator i = iAfterTime;

    // Check boundary cases
    if (i == val.begin()) {
        // Before first keyframe.  Extrapolate to the left.
        return (evalType == Ts_EvalValue) ?
            _Extrapolate(time, i, val, TsLeft) :
            _ExtrapolateDerivative(time, i, val, TsLeft);
    }
    // Note if at or after last keyframe.
    bool last = (i == val.end());

    // Get the keyframe at or before time
    --i;

    if (i->GetTime() == time && side == TsLeft) {
        // Evaluate at a keyframe on the left.  If the previous
        // keyframe is held then use the right side of the previous
        // keyframe.
        if (i != val.begin()) {
            TsSpline::const_iterator j = i;
            --j;
            if (j->GetKnotType() == TsKnotHeld) {
                return (evalType == Ts_EvalValue) ?
                    j->GetValue() :
                    j->GetValueDerivative();
            }
        }
        // handle derivatives of linear knots at keyframes differently
        if (i->GetKnotType() == TsKnotLinear && 
            evalType == Ts_EvalDerivative) {
            // if we are next to last, eval from the right, 
            // otherwise use the specified direction
            return _GetSlope(time, i, val, last && (side == TsLeft) ?
                             TsRight :
                             side);
        }
        return (evalType == Ts_EvalValue) ?
            i->GetLeftValue() :
            i->GetLeftValueDerivative();
    }
    else if (last) {
        // After last key frame.  Extrapolate to the right.
        return (evalType == Ts_EvalValue) ?
            _Extrapolate(time, i, val, TsRight) :
            _ExtrapolateDerivative(time, i, val, TsRight);
    }
    else if (i->GetTime() == time) {
        // Evaluate at a keyframe on the right
        // handle derivatives of linear knots at keyframes differently
        if (i->GetKnotType() == TsKnotLinear 
            && evalType == Ts_EvalDerivative)
        {
            return _GetSlope(time, i, val, 
                             (i == val.begin() && side == TsRight) ?
                             TsLeft : side);
        }
        return (evalType == Ts_EvalValue) ?
            i->GetValue() :
            i->GetValueDerivative();
    }
    else {
        // Evaluate at a keyframe on the right or between keyframes
        return (evalType == Ts_EvalValue) ?
            Ts_UntypedEvalCache::EvalUncached(*i, *iAfterTime, time) :
            Ts_UntypedEvalCache::EvalDerivativeUncached(
                *i, *iAfterTime, time);
    }
}

// For the routine below, define loose comparisons to account for precision
// errors.  This epsilon value is always used on the parameter space [0, 1],
// meaning it has the same effect no matter what the domain and range of the
// segment are.
#define EPS 1e-6
#define LT(a,b) ((b)-(a) > EPS)

bool
Ts_IsSegmentValueMonotonic( const TsKeyFrame &kf1, const TsKeyFrame &kf2 )
{
    bool monotonic = false;
    VtValue kf2LeftVtVal             = kf2.GetLeftValue();
    VtValue kf1VtVal                 = kf1.GetValue();
    VtValue kf2LeftTangentSlopeVtVal = kf2.GetLeftTangentSlope();
    VtValue kf1RightTangentSlopeVtVal= kf1.GetRightTangentSlope();

    if (kf1.GetTime() >= kf2.GetTime()) {
        TF_CODING_ERROR("The first key frame must come before the second.");
        return false;
    }

    if (kf1.GetKnotType() == TsKnotBezier &&
        kf2.GetKnotType() == TsKnotBezier &&
        kf1VtVal.IsHolding<double>() &&
        kf2LeftVtVal.IsHolding<double>() && 
        kf1RightTangentSlopeVtVal.IsHolding<double>() &&
        kf2LeftTangentSlopeVtVal.IsHolding<double>())
    {
        monotonic = true;
        //get Bezier control points
        double x0 = kf1VtVal.Get<double>(); 
        double x1 = kf1VtVal.Get<double>() + 
                    ( kf1.GetRightTangentSlope().Get<double>() *
                      kf1.GetRightTangentLength());
        double x2 =  kf2LeftVtVal.Get<double>() - 
                     ( kf2.GetLeftTangentSlope().Get<double>() * 
                       kf2.GetLeftTangentLength());
        double x3 =  kf2LeftVtVal.Get<double>(); 
        // By taking the derivative of Bezier curve equation we obtain:
        //
        // f'(x0 + (-3x0+3x1)*t + (-x0+3x1-3x2+x3)*t^2+(-3x0+9x1-9x2+3x3)*t^3)= 
        //  (-3x0 + 9x1-9x2+3x3)*t^2 + (6x0-12x1-6x2)*t+(-3x0+3x1) 
        //
        //  (-3x0 + 9x1-9x2+3x3)*t^2 + (6x0-12x1-6x2)*t+(-3x0+3x1) =0
        //
        //  divide by 3:
        //  (-x0 + 3x1-3x2+x3)*t^2 + (2x0-4x1-2x2)*t+(-x0+x1)=0
        double a = -x0+(3*x1)-(3*x2)+x3;
        double b = (2*x0)-(4*x1)+(2*x2);
        double c = -x0+x1;
        double polyDeriv[3] = {c, b, a};
        double root0 = 0.0, root1 = 0.0;

        //  compute the roots of the equation
        //  using _SolveQuadratic() to solve for the roots
        if (Ts_SolveQuadratic(polyDeriv, &root0, &root1)) {

            //IF we have a parabola there will be only one maximum/minimum
            // if a == 0, than the cubic term of the bezier equation is 
            // zero as well, giving us the quadratic bezier curve. 
            if ((GfIsClose(a, 0, EPS)) && (LT(0, root0) && LT(root0, 1))) {
                monotonic = false;
            }
            //IF we have a hyperbola there can be two maxima/minima
            //IF two roots are equal: we have a point where the slope
            // becomes horizontal but than it continues in the way it was
            // before the point (monotonic = true)
            //IF two roots are different: and either of them falls in the 
            // range between 0 and 1 we have a maximum/minimum
            // (monotonic = false)
            else if ((!GfIsClose(root0, root1, EPS)) && 
                     ((LT(0, root0) && LT(root0, 1)) || 
                      (LT(0, root1) && LT(root1, 1))) )
            {
                monotonic = false; 
            }
        }
    }
    return monotonic;
}

////////////////////////////////////////////////////////////////////////
// Functions shared by piecewise linear sampling and range functions

static
std::pair<TsSpline::const_iterator, TsSpline::const_iterator>
_GetBounds( const TsSpline & val, TsTime startTime, TsTime endTime )
{
    if (startTime > endTime) {
        TF_CODING_ERROR("invalid interval (start > end)");
        return std::make_pair(val.end(), val.end());
    }

    // Find the bounding keyframes.  We first find the keyframe at or before
    // the startTime and the keyframe after endTime to determine the segments.
    // If there is no keyframe at or before startTime we use the first keyframe
    // and if there is no keyframe after endTime we use the last keyframe.
    // This function assumes there's at least one keyframe.
    TsSpline::const_iterator i = val.upper_bound(startTime);
    if (i != val.begin()) {
        --i;
    }
    TsSpline::const_iterator j = val.upper_bound(endTime);
    if (j == val.end()) {
        --j;
    }

    return std::make_pair(i, j);
}

////////////////////////////////////////////////////////////////////////
// Range functions

// Note that if there is a knot at endTime that is discontinuous, its right
// side will be ignored
template<typename T>
    static std::pair<T, T>
_GetBezierRange( const Ts_Bezier<T>* bezier,
        double startTime, double endTime )
{
    T min =  std::numeric_limits<T>::infinity();
    T max = -std::numeric_limits<T>::infinity();

    // Find the limits of the spline parameter within [startTime,endTime).
    double uMin = 0.0, uMax = 1.0;
    if (startTime > bezier->timePoints[0] || endTime < bezier->timePoints[3]) {
        if (startTime > bezier->timePoints[0]) {
            uMin = GfClamp(Ts_SolveCubic(bezier->timeCoeff,
                        startTime), 0.0, 1.0);
        }
        if (endTime < bezier->timePoints[3]) {
            uMax = GfClamp(Ts_SolveCubic(bezier->timeCoeff,
                        endTime), 0.0, 1.0);
        }
        if (uMin > uMax) {
            uMin = uMax;
        }
    }

    // Get initial bounds from the endpoints.
    if (uMin == 0.0) {
        min = GfMin(min, bezier->valuePoints[0]);
        max = GfMax(max, bezier->valuePoints[0]);
    }
    else {
        T y = Ts_EvalCubic(bezier->valueCoeff, uMin);
        min = GfMin(min, y);
        max = GfMax(max, y);
    }
    if (uMax == 1.0) {
        min = GfMin(min, bezier->valuePoints[3]);
        max = GfMax(max, bezier->valuePoints[3]);
    }
    else {
        T y = Ts_EvalCubic(bezier->valueCoeff, uMax);
        min = GfMin(min, y);
        max = GfMax(max, y);
    }

    // Find the roots of the derivative of the value Bezier.  The values
    // at these points plus the end points are the candidates for the
    // min and max.
    double valueDeriv[3], root0, root1;
    Ts_CubicDerivative(bezier->valueCoeff, valueDeriv);
    if (Ts_SolveQuadratic(valueDeriv, &root0, &root1)) {
        if (root0 > uMin && root0 < uMax) {
            T y = Ts_EvalCubic(bezier->valueCoeff, root0);
            min = GfMin(min, y);
            max = GfMax(max, y);
        }
        if (root1 > uMin && root1 < uMax) {
            T y = Ts_EvalCubic(bezier->valueCoeff, root1);
            min = GfMin(min, y);
            max = GfMax(max, y);
        }
    }

    return std::make_pair(min, max);
}

// Note that if there is a knot at endTime that is discontinuous, its right
// side will be ignored
template<typename T>
static std::pair<T, T>
_GetSegmentRange(const Ts_EvalCache<T> * cache,
        double startTime, double endTime )
{
    return _GetBezierRange<T>(cache->GetBezier(), startTime, endTime);
}

template<typename T>
static std::pair<VtValue, VtValue>
_GetCurveRange( const TsSpline & val, double startTime, double endTime )
{
    T min =  std::numeric_limits<T>::infinity();
    T max = -std::numeric_limits<T>::infinity();

    // Find the latest key that's <= startTime; if all are later, use the
    // first key.
    //
    // This returns the first key that's > startTime.
    TsSpline::const_iterator i = val.upper_bound(startTime);
    if (i == val.begin()) {
        // All are > startTime; include left side of first keyframe
        T v = i->GetLeftValue().template Get<T>();
        min = GfMin(min, v);
        max = GfMax(max, v);
    }
    else {
        // The one before must be <= startTime
        --i;
    }

    // Normally, we don't have to do anything to include the value of right
    // side of the last knot that's wihin the range, but there are a couple of
    // cases where we do have to force this, below.
    bool forceRightSideOfLowerBound = false;

    // Find the earliest key that's >= endTime; if all are earlier, use the
    // latest.
    //
    // This returns the earliest key that's >= endTime.
    TsSpline::const_iterator j = val.lower_bound(endTime);
    if (j == val.end()) {
        // all are < endTime; for j, use the latest and make sure we include
        // its right side below
        --j;
        forceRightSideOfLowerBound = true;
    }

    // The other case where we need to force inclusion of the right side of
    // the last knot is when it's at endTime, and it's discontinuous.
    // (_GetSegmentRange below deals in bezier's which can't be
    // discontinuous, and so it does not consider the right side of
    // discontiguous knots at the right boundary.)
    if (!forceRightSideOfLowerBound && j->GetTime() == endTime) {
        if (j->GetIsDualValued())
            forceRightSideOfLowerBound = true;
        // Is prev knot held?
        else if (j != val.begin()) {
            TsSpline::const_iterator k = j;
            k--;
            if (k->GetKnotType() == TsKnotHeld)
                forceRightSideOfLowerBound = true;
        }
    }

    // If right side forced, include it now
    if (forceRightSideOfLowerBound) {
        T v = j->GetValue().template Get<T>();
        min = GfMin(min, v);
        max = GfMax(max, v);
    }

    // Handle the keyframe segments in the interval, excluding the region
    // (if any) past the end of the last keyframe, as this region is always
    // held, and its range would not contribute to the total range.
    TsSpline::const_iterator i2 = i; i2++;
    while (i != j) {
        if (i2 != val.end()) {
            Ts_EvalCache<T> cache(*i, *i2);

            std::pair<T, T> range =
                _GetSegmentRange<T>(&cache, startTime, endTime);
            min = GfMin(min, range.first);
            max = GfMax(max, range.second);
        } 

        i = i2;
        i2++;
    }

    return std::make_pair(VtValue(min), VtValue(max));
}

std::pair<VtValue, VtValue>
Ts_GetRange( const TsSpline & val, TsTime startTime, TsTime endTime )
{
    if (startTime > endTime) {
        TF_CODING_ERROR("invalid interval (start > end)");
        return std::make_pair(VtValue(), VtValue());
    }

    if (val.IsEmpty()) {
        return std::make_pair(VtValue(), VtValue());
    }

    // Range at a point is just the value at that point.  We want to
    // ignore extrapolation so ensure we're within the interval covered
    // by key frames.
    if (startTime == endTime) {
        if (startTime < val.begin()->GetTime()) {
            VtValue y = val.Eval(val.begin()->GetTime(), TsLeft);
            return std::make_pair(y, y);
        }
        else if (startTime >= val.rbegin()->GetTime()) {
            VtValue y = val.Eval(val.rbegin()->GetTime(), TsRight);
            return std::make_pair(y, y);
        }
        else {
            VtValue y = val.Eval(startTime, TsRight);
            return std::make_pair(y, y);
        }
    }

    // Get the range over the segments
    const std::type_info & t = val.GetTypeid();
    if (TfSafeTypeCompare(t, typeid(double))) {
        return _GetCurveRange<double>(val, startTime, endTime);
    }
    else if (TfSafeTypeCompare(t, typeid(float))) {
        return _GetCurveRange<float>(val, startTime, endTime);
    }
    else {
        // Cannot interpolate
        return std::make_pair(VtValue(), VtValue());
    }
}

////////////////////////////////////////////////////////////////////////
// Piecewise linear sampling functions

// Determine how far the inner Bezier polygon points are from the line
// connecting the outer points.  Return the maximum distance.
template <typename T>
static double
_BezierHeight( const TsTime timeBezier[4], const T valueBezier[4],
        double timeScale, double valueScale )
{
    T dv         = (valueBezier[3] - valueBezier[0]) * valueScale;
    TsTime dt  = ( timeBezier[3] -  timeBezier[0]) * timeScale;
    T dv1        = (valueBezier[1] - valueBezier[0]) * valueScale;
    TsTime dt1 = ( timeBezier[1] -  timeBezier[0]) * timeScale;
    T dv2        = (valueBezier[2] - valueBezier[0]) * valueScale;
    TsTime dt2 = ( timeBezier[2] -  timeBezier[0]) * timeScale;

    double len = dv * dv + dt * dt;

    double t1 = (dv1 * dv + dt1 * dt) / len;
    double t2 = (dv2 * dv + dt2 * dt) / len;

    double d1 = hypot(dv1 - t1 * dv, dt1 - t1 * dt);
    double d2 = hypot(dv2 - t2 * dv, dt2 - t2 * dt);

    return GfMax(d1, d2);
}

template <typename T>
static void
_SubdivideBezier( const T inBezier[4], T outBezier[4], double u, bool leftSide)
{
    if (leftSide) {
        // Left Bezier
        T mid        = GfLerp(u, inBezier[1], inBezier[2]);
        T tmp1       = GfLerp(u, inBezier[2], inBezier[3]);
        T tmp0       = GfLerp(u, mid, tmp1);
        outBezier[0] = inBezier[0];
        outBezier[1] = GfLerp(u, inBezier[0], inBezier[1]);
        outBezier[2] = GfLerp(u, outBezier[1], mid);
        outBezier[3] = GfLerp(u, outBezier[2], tmp0);
    }
    else {
        // Right Bezier
        T mid        = GfLerp(u, inBezier[1], inBezier[2]);
        T tmp1       = GfLerp(u, inBezier[0], inBezier[1]);
        T tmp0       = GfLerp(u, tmp1, mid);
        outBezier[3] = inBezier[3];
        outBezier[2] = GfLerp(u, inBezier[2], inBezier[3]);
        outBezier[1] = GfLerp(u, mid, outBezier[2]);
        outBezier[0] = GfLerp(u, tmp0, outBezier[1]);
    }
}

// Sample a pair of Beziers (value and time) with results in samples.
template <typename T>
static void
_SampleBezier( const TsTime timeBezier[4], const T valueBezier[4],
        double startTime, double endTime,
        double timeScale, double valueScale, double tolerance,
        TsSamples & samples )
{
    // Beziers have the convex hull property and are easily subdivided.
    // We use the convex hull to determine if a linear interpolation is
    // sufficently accurate and, if not, we subdivide and recurse.  If
    // timeBezier is outside the time domain then we simply discard it.

    // Discard if left >= right.  If this happens it should only be by
    // a tiny amount due to round off error.
    if (timeBezier[0] >= timeBezier[3]) {
        return;
    }

    // Discard if outside the domain
    if (timeBezier[0] >= endTime ||
            timeBezier[3] <= startTime) {
        return;
    }

    // Find the distance from the inner points of the Bezier polygon to
    // the line connecting the outer points.  If the larger of these
    // distances is smaller than tolerance times some factor then we
    // decide that the Bezier is flat and we sample it with a line,
    // otherwise we subdivide.
    //
    // Since the Bezier cannot reach its inner convex hull vertices, the
    // distances to those vertices is an overestimate of the error.  So
    // we increase the tolerance by some factor determined by what works.
    static const double toleranceFactor = 1.0;
    double e = _BezierHeight(timeBezier, valueBezier, timeScale, valueScale);
    if (e <= toleranceFactor * tolerance) {
        // Linear approximation
        samples.push_back(TsValueSample(timeBezier[0],
                    VtValue(valueBezier[0]),
                    timeBezier[3],
                    VtValue(valueBezier[3])));
    }

    // Blur sample if we're below the tolerance in time
    else if (timeScale * (timeBezier[3] - timeBezier[0]) <= tolerance) {
        Ts_Bezier<T> tmpBezier(timeBezier, valueBezier);
        std::pair<T, T> range =
            _GetBezierRange<T>(&tmpBezier, startTime, endTime);
        samples.push_back(
                TsValueSample(GfMax(timeBezier[0], startTime),
                    VtValue(range.first),
                    GfMin(timeBezier[3], endTime),
                    VtValue(range.second),
                    true));
    }

    // Subdivide
    else {
        T leftValue[4], rightValue[4];
        TsTime leftTime[4], rightTime[4];
        _SubdivideBezier(valueBezier, leftValue,  0.5,  true);
        _SubdivideBezier(timeBezier,  leftTime,   0.5,  true);
        _SubdivideBezier(valueBezier, rightValue, 0.5, false);
        _SubdivideBezier(timeBezier,  rightTime,  0.5, false);

        // Recurse
        _SampleBezier(leftTime, leftValue, startTime, endTime,
                timeScale, valueScale, tolerance, samples);
        _SampleBezier(rightTime, rightValue, startTime, endTime,
                timeScale, valueScale, tolerance, samples);
    }
}

template <typename T>
static void
_SampleBezierClip( const TsTime timePoly[4],
        const TsTime timeBezier[4], const T valueBezier[4],
        double startTime, double endTime,
        double timeScale, double valueScale, double tolerance,
        TsSamples & samples )
{
    static const double rootTolerance = 1.0e-10;

    // Check to see if the first derivative ever goes to 0 in the interval
    // [0,1].  If it does then the cubic is not monotonically increasing
    // in that interval.
    double root0 = 0, root1 = 1;
    double timeDeriv[3];
    Ts_CubicDerivative( timePoly, timeDeriv );
    if (Ts_SolveQuadratic( timeDeriv, &root0, &root1 )) {
        if (root0 >= 0.0 - rootTolerance &&
                root1 <= 1.0 + rootTolerance) {
            // Bezier doubles back on itself in the interval.  We
            // subdivide the Bezier into a segment somewhere before
            // the double back and a segment after it such that the
            // first ends (in time) exactly where the second begins.

            // First compute at what time we should subdivide.  We take
            // the average of the times where the derivative is zero
            // clamped to the values at the end points.
            TsTime t0 = Ts_EvalCubic(timePoly, root0);
            TsTime t1 = Ts_EvalCubic(timePoly, root1);
            TsTime t  = 0.5 * (GfClamp(t0, timeBezier[0], timeBezier[3]) +
                    GfClamp(t1, timeBezier[0], timeBezier[3]));

            // If t0 < t1 then it's the interval [root0,root1] where the
            // curve is monotonically increasing and not the intervals
            // [0,root0] and [root1,1].  This can happen if the Bezier
            // has zero length tangents and in that case [root0,root1]
            // should be [0,1].  (It will also happen if the tangents
            // are pointing in the wrong direction but that violates
            // our assumptions so we don't handle it.)  Since [0,1] is
            // the whole segment we'll just evaluate normally in that
            // case.
            if (t0 >= t1) {
                // Find the solutions for t in the intervals [0,root0]
                // and [root1,1].  These are the parameters where we
                // subdivide the Bezier.
                root0 = Ts_SolveCubicInInterval(timePoly, timeDeriv, t,
                        GfInterval(0, root0));
                root1 = Ts_SolveCubicInInterval(timePoly, timeDeriv, t,
                        GfInterval(root1, 1));

                // Now compute the Bezier from 0 to root0 and the Bezier
                // from root1 to 1.  The former ends on t and the latter
                // begins on t and both are monotonically increasing.
                //
                T leftValue[4], rightValue[4];
                TsTime leftTime[4], rightTime[4];
                _SubdivideBezier(valueBezier, leftValue,  root0,  true);
                _SubdivideBezier(timeBezier,  leftTime,   root0,  true);
                _SubdivideBezier(valueBezier, rightValue, root1, false);
                _SubdivideBezier(timeBezier,  rightTime,  root1, false);

                // Left curve ends and right curve begins at t.
                leftTime[3]  = t;
                rightTime[0] = t;

                // Now evaluate the Beziers.  Since the left Bezier will end
                // at exactly the time the right Bezier starts but they end
                // and start at different values there'll be a gap in the
                // samples.  Technically that gap is real but we don't want
                // it anyway so we'll slightly shorten the last sample and
                // add a new one to bridge the gap.  We also need to handle
                // the situation where either the left or right interval
                // generates no samples.  We still bridge the gap but we
                // compute the extra sample differently in each case.
                _SampleBezier(leftTime, leftValue, startTime, endTime,
                        timeScale, valueScale, tolerance, samples);
                size_t numSamples1 = samples.size();
                if (numSamples1 > 0) {
                    // We may need to add sample across the gap between the
                    // left and right sides so add it here now so it stays
                    // in time order.  We will remove it or adjust its values
                    // after we sample the right side.
                    samples.push_back(
                        TsValueSample(0, VtValue(), 0, VtValue()));
                    numSamples1++;
                }

                _SampleBezier(rightTime, rightValue, startTime, endTime,
                        timeScale, valueScale, tolerance, samples);
                size_t numSamples2 = samples.size();

                // If there are no left samples (we check against 2 because we
                // also added a gap sample if there were left samples)
                if (numSamples1 < 2) {
                    return;
                }

                if (numSamples1 != numSamples2) {
                    // Samples in right interval and there are samples
                    // before the right interval.
                    TsValueSample& s = samples[numSamples1 - 2];
                    TsValueSample& gap = samples[numSamples1 - 1];
                    TsTime d = GfMin(0.001,
                            0.001 * (s.rightTime - s.leftTime));
                    s.rightTime -= d;

                    // Update the gap closing sample with the correct values
                    gap.leftTime = s.rightTime;
                    gap.leftValue = VtValue(s.rightValue);
                    gap.rightTime = rightTime[0];
                    gap.rightValue = VtValue(rightValue[0]);
                }
                else {
                    // No samples in right interval but there are samples.
                    // Add a sample across the gap only if the gap is in
                    // the sampled domain.  If not then the left Bezier
                    // wasn't sampled up to where the gap is.
                    TsValueSample& s = samples[numSamples1 - 2];
                    if (s.rightTime < endTime) {
                        TsValueSample& gap = samples[numSamples1 - 1];
                        TsTime d = GfMin(0.001,
                                0.001 * (s.rightTime - s.leftTime));
                        s.rightTime -= d;
                        // Update the gap closing sample with the correct values
                        gap.leftTime = s.rightTime;
                        gap.leftValue = VtValue(s.rightValue);
                        gap.rightTime = rightTime[3];
                        gap.rightValue = VtValue(rightValue[3]);
                    } else {
                        // Delete the gap closing sample as it is unneeded.
                        samples.pop_back();
                    }
                }

                return;
            }
        }
    }

    // Bezier does not double back on itself
    _SampleBezier(timeBezier, valueBezier, startTime, endTime,
            timeScale, valueScale, tolerance, samples);
}

// Sample segment with results in samples.
template <typename T>
static void
_SampleSegment(const Ts_EvalCache<T, TsTraits<T>::interpolatable> * cache,
        double startTime, double endTime,
        double timeScale, double valueScale, double tolerance,
        TsSamples & samples )
{
    const Ts_Bezier<T>* bezier  = cache->GetBezier();
    // Sample the Bezier
    _SampleBezierClip(bezier->timeCoeff,
            bezier->timePoints, bezier->valuePoints,
            startTime, endTime,
            timeScale, valueScale, tolerance, samples);
}

static void
_AddExtrapolateSample( const TsSpline & val, double t, double dtExtrapolate,
        TsSamples& samples )
{
    // Get segment endpoints
    VtValue yLeft, yRight;
    if (dtExtrapolate < 0.0) {
        yLeft  = val.Eval(t + dtExtrapolate, TsRight);
        yRight = val.Eval(t, TsLeft);
        samples.push_back(TsValueSample(t + dtExtrapolate, yLeft,
                    t, yRight));
    }
    else {
        yLeft  = val.Eval(t, TsRight);
        yRight = val.Eval(t + dtExtrapolate, TsLeft);
        samples.push_back(TsValueSample(t, yLeft,
                    t + dtExtrapolate, yRight));
    }
}

// XXX: Is this adequate?  What if the time scale is huge?  Does it need to be
// scaled based on the times in use?
static const double extrapolateDistance = 100.0;

static void
_EvalLinear( const TsSpline & val,
        TsTime startTime, TsTime endTime,
        TsSamples& samples )
{
    const TsKeyFrame & first = *val.begin();
    const TsKeyFrame & last = *val.rbegin();

    // Sample to left of first keyframe if necessary.  We'll take a sample
    // way to its left.
    if (startTime < first.GetTime()) {
        // Extrapolate from first keyframe
        _AddExtrapolateSample(val, first.GetTime(), 
            (startTime - first.GetTime() - extrapolateDistance), samples);

        // If endTime is at or before the first keyframe then we're done
        if (endTime <= first.GetTime()) {
            return;
        }

        // New start time is the time of the first keyframe
        startTime = first.GetTime();
    }

    // Find the bounding keyframes.  (We've already handled extrapolation to
    // the left above and we'll handle extrapolation to the right at the end.)
    std::pair<TsSpline::const_iterator, TsSpline::const_iterator> bounds =
            _GetBounds(val, startTime, endTime);
    TsSpline::const_iterator i = bounds.first;
    TsSpline::const_iterator j = bounds.second;

    // On a linear or held segment we just take a sample at the endpoints.
    while (i != j) {
        const TsKeyFrame& curKf  = *i;
        const TsKeyFrame& nextKf = *(++i);

        // Sample
        TsTime t0 = curKf.GetTime();
        TsTime t1 = nextKf.GetTime();
        samples.push_back(
                TsValueSample(t0, val.Eval(t0, TsRight),
                    t1, val.Eval(t1, TsLeft)));
    }

    // Sample to the right of the last keyframe if necessary.  We'll take
    // a sample 100 frames beyond the end time.
    if (endTime > last.GetTime()) {
        // Extrapolate from last keyframe
        _AddExtrapolateSample(val, last.GetTime(),
            (endTime - last.GetTime() + extrapolateDistance), samples);
    }
}

template<typename T>
static void
_EvalCurve( const TsSpline & val,
    TsTime startTime, TsTime endTime,
    double timeScale, double valueScale, double tolerance,
    TsSamples& samples )
{
    const TsKeyFrame & first = *val.begin();
    const TsKeyFrame & last = *val.rbegin();

    // Sample to left of first keyframe if necessary.  We'll take a sample
    // 100 frames before the start time.
    if (startTime < first.GetTime()) {
        // Extrapolate from first keyframe
        _AddExtrapolateSample(val, first.GetTime(), 
            (startTime - first.GetTime() - extrapolateDistance), samples);

        // If endTime is at or before the first keyframe then we're done
        if (endTime <= first.GetTime()) {
            return;
        }

        // New start time is the time of the first keyframe
        startTime = first.GetTime();
    }

    // Find the bounding keyframes.  (We've already handled extrapolation to
    // the left above and we'll handle extrapolation to the right at the end.)
    std::pair<TsSpline::const_iterator, TsSpline::const_iterator> bounds =
            _GetBounds(val, startTime, endTime);
    TsSpline::const_iterator i = bounds.first;
    TsSpline::const_iterator j = bounds.second;


    // Handle the keyframe segments in the interval, excluding the region
    // (if any) after the last keyframe, as this region is handled seperately
    // afterward.
    TsSpline::const_iterator i2 = i; i2++;
    while (i != j) {
        if (i2 != val.end()) {
            Ts_EvalCache<T> cache(*i, *i2);

            _SampleSegment<T>(&cache, startTime, endTime,
                    timeScale, valueScale, tolerance, samples);
        }

        i = i2;
        i2++;
    }

    // Sample to the right of the last keyframe if necessary.  We'll take
    // a sample 100 frames after the end time.
    if (endTime > last.GetTime()) {
        // Extrapolate from last keyframe
        _AddExtrapolateSample(val, last.GetTime(), 
            (endTime - last.GetTime() + extrapolateDistance), samples);
    }
}

TsSamples
Ts_Sample( const TsSpline & val, TsTime startTime, TsTime endTime,
    double timeScale, double valueScale, double tolerance )
{
    TsSamples samples;

    if (startTime > endTime) {
        TF_CODING_ERROR("invalid interval (start > end)");
        return samples;
    }
    else if (val.IsEmpty() || startTime == endTime) {
        return samples;
    }

    // Sample the segments between keyframes
    const std::type_info & t = val.GetTypeid();
    if (TfSafeTypeCompare(t, typeid(double))) {
        _EvalCurve<double>(val, startTime, endTime,
                timeScale, valueScale, tolerance, samples);
    }
    else if (TfSafeTypeCompare(t, typeid(float))) {
        _EvalCurve<float>(val, startTime, endTime,
                timeScale, valueScale, tolerance, samples);
    }
    else {
        _EvalLinear(val, startTime, endTime, samples);
    }

    return samples;
}

////////////////////////////////////////////////////////////////////////
// Breakdown

template<typename T>
static void
_Breakdown(TsKeyFrameMap * k,
           const TsKeyFrameMap::iterator & k1,
           const TsKeyFrameMap::iterator & k2,
           const TsKeyFrameMap::iterator & k3)
{
    // Wrap the keyframes in a spline in order to get an eval cache for the
    // segment.
    TsSpline spline(*k);

    // Setup Bezier cache for key frames k1 and k3
    Ts_EvalCache<T> cache(*spline.begin(), *spline.rbegin());

    // Get the Bezier from the cache
    const Ts_Bezier<T>* bezier = cache.GetBezier();

    // Compute the spline parameter for the time of k2 in the Bezier
    // defined by k1 and k3.
    double u = Ts_SolveCubic(bezier->timeCoeff, k2->GetTime());

    // Subdivide the Bezier at u
    T leftValue[4], rightValue[4];
    TsTime leftTime[4], rightTime[4];
    _SubdivideBezier(bezier->valuePoints, leftValue,  u,  true);
    _SubdivideBezier(bezier->timePoints,  leftTime,   u,  true);
    _SubdivideBezier(bezier->valuePoints, rightValue, u, false);
    _SubdivideBezier(bezier->timePoints,  rightTime,  u, false);

    // Update the middle key frame's slope.
    if (k2->SupportsTangents()) {
        k2->SetLeftTangentSlope  (VtValue(( leftValue[3] -  leftValue[2]) /
                (  leftTime[3] -   leftTime[2])));
        k2->SetRightTangentSlope (VtValue((rightValue[1] - rightValue[0]) /
                ( rightTime[1] -  rightTime[0])));
    }

    // Update the tangent lengths.  We change the inner lengths of k1 and k3,
    // and both of k2.
    if (k1->SupportsTangents())
        k1->SetRightTangentLength(leftTime[1] - leftTime[0]);
    if (k2->SupportsTangents())
        k2->SetLeftTangentLength (leftTime[3] - leftTime[2]);
    if (k2->SupportsTangents())
        k2->SetRightTangentLength(rightTime[1] - rightTime[0]);
    if (k3->SupportsTangents())
        k3->SetLeftTangentLength (rightTime[3] - rightTime[2]);
}

void
Ts_Breakdown( TsKeyFrameMap * k )
{
    // Sanity checks
    if (k->size() != 3) {
        TF_CODING_ERROR("Wrong number of key frames in breakdown");
        return;
    }
    TsKeyFrameMap::iterator k1 = k->begin();
    TsKeyFrameMap::iterator k2 = k1; k2++;
    TsKeyFrameMap::iterator k3 = k2; k3++;

    if (k1->GetTime() >= k2->GetTime() ||
        k2->GetTime() >= k3->GetTime()) {
        TF_CODING_ERROR("Bad key frame ordering in breakdown");
        return;
    }

    // Breakdown
    VtValue v = k1->GetZero();
    if (TfSafeTypeCompare(v.GetTypeid(), typeid(double))) {
        _Breakdown<double>(k, k1, k2, k3);
    }
    else if (TfSafeTypeCompare(v.GetTypeid(), typeid(float))) {
        _Breakdown<float>(k, k1, k2, k3);
    }
    else {
        // No tangents for this value type so nothing to do
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
