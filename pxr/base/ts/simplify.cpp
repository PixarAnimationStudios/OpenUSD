//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/defines.h"
#include "pxr/base/work/loops.h"

#include "pxr/base/ts/simplify.h"

#include "pxr/base/ts/evalCache.h"
#include "pxr/base/ts/evaluator.h"
#include "pxr/base/ts/keyFrameUtils.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

#define SIMPLIFY_DEBUG 0

static const double _toleranceEspilon = 1e-6;
static const double _minTanLength = 0.1;

// *****************   SIMPLIFY ***********************************

// Overview:  This is a "greedy" algorithm which iteratively removes keys and
// adjusts the neighbor tangents' lengths to compensate.  If runs over all the
// keys and measures the error resulting from removing each one and making the
// best compensation possible.  Then, in a loop it removes the one with the
// least error (compensating the neighbor tangents) and then re-evaluates the
// neighbors for the error-if-removed metric.  It stops when the smallest such
// error is too big.

// We use RMS for compensating the tangents, since the derivative must be
// smooth.  The user-facing tolerance is based on Max.
enum _SimplifyErrorType {
    _SimplifyErrorTypeRMS,
    _SimplifyErrorTypeMAX,
};

// Utility routine for setting the left tangent length.
static void
_SetLeftTangentLength(
    TsKeyFrame *key,
    double length)
{
    if (!key)
        return;
    if (!key->SupportsTangents())
        return;

    key->SetLeftTangentLength(length);
}

// Similar to the above, for the right side
static void
_SetRightTangentLength(
    TsKeyFrame *key, 
    double length)
{
    if (!key)
        return;
    if (!key->SupportsTangents())
        return;

    key->SetRightTangentLength(length);
}

// Compute the error within spanInterval at each frame. vals are the reference
// values in the original spline in the interval valsInterval.  If useMax,
// then return the max error, else rms
static double
_ComputeError(const TsSpline &spline,
        const GfInterval &spanInterval,
        const std::vector<double> &vals,
        const GfInterval &valsInterval,
        bool useMax)
{
    if (!TF_VERIFY(spanInterval.GetMin() >= valsInterval.GetMin()))
        return DBL_MAX;
    if (!TF_VERIFY(vals.size() == valsInterval.GetSize() + 1))
        return DBL_MAX;
    // where to start looking in vals
    size_t valsBase = size_t(spanInterval.GetMin() - valsInterval.GetMin());
    if (!TF_VERIFY(valsBase < vals.size()))
        return DBL_MAX;

    size_t numSamples = size_t(spanInterval.GetSize() + 1);
    if (!TF_VERIFY(valsBase + numSamples <= vals.size()))
        return DBL_MAX;

    double err=0;
    for (size_t i = 0; i < numSamples; i++) {
        double t = spanInterval.GetMin() + i;
        double thisVal = spline.Eval(t).Get<double>();
        double thisErr = thisVal - vals[valsBase + i];
        if (useMax) {
            // max
            double absErr = TfAbs(thisErr);
            if (absErr > err)
                err = absErr;
        } else {
            // rms
            err += thisErr * thisErr;
        }
    }
    return useMax ? err : sqrt(err/numSamples);
}

// Compute the error from setting the left or right tangent of k to l within
// spanInterval at each frame. vals are the reference values in the original
// spline in the interval valsInterval
static double
_ComputeErrorForLength(bool affectRight, double l, 
        const TsKeyFrame &k,
        TsSpline* spline,
        const GfInterval &spanInterval,
        const std::vector<double> &vals,
        const GfInterval &valsInterval)
{
    TsKeyFrame nk = k;
    TsTime spanSize = spanInterval.GetSize();

    // Set the length.  
    if (affectRight)
        _SetRightTangentLength(&nk, l * spanSize);
    else
        _SetLeftTangentLength(&nk, l * spanSize);

    spline->SetKeyFrame(nk);
    return _ComputeError(*spline, spanInterval, vals, valsInterval, 
            /* useMax = */ false );
}

// Assumed knots at the ends of the spanInterval and none inside; will
// world
// stretch the inner tangents for best result.  vals are the reference values
// in the original spline in valsInterval, at each frame, that we will compute
// the error in refernce to.  
static void _SimplifySpan(TsSpline* spline, 
        const GfInterval &spanInterval,
        const std::vector<double> &vals,
        const GfInterval &valsInterval)
{
    TRACE_FUNCTION();

    std::vector<TsKeyFrame> keyFrames 
        = spline->GetKeyFramesInMultiInterval(GfMultiInterval(spanInterval));

    // Not illegal, but we can't simplify the span without 2 knots.
    if (keyFrames.size() != 2) {
        return;
    }

    // If there is no error, even before messing with the tangents, then
    // we're already done.  This could typically happen if we're in a flat
    // stretch
    double initialErr = 
        _ComputeError(*spline, spanInterval, vals, valsInterval, 
        /* useMax = */ false );
    if (initialErr < 1e-10)
        return;

    TsKeyFrame k0 = keyFrames.front();
    TsKeyFrame k1 = keyFrames.back();
    double v0 = k0.GetValue().Get<double>();
    double v1 = k1.GetValue().Get<double>();

    const double minVal = TfMin(v0, v1);
    const double maxVal = TfMax(v0, v1);
    const double tolerance = (maxVal - minVal) / 20000;

    TsTime spanSize = spanInterval.GetSize();
    if (spanSize == 0)
        return;

    // Initial guess at tangent lengths
    _SetRightTangentLength(&k0, .33 * spanSize);
    _SetLeftTangentLength(&k1, .33 * spanSize);

    spline->SetKeyFrame(k0);
    spline->SetKeyFrame(k1);

    // These are lengths for the tangent, as fractions of the span size
    double lo, hi;

    int iter = 0;
    // Delta length (normalized) to use for slop calc; we'll sample this delta
    // +/- the current guess to approximate the slope
    const double ldel = .00001;
    // The error due to the last 2 iterations
    double lastErr = 1e10;
    double thisErr = 1e10;
    // Each iteration adjusts one tangent length for best results,
    // alternating, using binary search.  Stop when the error is tiny, or
    // stops changing very much (or is not converging)
    for (; iter < 100; iter++) {
#if SIMPLIFY_DEBUG
        printf("ITER %d **************\n", iter);
#endif
        // The range for our guesses spans from lo to hi.  If we look from 0
        // to 1, then our tangent handles could overlap, so instead we only
        // look from 0 to 0.5, ensuring that they will never cross (this is
        // what the animators seems to like).  Also we use _minTanLength to
        // avoid going all the way to the low limit so we don't allow tangents
        // to become unusably short.  _minTanLength is expressed in absolute
        // length, where lo is normalized to [0,1] on the spanInterval.  So
        // convert to normalized.
        lo = _minTanLength / spanSize;
        hi = 0.5 - 2 * ldel;
        while (1) {
            // New guess
            double g = (lo + hi) * .5;
            bool isK0 = (iter & 1) == 0;
            double err0 = _ComputeErrorForLength(isK0,
                        (g - ldel), isK0 ? k0:k1, spline, 
                        spanInterval, vals, valsInterval);
            double err1 = _ComputeErrorForLength(isK0,
                        (g + ldel), isK0 ? k0:k1, spline, 
                        spanInterval, vals, valsInterval);


            double slope = (err1 - err0) / (2 * ldel);
#if SIMPLIFY_DEBUG
            printf("guess g is %g  err0 %g err1 %g, slope %g\n", g, err0, err1,
                    slope);
#endif
            if (slope > 0) {
                hi = g;

#if SIMPLIFY_DEBUG
                printf("New hi %g\n", hi);
#endif
            }
            else {
                lo = g;

#if SIMPLIFY_DEBUG
                printf("New lo %g\n",lo);
#endif
            }

            // If we've converged, time to break
            if (hi - lo < .00005) {
                // Recompute the error for the actual guess (also leaving the
                // spline simplified for that guess)
                thisErr = _ComputeErrorForLength(isK0,
                        g, isK0 ? k0:k1, spline, 
                        spanInterval, vals, valsInterval);
                break;
            }
        }


        // If the error changed very little, done
        if (TfAbs(lastErr - thisErr) < tolerance)
            break;
        lastErr = thisErr;
    }
#if SIMPLIFY_DEBUG
    printf("span [%g %g] Num iters %d thisErr is %g\n", 
        spanInterval.GetMin(), spanInterval.GetMax(), iter, thisErr);
#endif
}

// Return true if there is a kink in the spline in the interval.  We look
// just at the X cubic, i.e. time(u).  Since elsewhere in Ts we fix this
// function to be monotinically increasing, we'll call foul if we ever get a
// slope close to 0 inside the interval.  So, we just have to find the extereme
// of time'(u); if it's between 0 and 1, and time'(u) opens upward, that's the
// min; if it's close to 0 we have a kink.
static bool
_IsSplineKinkyInInterval(const TsSpline &spline, 
        const GfInterval &spanInterval)
{
    TsSpline::const_iterator ki0 = spline.find(spanInterval.GetMin());
    TsSpline::const_iterator ki1 = spline.find(spanInterval.GetMax());
    // This is now legal (but still false)
    if (!(ki0 != spline.end() && ki1 != spline.end()))
        return false;

    // This can supply us the coefficients
    Ts_EvalCache<double, true>::TypedSharedPtr cache;
    cache = Ts_EvalCache<double, true>::New(*ki0, *ki1);
    const Ts_Bezier<double> *bezier = cache->GetBezier();
    // True if the parabola opens upward
    bool vertexMin = bezier->timeCoeff[3] > 0;

    double min = DBL_MAX;
    // If not flat...
    if (bezier->timeCoeff[3] != 0) {
        // The u coord at the vertex, gotten by solving time''(u) = 0
        TsTime uv = -bezier->timeCoeff[2]/(3*bezier->timeCoeff[3]);
        // If the deriv is very flat near the edges of the interval, don't
        // flag this as a kink
        if (vertexMin && uv > .05 && uv < .95) {
            // Get the value of time'(uv)
            min = Ts_EvalCubicDerivative(bezier->timeCoeff, uv);
            if (min < .001) {
                // Kinky!
                return true;
            }
        }
    }
    return false;
}

// If the key at the given time were removed, compute the resulting error.
// Two intervals supplied; spanInterval is the interval to simplify over.
// vals are the reference values in the original spline in valsInterval, at
// each frame, that we will compute the error in refernce to.  Note that the
// spline will be unchanged upon return.  This assumes routine that there are
// knots at t and on the ends of spanInterval. If the simplify results in a
// kink, we'll pretend the error was huge.
double
_ComputeErrorIfKeyRemoved(TsSpline* spline, TsTime t,
        const GfInterval &spanInterval,
        const std::vector<double> &vals,
        const GfInterval &valsInterval)
{
    if (!TF_VERIFY(vals.size() == valsInterval.GetSize() + 1))
            return DBL_MAX;

    // Get the keys that will be changed by _SimplifySpan
    TsSpline::const_iterator k0 = spline->find(spanInterval.GetMin());
    TsSpline::const_iterator k  = spline->find(t);
    TsSpline::const_iterator k1 = spline->find(spanInterval.GetMax());

    if (!TF_VERIFY(k != spline->end()))
        return DBL_MAX;

    TsKeyFrame kCopy  = *k;
    TsKeyFrame k0Copy;
    bool k0CopyValid = false;
    TsKeyFrame k1Copy;
    bool k1CopyValid = false;
    if (k0 != spline->end()) {
        k0Copy = *k0;
        k0CopyValid = true;
    }
    if (k1 != spline->end()) {
        k1Copy = *k1;
        k1CopyValid = true;
    }

    spline->RemoveKeyFrame(kCopy.GetTime());

    // Find the best tangents for the neighbors
    _SimplifySpan(spline, spanInterval, vals, valsInterval);

    double err = DBL_MAX;
    // If the spline has a kink in the interval, let the large error stand
    if (!_IsSplineKinkyInInterval(*spline, spanInterval)) {
        // Compute the error over the larger interval
        err = _ComputeError(*spline, valsInterval, vals, valsInterval,
            /* useMax = */ true);
    }

    // Put back the keys
    spline->SetKeyFrame( kCopy);
    // We may have set these in _SimplifySpan, so we want to set them back to
    // what they were before.
    if (k0CopyValid) {
        spline->SetKeyFrame(k0Copy);
    }
    if (k1CopyValid) {
        spline->SetKeyFrame(k1Copy);
    }

    return err;
}
    
// Info per knots in range
namespace {
struct _EditSimplifyKnotInfo {
    TsTime t;
    TsKnotType knotType;
    bool removable;
    // The error that would result in the spline were this removed
    double errIfRemoved; 
};
}

// Set the error-if-removed for the ith element of the vector of
// _EditSimplifyKnotInfo's
static void _SetKnotInfoErrorIfKeyRemoved(
        std::vector<_EditSimplifyKnotInfo> &ki, 
        size_t i, TsSpline* spline, 
        const std::vector<double> &vals,
        const GfInterval &valsInterval)
{
    if (!TF_VERIFY(i >= 0 && i < ki.size()))
        return;

    if (ki[i].removable) {
        // Must be inside to be removable
        if (!TF_VERIFY(i > 0 && i < ki.size()-1))
            return;

        // We know it's not on the end
        ki[i].errIfRemoved = _ComputeErrorIfKeyRemoved(spline, ki[i].t,
                GfInterval(ki[i-1].t, ki[i+1].t),
                vals, valsInterval);
    } else {
        // Shouldn't ever be accessing this if !k.removable, but just in
        // case
        ki[i].errIfRemoved = DBL_MAX;
    }
}

// True if the knot has a flat segment on either side
static
bool _IsKnotOnPlateau(const TsSpline& spline, const TsKeyFrame& key)
{
    const TsKeyFrameMap& keyMap = spline.GetKeyFrames();

    TsKeyFrameMap::const_iterator kIter = 
        keyMap.lower_bound(key.GetTime());

    if (!TF_VERIFY(kIter != keyMap.end()))
            return false;

    if (kIter != keyMap.begin()) {
        if (spline.IsSegmentFlat(*(kIter-1), key))
            return true;
    }

    if (kIter+1 != keyMap.end()) {
        if (spline.IsSegmentFlat(key, *(kIter+1)))
            return true;
    }

    return false;
}

// True if the knot is an extreme.  It must be > one neighbor and <= the
// other, or < one and >= the other.  The max value difference between it and
// its neighbors must also be > tolerance.
static
bool _IsKnotAnExtreme(const TsSpline &spline, const TsKeyFrame &k,
        double tolerance)
{

    std::pair<TsExtrapolationType, TsExtrapolationType> extrap = 
        spline.GetExtrapolation();

    // Does it have left/right neighbor?
    bool hasLeft = false;
    bool hasRight = false;

    const TsKeyFrameMap& keyMap = spline.GetKeyFrames();
    // This points at k
    TsKeyFrameMap::const_iterator kIter = 
        keyMap.lower_bound(k.GetTime());
    if (!TF_VERIFY(kIter != keyMap.end()))
            return false;

    if (kIter != keyMap.begin()) {
        hasLeft = true;
    } else {
        // Cases below get tricky to evaluate if we're at an end, and extrap
        // is not held;  very rare, so just return true
        if (extrap.first != TsExtrapolationHeld)
            return true;
        hasLeft = false;
    }

    if (kIter+1 != keyMap.end()) {
        hasRight = true;
    } else {
        // Cases below get tricky to evaluate if we're at an end, and extrap
        // is not held;  very rare, so just return true
        if (extrap.second != TsExtrapolationHeld)
            return true;
        hasRight = false;
    }

    if (!hasLeft && !hasRight)
        return false;

    // Nomenclature:
    //   Knot values left to right: v0, v1, v, v2, v3 where v is k's value;
    //   v0 and v3 only used (below) if hasLeft and hasRight
    double v1, v, v2;
    v = k.GetValue().Get<double>();
    // Set v1 and v2 to v in case the knots don't exist
    v1 = v2 = v;

    if (hasLeft) {
        v1 = (kIter-1)->GetValue().Get<double>();
    }

    if (hasRight) {
        v2 = (kIter+1)->GetValue().Get<double>();
    }

    // The values we will test v against
    double vl = v1;
    double vr = v2;

    // For something to be an extreme, it should be monotonically bigger than
    // its two neighbors in each direction (if they exist), and by at least
    // 'tolerance'
    if (hasLeft && hasRight) {
        if (kIter-1 != keyMap.begin() && kIter+2 != keyMap.end()) {
            double v0 = (kIter-2)->GetValue().Get<double>();
            double v3 = (kIter+2)->GetValue().Get<double>();
            if (v > v1 && v1 > v0 && v > v2 && v2 > v3) {
                    vl = v0;
                    vr = v3;
                }
            if (v < v1 && v1 < v0 && v < v2 && v2 < v3) {
                    vl = v0;
                    vr = v3;
                }
            }
        }

    double delta = 0;
    if ((v > vl && v >= vr) || (v >= vl && v > vr)) {
        delta = TfMax(v - vl, v - vr);
    }
    if ((v < vl && v <= vr) || (v <= vl && v < vr)) {
        delta = TfMax(vl - v, vr - v);
    }
    return delta > tolerance;
}

// The actual tolerance is maxErrFract times the value range of the spline
// within the bounds of the intervals.
void TsSimplifySpline(TsSpline* spline,
                        const GfMultiInterval &inputIntervals,
                        double maxErrFract,
                        double extremeMaxErrFract)
{
    TRACE_FUNCTION();

#if SIMPLIFY_DEBUG
    printf("TsSimplifySpline maxErrFract: %g\n", maxErrFract);
#endif

    if (!spline) {
        TF_CODING_ERROR("Invalid spline.");
        return;
    }

    // If the max desired error is effectively zero, there's nothing to do.
    if (maxErrFract < _toleranceEspilon) {
        return;
    }

    // Reduce the intervals to a valid range.
    GfMultiInterval intervals = inputIntervals;
    intervals.Intersect(spline->GetFrameRange());

    TsSpline splineCopy = *spline;

    // Want to get the keframes in the bounds of the selection, plus an extra
    // one on either end (if any).
    GfInterval valsInterval = intervals.GetBounds();
    if (valsInterval.IsEmpty())
        return;

    // Clear Redundant keys as a pre-pass to handle easy-to-remove keys in a
    // linear fashion, rather than relying on the N^2 algorithm below.
    // We'll play it safe and leave the last knot in each interval.
    // See PRES-74561
    bool anyRemoved = splineCopy.ClearRedundantKeyFrames(VtValue(), intervals);

    GfInterval fullRange = splineCopy.GetFrameRange();

    // Extra one before
    if (!valsInterval.IsMinClosed() && 
        splineCopy.count(valsInterval.GetMin())) {
        // If the interval's min is open, check if there's a keyframe exactly
        // at the min. If so add it by closing the min of the interval
        valsInterval.SetMin(valsInterval.GetMin(), true);
    } else {
        std::optional<TsKeyFrame> before = 
            splineCopy.GetClosestKeyFrameBefore(intervals.GetBounds().GetMin());

        // Expand the valsInterval if extra ones existed
        if (before)
            valsInterval.SetMin(before->GetTime());
    }
    
    // Extra one after
    if (!valsInterval.IsMaxClosed() && 
        splineCopy.count(valsInterval.GetMax())) {
        // If the interval's max is open, check if there's a keyframe exactly
        // at the max. If so add it by closing the min of the interval
        valsInterval.SetMax(valsInterval.GetMax(), true);
    } else {
        std::optional<TsKeyFrame> after =
            splineCopy.GetClosestKeyFrameAfter(intervals.GetBounds().GetMax());

        // Expand the valsInterval if extra ones existed
        if (after)
            valsInterval.SetMax(after->GetTime());
    }

    // Get all the keys
    std::vector<TsKeyFrame> keyFrames 
        = splineCopy.GetKeyFramesInMultiInterval(GfMultiInterval(valsInterval));

#if SIMPLIFY_DEBUG
    printf("TsSimplifySpline # of keyFrames: %ld\n", keyFrames.size());
#endif

    // Early out if not enough knots
    if (keyFrames.size() < 3) {
        if (anyRemoved) {
            *spline = splineCopy;
        }
        return;
    }

    // Verify that the spline holds doubles
    if (!keyFrames.front().GetValue().IsHolding<double>()) {
        return;
    }

    // Compute the spline at every frame in 'valsInterval' for error
    // calculation; remember the range
    std::vector<double> vals;
    double minVal =  DBL_MAX;
    double maxVal = -DBL_MAX;
    for (double t = valsInterval.GetMin(); t <= valsInterval.GetMax(); 
            t += 1.0) {
        double v = splineCopy.Eval(t).Get<double>();
        if (v > maxVal)
            maxVal = v;
        if (v < minVal)
            minVal = v;
        vals.push_back(v);
    }

    double tolerance = (maxVal - minVal) * maxErrFract;
    // For fully flat (or almost fully flat) curves, set the tolerance
    // a little above zero, else nothing will happen
    if (TfAbs(maxVal - minVal) < _toleranceEspilon) {
        tolerance = _toleranceEspilon;
    }

    // See _IsKnotAnExtreme
    // Legacy code set this to a fixed fraction of the overall function range.
    double extremeTolerance = (maxVal - minVal) * extremeMaxErrFract;

    // Similar correction to the above, else for for fully flat (or almost
    // fully flat) curves, everything will be considered and extreme and
    // nothing will be removed.
    if (TfAbs(maxVal - minVal) < _toleranceEspilon) {
        extremeTolerance = _toleranceEspilon;
    }

#if SIMPLIFY_DEBUG
    printf("TsSimplifySpline valsInterval min: %g max: %g\n", valsInterval.GetMin(), valsInterval.GetMax());
    printf("TsSimplifySpline minVal: %g maxVal: %g\n", minVal, maxVal);
    printf("TsSimplifySpline tolerance: %g\n", tolerance);
#endif

    // Set the tangents: If it's 1 frame away from its neighbors (or it's on
    // the end, and its outgoing extrapolation is flat) then we are free to set
    // its slope.  Set it to flat for extremes, else catrom-like.  For
    // lengths, if the tangent's neighbor is in the interval and 1 frame away,
    // we can set the length; set it to be 1/3 the way to its neighbor.

    std::pair<TsExtrapolationType, TsExtrapolationType> extrap =
            splineCopy.GetExtrapolation();

    for (size_t i = 0; i < keyFrames.size(); ++i) {
        // If not Bezier, nothing to do
        TsKeyFrame k = keyFrames[i];
        if (k.GetKnotType() != TsKnotBezier)
            continue;

        TsTime t = k.GetTime();
        // Is there a knot 1 frame adjacent to the right?
        bool rightAdjacent = i < keyFrames.size() - 1 && 
                (keyFrames[i+1].GetTime() - t) == 1;
        // Is there a knot 1 frame adjacent to the left?
        bool leftAdjacent = i > 0  && 
                (t - keyFrames[i-1].GetTime()) == 1;

#if SIMPLIFY_DEBUG
        printf("TsSimplifySpline keyFrame: %ld at time %g rightAdjacent: %d leftAdjacent: %d\n", i, t, rightAdjacent, leftAdjacent);
#endif

        if (!leftAdjacent && !rightAdjacent)
            continue;

        // Right val at this frame
        double vr  = keyFrames[i].GetValue().Get<double>();
        // Left val at this frame
        double vl  = keyFrames[i].GetLeftValue().Get<double>();
        // Prev, next if adjacent; init for compiler
        double vp=0, vn=0;

        if (leftAdjacent)
            vp = keyFrames[i-1].GetValue().Get<double>();

        if (rightAdjacent)
            // Use left value if dual valued
            vn = keyFrames[i+1].GetLeftValue().Get<double>();

        std::optional<double> slope;

        if (leftAdjacent && rightAdjacent) {
            // If it's an extreme or on a plateau, flatten its slope
            if (_IsKnotOnPlateau(splineCopy,  keyFrames[i])
                || _IsKnotAnExtreme(splineCopy,  keyFrames[i],
                    extremeTolerance)) {
#if SIMPLIFY_DEBUG
                printf("TsSimplifySpline keyFrame: %ld at time %g"
                        " _IsKnotOnPlateau: YES\n", i, t);
#endif
                slope = 0.0;
            } else {
#if SIMPLIFY_DEBUG
                printf("TsSimplifySpline keyFrame: %ld at time %g"
                        " _IsKnotOnPlateau: NO\n", i, t);
#endif
                // Parallel to neighbors
                slope = (vn - vp) / 2.0;
            }
        } else if (t == fullRange.GetMin() && rightAdjacent && 
                extrap.first == TsExtrapolationHeld) {
            // Left edge, just point at right neighbor
            slope = vn - vr;
#if SIMPLIFY_DEBUG
            printf("TsSimplifySpline keyFrame: %ld left edge\n", i);
#endif
        } else if (t == fullRange.GetMax() && leftAdjacent && 
                extrap.second == TsExtrapolationHeld) {
            // Right edge, just point at left neighbor
            slope = vl - vp;
#if SIMPLIFY_DEBUG
            printf("TsSimplifySpline keyFrame: %ld at time %g right edge\n", 
                    i, t);
#endif
        }

        if (leftAdjacent)
            _SetLeftTangentLength(&k, .3333 * 1.0);
        if (rightAdjacent)
            _SetRightTangentLength(&k, .3333 * 1.0);

        if (slope && k.SupportsTangents()) {
            k.SetLeftTangentSlope(VtValue(*slope));
            k.SetRightTangentSlope(VtValue(*slope));
        }
        keyFrames[i] = k;
        splineCopy.SetKeyFrame(k);

#if SIMPLIFY_DEBUG
        printf("TsSimplifySpline keyFrame: %ld at time %g result slope"
                " %g/%g length %g/%g\n", i, t,
                k.GetLeftTangentSlope().Get<double>(), 
                k.GetRightTangentSlope().Get<double>(), 
                k.GetLeftTangentLength(), k.GetRightTangentLength());
#endif
    }

    // This holds the data about what's removable and the error ifremoved, per
    // knot.
    std::vector<_EditSimplifyKnotInfo> ki;
    // We'll have the number of key frames, plus one on either side.
    ki.reserve(keyFrames.size() + 2);

    size_t numRemovable = 0;

    // This is in order, so we prepend here, and push back after the
    // loop
    double extraPoint = keyFrames[0].GetTime() - 1.0;
    _EditSimplifyKnotInfo k;
    k.t = extraPoint;
    k.knotType = keyFrames[0].GetKnotType();
    k.removable = false;
    ki.push_back(k);

    // First figure out which are removable
    for (size_t i = 0; i < keyFrames.size(); ++i) {
        _EditSimplifyKnotInfo k;
        k.t = keyFrames[i].GetTime();
        k.knotType = keyFrames[i].GetKnotType();

        // Removable if it's selected, not an extreme, and not on the ends of
        // the valsInterval.  (We only compute error within the valsInterval,
        // so the effect of removing an end would not be known.)
        k.removable = intervals.Contains(k.t) && 
            // This is a little hacky, but the first frame is still not
            // removable
            i != 0
            && !_IsKnotAnExtreme(splineCopy,  keyFrames[i],
                    extremeTolerance);

#if SIMPLIFY_DEBUG
        printf("TsSimplifySpline keyFrame: %ld at time %g %s\n", 
                i, k.t, intervals.Contains(k.t)?"CONTAINED":"NOT CONTAINED");
        printf("TsSimplifySpline keyFrame: %ld at time %g %s\n", i, k.t, 
                _IsKnotAnExtreme(splineCopy,  keyFrames[i], extremeTolerance)
                ? "EXTREME" : "normal");
        printf("TsSimplifySpline keyFrame: %ld at time %g %s\n", i, k.t, 
                k.removable ? "REMOVABLE" : "KEEP");
#endif

        if (k.removable)
            numRemovable++;

        ki.push_back(k);
    }
    // Add the last one past the end of our knots
    extraPoint = keyFrames[keyFrames.size()-1].GetTime() + 1.0;
    k.t = extraPoint;
    k.knotType = keyFrames[keyFrames.size()-1].GetKnotType();
    k.removable = false;
    ki.push_back(k);

    if (numRemovable == 0) {
        if (anyRemoved) {
            *spline = splineCopy;
        }
        return;
    }

    // Set the error-if-removed for each one
    for (size_t i = 0; i < ki.size(); ++i) {
        _SetKnotInfoErrorIfKeyRemoved(
                ki, i, &splineCopy, vals, valsInterval);
    }

    // At this point, keyFrames will not be reflective of what's in
    // splineCopy; clear to make this evident
    keyFrames.clear();

    // Main loop
    while (1) {
        // Find the minimum error for those knots that are removable
        size_t bestIndex = 0;
        bool first = true;
        for (size_t i = 0; i < ki.size(); ++i) {
            if (ki[i].removable) {
                if (first) {
                    bestIndex = i;
                    first = false;
                } else {
                    if (ki[i].errIfRemoved < ki[bestIndex].errIfRemoved) {
                        bestIndex = i;
                    }
                }
            }
        }

#if SIMPLIFY_DEBUG
        printf("Best to remove at time %g (errIfRemoved was %g, tol %g)\n", 
            ki[bestIndex].t, ki[bestIndex].errIfRemoved, tolerance);
#endif

        // If the best one is less than our tolerance, remove it
        if (ki[bestIndex].errIfRemoved <= tolerance) {
#if SIMPLIFY_DEBUG
            printf("   Removing it\n");
#endif
            splineCopy.RemoveKeyFrame(ki[bestIndex].t);
            // bestIndex should always be inside
            if (!TF_VERIFY(bestIndex > 0 && bestIndex < ki.size()-1))
                return;
            // Fix the adjacent handles
            _SimplifySpan(&splineCopy, 
                GfInterval(ki[bestIndex-1].t, ki[bestIndex+1].t), 
                    vals, valsInterval);
            
            // Now remove the entry from ki
            ki.erase(ki.begin() + bestIndex);

            // Now we have to fix the errIfRemoved data held in the adjcant
            // knots. Deleting a Bezier only has affect on the new conjoined
            // span.
            _SetKnotInfoErrorIfKeyRemoved(
                    ki, bestIndex - 1, &splineCopy, vals, valsInterval);
            _SetKnotInfoErrorIfKeyRemoved(
                    ki, bestIndex, &splineCopy, vals, valsInterval);

            anyRemoved = true;
        } else {
            // Can't remove anything; done
            break;
        }
    }

    // If we removed any knots, then save the result.
    // XXX: If we didn't remove anything, but maybe just adjusted handles,
    // shouldn't we save that too?
    if (anyRemoved) {
        *spline = splineCopy;
    }
}

void TsSimplifySplinesInParallel(
    const std::vector<TsSpline *> &splines,
    const std::vector<GfMultiInterval> &intervals,
    double maxErrorFraction,
    double extremeMaxErrFract) 
{
    TRACE_FUNCTION();

    // Per the API, an empty intervals means use the full interval of each
    // spline
    bool useFullRangeForEach = intervals.empty();

    if (useFullRangeForEach) {
        WorkParallelForEach(splines.begin(), splines.end(), 
                            [&](TsSpline *spline)
        {
            TsSimplifySpline(spline,
                    GfMultiInterval(spline->GetFrameRange()),
                    maxErrorFraction, extremeMaxErrFract);
        });
        return;
    }

    const size_t numSplines = splines.size();

    // If we're here, intervals was not empty, and hence must be the same size
    // as splines
    if (splines.size() != intervals.size()) {
        TF_CODING_ERROR("splines size %zd != intervals size %zd",
                splines.size(), intervals.size());
        return;
    }
    // If just one, don't bother to construct the arg for WorkParallelForEach,
    // just call TsSimplifySpline()
    if (numSplines == 1) {
        TsSimplifySpline(splines[0], intervals[0], maxErrorFraction,
                extremeMaxErrFract);
        return;
    }

    // Make an argument for WorkParallelForEach that we can pass iterators to
    typedef std::pair<TsSpline *, GfMultiInterval> SplineAndIntervals;
    std::vector<SplineAndIntervals> args;
    args.reserve(numSplines);
    for (size_t i = 0; i < numSplines; i++)
        args.emplace_back(splines[i], intervals[i]);

    WorkParallelForEach(args.begin(), args.end(), 
                        [&](SplineAndIntervals &splineAndIntervals)
    {
        TsSimplifySpline(splineAndIntervals.first, 
                           splineAndIntervals.second,
            maxErrorFraction, extremeMaxErrFract);
    });
}

void TsResampleSpline(TsSpline* spline,
                        const GfMultiInterval &inputIntervals,
                        double maxErrorFraction)
{
    if (!spline) {
        TF_CODING_ERROR("Invalid spline.");
        return;
    }

    // Reduce the intervals to a valid range.
    GfMultiInterval intervals = inputIntervals;
    intervals.Intersect(spline->GetFrameRange());

    TsSpline splineCopy = *spline;

    // Sample in all intervals by adding keyframes on every frame.
    TF_FOR_ALL(it, intervals)
    {
        for (double t = it->GetMin(); t <= it->GetMax(); ++t)
            splineCopy.Breakdown(t, TsKnotBezier, false, 0.33);
    }

    *spline = splineCopy;

    // Now simplify to get rid of unneeded keyframes.
    TsSimplifySpline(spline, intervals, maxErrorFraction);
}

PXR_NAMESPACE_CLOSE_SCOPE
