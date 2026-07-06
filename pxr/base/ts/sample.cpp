//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/iterator.h"
#include "pxr/base/ts/sample.h"
#include "pxr/base/ts/splineData.h"
#include "pxr/base/ts/regressionPreventer.h"
#include "pxr/base/ts/debugCodes.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/tf/diagnostic.h"

#include <algorithm>
#include <cmath>

PXR_NAMESPACE_OPEN_SCOPE

// XXX: Should this go in sample.h? Or maybe even knotData.h?
using Ts_DoubleKnotData = Ts_TypedKnotData<double>;

////////////////////////////////////////////////////////////////////////////////
// SAMPLING

namespace
{

constexpr double inf = std::numeric_limits<double>::infinity();

// Constants for constructing GfInterval objects
constexpr bool OPEN = false;
constexpr bool CLOSED = true;

// Writes the pre values of the given output data. To finalize the data,
// call _FinalizeKnotData.
void _SetKnotDataPreValues(
    const Ts_Segment& segment,
    Ts_DoubleKnotData& data
) {
    data.time = segment.p1[0];
    data.preTanAlgorithm = TsTangentAlgorithmNone;

    if (segment.interp == Ts_SegmentInterp::PreExtrap ||
        segment.interp == Ts_SegmentInterp::ValueBlock)
    {
        data.preTanWidth = 0.0;
        data.preTanSlope = 0.0;
    } else {
        data.preTanWidth = segment.p1[0] - segment.t1[0];
        data.preTanSlope =
            data.preTanWidth == 0
            ? 0
            : (segment.p1[1] - segment.t1[1]) / data.preTanWidth;
    }

    data.preValue = segment.p1[1];
}

// Writes the post values of the given output data. To finalize the data,
// call _FinalizeKnotData.
void _SetKnotDataValues(
    const Ts_Segment& segment,
    Ts_DoubleKnotData& data
) {
    data.time = segment.p0[0];
    data.postTanAlgorithm = TsTangentAlgorithmNone;

    if (segment.interp == Ts_SegmentInterp::PostExtrap ||
        segment.interp == Ts_SegmentInterp::ValueBlock)
    {
        data.postTanWidth = 0.0;
        data.postTanSlope = 0.0;
    } else {
        data.postTanWidth = segment.t0[0] - segment.p0[0];
        data.postTanSlope =
            data.postTanWidth == 0
            ? 0
            : (segment.t0[1] - segment.p0[1]) / data.postTanWidth;
    }

    if (segment.p1[0] != +inf) {
        data.nextInterp = segment.GetInterpMode();
    }
    if (data.nextInterp == TsInterpCurve) {
        switch (segment.interp)
        {
            case Ts_SegmentInterp::Hermite:
                data.curveType = TsCurveTypeHermite;
                break;
            case Ts_SegmentInterp::Bezier:
                data.curveType = TsCurveTypeBezier;
                break;
            default:
                TF_VERIFY(false,
                            "Unable to set knot data curve type from "
                            "unknown segment curve type.");
        }
    }

    data.value = segment.p0[1];
}

void _FinalizeKnotData(Ts_DoubleKnotData& data) {
    if (data.value == data.preValue) {
        data.dualValued = false;
        data.preValue = 0;
    } else {
        data.dualValued = true;
    }
}

TsSplineSampleSource _GetSegmentSource(
    const Ts_Segment* segment,
    const Ts_SplineData* const data)
{
    GfInterval knotInterval(data->times.front(), data->times.back(),
                            CLOSED, OPEN);
    GfInterval loopedInterval = data->loopParams.GetLoopedInterval();
    GfInterval splineInterval = data->HasInnerLoops()
        ? knotInterval | loopedInterval
        : knotInterval;

    const TsTime& segmentStartTime = segment->p0[0];
    const TsTime& segmentEndTime = segment->p1[0];
    if (segmentStartTime < splineInterval.GetMin())
    {
        return data->preExtrapolation.IsLooping()
            ? TsSourcePreExtrapLoop
            : TsSourcePreExtrap;
    }
    if (segmentEndTime > splineInterval.GetMax())
    {
        return data->postExtrapolation.IsLooping()
            ? TsSourcePostExtrapLoop
            : TsSourcePostExtrap;
    }

    if (!data->HasInnerLoops()) {
        return TsSourceKnotInterp;
    }

    GfInterval segmentInterval =
        GfInterval(segmentStartTime, segmentEndTime, CLOSED, OPEN);
    GfInterval protoInterval = data->loopParams.GetPrototypeInterval();
    if (protoInterval.Contains(segmentInterval)) {
        return TsSourceInnerLoopProto;
    } else if (loopedInterval.Contains(segmentInterval)) {
        return segmentStartTime < protoInterval.GetMin()
            ? TsSourceInnerLoopPreEcho
            : TsSourceInnerLoopPostEcho;
    }
    return TsSourceKnotInterp;
}

void
_SubdivideBezier(const GfVec2d cp[4],
                 const double u,
                 GfVec2d leftCp[4],
                 GfVec2d rightCp[4])
{
    // Intermediate points
    GfVec2d cp01   = GfLerp(u, cp[0], cp[1]);
    GfVec2d cp12   = GfLerp(u, cp[1], cp[2]);
    GfVec2d cp23   = GfLerp(u, cp[2], cp[3]);

    GfVec2d cp012  = GfLerp(u, cp01, cp12);
    GfVec2d cp123  = GfLerp(u, cp12, cp23);

    GfVec2d cp0123 = GfLerp(u, cp012, cp123);

    // Left Bezier
    leftCp[0] = cp[0];
    leftCp[1] = cp01;
    leftCp[2] = cp012;
    leftCp[3] = cp0123;

    // Right Bezier
    rightCp[0] = cp0123;
    rightCp[1] = cp123;
    rightCp[2] = cp23;
    rightCp[3] = cp[3];
}

void
_SampleBezier(GfVec2d cp[4],
              const GfInterval& segmentInterval,
              TsSplineSampleSource source,
              double timeScale,
              double valueScale,
              double tolerance,
              Ts_SampleDataInterface* sampledSpline)
{
    // Bezier curves exist entirely within the bounds of their control points
    // so we compute the height of the bounding box. This is the length of the
    // vectors perpendicular to the baseline from cp[0] to cp[3].
    //
    // All height computations are done in "tolerance space", scaled by
    // timeScale and valueScale so we can just compare the length of the
    // perpendicular vectors to _tolerance (really compare length squared to
    // _tolerance squared). If greater than _tolerance then we split the Bezier
    // into 2 halves and recurse on each one.
    GfVec2d scaleVec(timeScale, valueScale);
    GfVec2d baseVec = GfCompMult(scaleVec, cp[3] - cp[0]);
    GfVec2d vec1 = GfCompMult(scaleVec, cp[1] - cp[0]);
    GfVec2d vec2 = GfCompMult(scaleVec, cp[2] - cp[0]);

    // baseVec is the vector from cp[0] to cp[3]. Compute the perpendicular
    // distance from that base line to each of cp[1] and cp[2]. The values
    // t1 * baseVec and t2 * baseVec are the projections of vec1 and vec2 onto
    // baseVec. So (vec1 - t1 * baseVec) is the perpendicular component of vec1.
    double lenSquared = baseVec.GetLengthSq();
    double t1 = vec1 * baseVec / lenSquared;
    double t2 = vec2 * baseVec / lenSquared;

    double h1Squared = (vec1 - t1 * baseVec).GetLengthSq();
    double h2Squared = (vec2 - t2 * baseVec).GetLengthSq();

    // If the length of both perpendiculars are <= tolerance, we're done, baseVec
    // is our linear approximation of this part of the curve.
    if (std::max(h1Squared, h2Squared) <= tolerance * tolerance) {
        double t1 = cp[0][0];
        double t2 = cp[3][0];
        double v1 = cp[0][1];
        double v2 = cp[3][1];

        if (t1 < segmentInterval.GetMin()) {
            double u = (segmentInterval.GetMin() - t1) / (t2 - t1);
            t1 = GfLerp(u, t1, t2);
            v1 = GfLerp(u, v1, v2);
        }
        if (t2 > segmentInterval.GetMax()) {
            double u = (segmentInterval.GetMax() - t1) / (t2 - t1);
            t2 = GfLerp(u, t1, t2);
            v2 = GfLerp(u, v1, v2);
        }

        sampledSpline->AddSegment(t1, v1, t2, v2, source);
    } else {
        // The height of the control point bounding box is greater than
        // _tolerance, so split the curve and recurse on the halves.
        GfVec2d leftCp[4], rightCp[4];
        _SubdivideBezier(cp, 0.5, leftCp, rightCp);
        bool doLeft = segmentInterval.Intersects(GfInterval(leftCp[0][0],
                                                            leftCp[3][0]));
        bool doRight = segmentInterval.Intersects(GfInterval(rightCp[0][0],
                                                             rightCp[3][0]));
        if (doLeft) {
            _SampleBezier(leftCp,
                            segmentInterval,
                            source,
                            timeScale,
                            valueScale,
                            tolerance,
                            sampledSpline);
        }
        if (doRight) {
            _SampleBezier(rightCp,
                            segmentInterval,
                            source,
                            timeScale,
                            valueScale,
                            tolerance,
                            sampledSpline);
        }
    }
}

// Get simplification of a looping extrapolation. If not looping, return
// original extrapolation.
TsExtrapolation
_SimplifyOuterLooping(
    const Ts_SplineData* data,
    const double boundaryAdjacentTime,
    const TsExtrapolation& extrap)
{
    if (!extrap.IsLooping()) {
        return extrap;
    }

    const auto& times = data->times;
    if (times.empty()) {
        return TsExtrapolation(TsExtrapValueBlock);
    }

    const bool hasBoundary = extrap.loopBoundaryTime.has_value();
    if (hasBoundary) {
        const double boundary = extrap.loopBoundaryTime.value();
        const bool found = std::binary_search(times.begin(), times.end(),
                                              boundary);
        if (found && boundary == boundaryAdjacentTime) {
            return TsExtrapolation(TsExtrapHeld);
        } else if (!found) {
            return TsExtrapolation(TsExtrapValueBlock);
        }
    } else if (data->times.size() == 1 && !data->HasInnerLoops()) {
        return TsExtrapolation(TsExtrapHeld);
    }

    return extrap;
}

// Returns a shifted loop boundary time after baking enough knots to cover
// numSegments into the given resultData for pre extrapolation looping.
double
_BakeAndShiftPreExtrapLoop(
    const Ts_SplineData* const data,
    const GfInterval& interval,
    const size_t numSegments,
    Ts_SplineData* resultData)
{
    const double finiteTime = interval.GetMax();
    const GfInterval startPoint(finiteTime, finiteTime, CLOSED, CLOSED);

    Ts_SegmentIterator it(data, startPoint);
    Ts_TypedKnotData<double> d;
    Ts_Segment segment = *it;
    const double shiftedBoundary = segment.p0[0];

    // Scenario A: First segment starts exactly at the open interval max.
    // Scenario B: First segment starts exactly at the closed interval max.
    // Scenario C: The interval max lies somewhere in (last knot time, +inf)
    // Scenario D: The interval max lies between two knots.
    const bool scenarioA = segment.p0[0] == interval.GetMax()
                           && interval.IsMaxOpen();
    const bool scenarioB = segment.p0[0] == interval.GetMax()
                           && interval.IsMaxClosed();
    const bool scenarioC = segment.p1[0] == +inf;
    const bool scenarioD = !scenarioA && !scenarioB && !scenarioC;

    // Add a pre-values-only knot if the finite max is between two knots,
    // so that we can breakdown a knot at the max later in Ts_Truncate.
    if (scenarioD) {
        _SetKnotDataPreValues(segment, d);
        d.value = d.preValue;
        _FinalizeKnotData(d);
        resultData->SetKnotFromDouble(&d, VtDictionary());
        d = Ts_TypedKnotData<double>();
    }

    // Set the values of the ending knot unless we want to truncate
    // away the values side of the knot at the open interval max.
    if (!scenarioA) {
        _SetKnotDataValues(segment, d);
    }

    for (size_t i = 0; i < numSegments; ++i) {
        const bool success = it.StepBackward();
        TF_VERIFY(success,
                  "Ts_SegmentIterator should be able to StepBackward "
                  "infinitely into looped extrap regions");

        segment = *it;
        _SetKnotDataPreValues(segment, d);

        // If we truncated away the values side of the knot earlier, set the
        // knot's value from the recorded pre-value so that any held
        // extrapolation behaves as expected.
        if (scenarioA && i == 0) {
            d.value = d.preValue;
        }

        _FinalizeKnotData(d);
        resultData->SetKnotFromDouble(&d, VtDictionary());
        d = Ts_TypedKnotData<double>();

        _SetKnotDataValues(segment, d);
    }

    // We never need the pre-values of the first knot, so don't call
    // _FinalizeKnotData.
    resultData->SetKnotFromDouble(&d, VtDictionary());

    return shiftedBoundary;
}

// Returns a shifted loop boundary time after baking enough knots to cover
// numSegments into the given resultData for post extrapolation looping.
double
_BakeAndShiftPostExtrapLoop(
    const Ts_SplineData* const data,
    const GfInterval& interval,
    const size_t numSegments,
    Ts_SplineData* resultData)
{
    const double finiteTime = interval.GetMin();
    const GfInterval startPoint(finiteTime, finiteTime, CLOSED, CLOSED);

    Ts_SegmentIterator it(data, startPoint);
    Ts_TypedKnotData<double> d;
    Ts_Segment segment = *it;

    // Scenario A: First segment starts exactly at the interval min.
    // Scenario B: The interval min lies somewhere in (-inf, first knot time)
    // Scenario C: The interval min lies between two knots.
    const bool scenarioA = segment.p0[0] == interval.GetMin();
    const bool scenarioB = segment.p0[0] == -inf;
    const bool scenarioC = !scenarioA && !scenarioB;

    // Add a post-values-only knot if the finite min is between two knots,
    // so that we can breakdown a knot at the min later in Ts_Truncate.
    if (scenarioC) {
        _SetKnotDataValues(segment, d);
        resultData->SetKnotFromDouble(&d, VtDictionary());
        d = Ts_TypedKnotData<double>();
    }

    // Set pre values of the next knot only if the interval contains
    // those pre values.
    if (scenarioB || scenarioC) {
        _SetKnotDataPreValues(segment, d);
    }

    const double shiftedBoundary = scenarioA ? segment.p0[0]
                                             : segment.p1[0];

    for (size_t i = 0; i < numSegments; ++i) {
        // When the iterator's first segment starts exactly at the interval
        // min, we don't need to step the iterator forward to access the
        // first full segment.
        const bool scenarioAStart = scenarioA && i == 0;
        if (!scenarioAStart) {
            const bool success = it.StepForward();
            TF_VERIFY(success,
                      "Ts_SegmentIterator should be able to StepForward "
                      "infinitely into looped extrap regions");
            segment = *it;
        }

        _SetKnotDataValues(segment, d);

        // We should only finalize knots that have both pre and post data
        // written to them. Scenario A's first knot only has post values.
        if (!scenarioAStart) {
            _FinalizeKnotData(d);
        }

        resultData->SetKnotFromDouble(&d, VtDictionary());
        d = Ts_TypedKnotData<double>();

        _SetKnotDataPreValues(segment, d);
    }

    // Complete the last knot
    bool success = it.StepForward();
    TF_VERIFY(success,
              "Ts_SegmentIterator should be able to step forward infinitely"
              "into looping extrap");
    segment = *it;
    _SetKnotDataValues(segment, d);
    _FinalizeKnotData(d);
    resultData->SetKnotFromDouble(&d, VtDictionary());

    return shiftedBoundary;
}

// Requires that extrap was distilled from _SimplifyOuterLooping. Updates
// extrap parameter and creates a Ts_SplineData, which the caller is
// responsible for deallocating.
Ts_SplineData*
_BakeAndShiftExtrapLoopBoundary(
    const Ts_SplineData* const data,
    const double boundaryOppositeTime,
    const GfInterval& interval,
    const bool isPre,
    TsExtrapolation* extrap)
{
    if (!extrap->IsLooping()) {
        TF_CODING_ERROR("_BakeAndShiftExtrapLoopBoundary must be called with "
                        "looping extrap, returning 0");
        return nullptr;
    }
    const auto& times = data->times;

    // Get the num segments that are looped over.
    size_t numSegments = 0;
    double loopBoundaryTime = isPre ? times.back() : times.front();
    if (data->HasInnerLoops()) {
        // Inner looping being present indicates that looping extrap doesn't
        // specifiy loopBoundaryTime. We need to count the number of segments.
        Ts_SegmentKnotIterator it(data, GfInterval::GetFullInterval());
        while (!it.AtEnd()) {
            numSegments++;
            ++it;
        }
        TF_VERIFY(numSegments > 0,
                  "The number of segments in correctly looping "
                  "extrapolation should be positive. Ensure that "
                  "_SimplifyOuterLooping was run.");
    } else {
        // Because we ran _SimplifyOuterLooping earlier, the
        // loopBoundaryTime is guaranteed to exist in the times vec.
        if (extrap->loopBoundaryTime.has_value()) {
            loopBoundaryTime = extrap->loopBoundaryTime.value();
        }
        const auto& it = std::lower_bound(times.begin(),
                                          times.end(),
                                          loopBoundaryTime);
        TF_VERIFY(it != times.end() && *it == loopBoundaryTime,
                  "The value of loopBoundaryTime must correspond to a knot "
                  "time. Ensure that _SimplifyOuterLooping was run.");

        // In the post-extrapolation case, subtract one to get the number
        // of segments, because times.end() is one step beyond the actual
        // looping range covered.
        std::ptrdiff_t dist = isPre ? std::distance(times.begin(), it)
                                    : std::distance(it, times.end()) - 1;
        TF_VERIFY(dist >= 1,
                  "The number of segments in correctly looping "
                  "extrapolation should be positive. Ensure that "
                  "_SimplifyOuterLooping was run.");

        numSegments = (size_t) dist;
    }

    // When we are handling infinitely looping pre extrapolation, the max
    // side of the interval is finite. Vice versa for post extrapolation.
    const bool shiftBoundary = isPre ? loopBoundaryTime > interval.GetMax()
                                     : loopBoundaryTime < interval.GetMin();
    
    Ts_SplineData* resultData = nullptr;
    if (shiftBoundary) {

        if (extrap->mode == TsExtrapLoopOscillate) {
            numSegments *= 2;
            extrap->mode = TsExtrapLoopReset;
        }

        resultData = Ts_SplineData::Create(data->GetValueType(), data);
        if (resultData == nullptr) {
            return resultData;
        }

        extrap->loopBoundaryTime =
            isPre ? _BakeAndShiftPreExtrapLoop(data, interval, numSegments,
                                               resultData)
                  : _BakeAndShiftPostExtrapLoop(data, interval, numSegments,
                                                resultData);
    } else {
        resultData = Ts_Bake(data, GfInterval::GetFullInterval(),
                             /* includeExtrapLooops */ false);
    }

    return resultData;
}

// _ReparameterizeExtrapLoop()

} // anonymous namespace

////////////////////////////////////////////////////////////////////////////////
// SAMPLE ENTRY POINT

void
Ts_Sample(
    const Ts_SplineData* const data,
    const GfInterval& timeInterval,
    const double timeScale,
    const double valueScale,
    const double tolerance,
    Ts_SampleDataInterface* sampledSpline)
{
    // All arguments should have been validated before reaching this point,
    // but just to be safe...
    if (!TF_VERIFY((data &&
                    !timeInterval.IsEmpty() &&
                    timeScale > 0.0 &&
                    valueScale > 0.0 &&
                    tolerance > 0.0 &&
                    sampledSpline),
                   "Invalid argument to Ts_Sample."))
    {
        return;
    }

    if (data->times.empty()) {
        return;
    }

    if (!timeInterval.IsFinite()) {
        TF_CODING_ERROR("Cannot provide samples for an infinite interval.");
        return;
    }

    Ts_SegmentIterator it(data, timeInterval);
    TF_VERIFY(!it.AtEnd(),
              "Prior checks in Ts_Sample should guarantee correct "
              "Ts_SegmentIterator initialization.");

    while (!it.AtEnd()) {
        Ts_Segment segment = *it;

        switch (segment.interp) {
            case Ts_SegmentInterp::ValueBlock:
                ++it;
                continue;
            case Ts_SegmentInterp::Held:
            case Ts_SegmentInterp::Linear:
            case Ts_SegmentInterp::PreExtrap:
            case Ts_SegmentInterp::PostExtrap:
            {
                double x0, x1, y0, y1;
                x0 = segment.p0[0];
                y0 = segment.p0[1];
                x1 = segment.p1[0];
                y1 = segment.interp == Ts_SegmentInterp::Held ? y0
                                                              : segment.p1[1];

                // Clamp this segment to timeInterval. We need to interpolate
                // the y value at the edge of timeInterval. This is a little
                // tricky because extrapolation segments end with an infinite
                // time value and a slope instead of a value.  We first need to
                // clamp the infinite values so we can get rid of the
                // infinities, then we can clamp finite segment end points.
                //
                // Note: We add 0.0 so that -0 converts to 0.
                //
                // Compute the slope. Start by assuming it is 0.0
                double slope = 0.0;
                if (x0 == -inf) {
                    // Pre-extrapolation, p0[1] is the slope.
                    slope = segment.p0[1];
                    // Since we know it's infinite, clamp it.
                    x0 = timeInterval.GetMin();
                    y0 = y1 - slope * (x1 - x0) + 0.0;
                } else if (x1 == +inf) {
                    // Post-extrapolation, p1[1] is the slope
                    slope = segment.p1[1];
                    // Since we know it's infinite, clamp it.
                    x1 = timeInterval.GetMax();
                    y1 = y0 + slope * (x1 - x0) + 0.0;
                } else if (segment.interp == Ts_SegmentInterp::Linear) {
                    // Finite linear segment. The math is safe.
                    slope = (segment.p1[1] - segment.p0[1])
                        / (segment.p1[0] - segment.p0[0]);
                }

                // Now clamp the segment to timeInterval.
                if (timeInterval.GetMin() > x0) {
                    x0 = timeInterval.GetMin();
                    y0 = y1 - slope * (x1 - x0) + 0.0;
                }
                if (timeInterval.GetMax() < x1) {
                    x1 = timeInterval.GetMax();
                    y1 = y0 + slope * (x1 - x0) + 0.0;
                }
                const TsSplineSampleSource source =
                    _GetSegmentSource(&segment, data);
                sampledSpline->AddSegment(x0, y0, x1, y1, source);
                break;
            }
            case Ts_SegmentInterp::Bezier:
                Ts_RegressionPreventerBatchAccess::ProcessSegment(
                    &segment, TsAntiRegressionKeepRatio);
            case Ts_SegmentInterp::Hermite:
            {
                // The hermite segment already has correctly scaled tangents.
                GfVec2d cp[4];
                cp[0] = GfVec2d(segment.p0[0], segment.p0[1]);
                cp[1] = GfVec2d(segment.t0[0], segment.t0[1]);
                cp[2] = GfVec2d(segment.t1[0], segment.t1[1]);
                cp[3] = GfVec2d(segment.p1[0], segment.p1[1]);
                const TsSplineSampleSource source =
                    _GetSegmentSource(&segment, data);
                _SampleBezier(cp, timeInterval, source, timeScale,
                              valueScale, tolerance, sampledSpline);
            }
        }

        ++it;
    }
}

////////////////////////////////////////////////////////////////////////////////
// BAKE ENTRY POINT

Ts_SplineData* Ts_Bake(
    const Ts_SplineData* const data,
    const GfInterval& timeInterval,
    const bool includeExtrapLoops)
{
    Ts_SplineData* bakedData = nullptr;
    
    // Check some special cases
    if (!data) {
        // We're done.
        return nullptr;
    }

    // Only a finite number of knots allowed.
    if (includeExtrapLoops &&
        (data->times.size() > 1 || data->HasInnerLoops()) &&
        ((data->preExtrapolation.IsLooping() && !timeInterval.IsMinFinite()) ||
         (data->postExtrapolation.IsLooping() && !timeInterval.IsMaxFinite())))
    {
        TF_CODING_ERROR("Attempt to bake an infinite number of knots.");
        return nullptr;
    }

    if (timeInterval.IsEmpty()) {
        return nullptr;
    }

    // Populate baked data with an empty spline data of the right type. Copy the
    // looping and other parameters from data.
    bakedData = Ts_SplineData::Create(data->GetValueType(), data);
    if (data->times.empty()) {
        // Nothing to bake, just reset the inner loop params. Baked data no
        // longer has inner loops.
        bakedData->loopParams = TsLoopParams();
        return bakedData;
    }

    // Truncate time interval to the knot span if we don't want to iterate
    // extrap loops.
    GfInterval inputInterval = timeInterval;
    if (!includeExtrapLoops) {
        GfInterval knotSpan = GfInterval(data->times.front(),
                                         data->times.back());
        if (data->HasInnerLoops()) {
            knotSpan |= data->loopParams.GetLoopedInterval();
        }
        inputInterval &= knotSpan;
    }

    // Bake -- create the iterator.
    Ts_SegmentIterator it(data, inputInterval);
    TF_VERIFY(!it.AtEnd(), "Ts_SegmentIterator should not be AtEnd() given a "
                           " valid interval and non-empty spline.");

    // Step the iterator back if possible to get information about the pre-side
    // of the first knot.
    Ts_Segment segment = *it;
    if (segment.p0[0] != -inf) {
        const bool success = it.StepBackward();
        TF_VERIFY(success,
                  "Ts_SegmentIterator should be able to step backward "
                  "when the current segment's start time is not -inf.");
        segment = *it;
    }
    Ts_TypedKnotData<double> d;
    _SetKnotDataPreValues(segment, d);

    // Go back to the original start segment of the iterator, or the
    // second segment if the original segment started at time -inf.
    if (segment.p1[0] != +inf) {
        bool success = it.StepForward();
        TF_VERIFY(success,
                  "Ts_SegmentIterator should be able to step forward "
                  "when the current segment's end time is not +inf");
    }

    while (!it.AtEnd()) {
        segment = *it;
        _SetKnotDataValues(segment, d);

        _FinalizeKnotData(d);
        bakedData->SetKnotFromDouble(&d, VtDictionary());
        d = Ts_TypedKnotData<double>();

        _SetKnotDataPreValues(segment, d);
        ++it;
    }

    // Step the iterator forward if possible to get information about the
    // post-side of the last knot. Note that we don't need to add another
    // knot if the previously added knot's time is >= the interval
    // max.
    const bool prevKnotDone = !bakedData->times.empty() &&
        bakedData->times.back() >= inputInterval.GetMax();
    if (segment.p1[0] != +inf && !prevKnotDone) {
        if (d.time == inputInterval.GetMax() && inputInterval.IsMaxOpen()) {
            d.value = d.preValue;
        } else {
            bool success = it.StepForward();
            TF_VERIFY(success,
                    "Ts_SegmentIterator should be able to step forward "
                    "when the current segment's end time is not +inf");
            segment = *it;
            _SetKnotDataValues(segment, d);
        }
        _FinalizeKnotData(d);
        bakedData->SetKnotFromDouble(&d, VtDictionary());
    }

    // reset the inner loop params. Baked data no longer has inner loops.
    bakedData->loopParams = TsLoopParams();

    return bakedData;
}

Ts_SplineData*
Ts_Truncate(
    const Ts_SplineData* const data,
    const GfInterval& interval,
    TsExtrapolation preFallback,
    TsExtrapolation postFallback)
{
    if (interval.IsEmpty()) {
        TF_CODING_ERROR("Cannot truncate spline to an empty interval");
        return nullptr;
    }

    if (data->times.empty()) {
        TF_CODING_ERROR("Cannot truncate empty spline");
        return nullptr;
    }

    double firstTime = data->times.front();
    double lastTime = data->times.back();
    if (data->HasInnerLoops()) {
        const GfInterval lpInterval = data->loopParams.GetLoopedInterval();
        firstTime = std::min(firstTime, lpInterval.GetMin());
        lastTime = std::max(lastTime, lpInterval.GetMax());
    }
    const bool minFinite = interval.IsMinFinite();
    const bool maxFinite = interval.IsMaxFinite();

    // Collapse any degenerate looping extrapolation modes to simpler modes.
    TsExtrapolation preExtrap = data->preExtrapolation;
    preExtrap = _SimplifyOuterLooping(data, firstTime, preExtrap);
    TsExtrapolation postExtrap = data->postExtrapolation;
    postExtrap = _SimplifyOuterLooping(data, lastTime, postExtrap);

    // If both ends are infinite, clone the spline data and return
    if (!minFinite && !maxFinite) {
        Ts_SplineData* resultData = data->Clone();
        if (resultData == nullptr) {
            TF_VERIFY(false,
                      "Ts_Truncate failed to clone input spline data");
            return nullptr;
        }
        resultData->preExtrapolation = preExtrap;
        resultData->postExtrapolation = postExtrap;
        return resultData;
    }

    Ts_SplineData* resultData = nullptr;
    if (minFinite && !maxFinite && postExtrap.IsLooping()) {
        resultData = _BakeAndShiftExtrapLoopBoundary(data, firstTime, interval,
                                                     /* isPre */ false,
                                                     &postExtrap);
    } else if (!minFinite && maxFinite && preExtrap.IsLooping()) {
        resultData = _BakeAndShiftExtrapLoopBoundary(data, lastTime, interval,
                                                     /* isPre */ true,
                                                     &preExtrap);
    } else {
        // We need to clone the data to set simplified extrap if it has
        // degenerate looping into infinite regions, so that baking succeeds.
        // The alternative is to modify baking to allow looping
        // in infinite regions if the looping is degenerate.
        const bool postInfLooping =
            !maxFinite && data->postExtrapolation.IsLooping();
        const bool preInfLooping =
            !minFinite && data->preExtrapolation.IsLooping();

        if (firstTime == lastTime) {
            resultData = data->Clone();
        } else if (postInfLooping || preInfLooping) {
            Ts_SplineData* bakeInput = data->Clone();
            if (postInfLooping) {
                bakeInput->postExtrapolation = postExtrap;
            }
            if (preInfLooping) {
                bakeInput->preExtrapolation = preExtrap;
            }

            resultData = Ts_Bake(bakeInput, interval,
                                 /* includeExtrapLoops */ true);
            delete bakeInput;
        } else {
            resultData = Ts_Bake(data, interval,
                                 /* includeExtrapLoops */ true);
        }
    }

    if (resultData == nullptr) {
        TF_VERIFY(false, "Ts_Truncate failed bake of input spline data");
        return nullptr;
    }
    resultData->loopParams = TsLoopParams();

    // Write back simplified extrapolation. This ensures we don't
    // breakdown in a looped section that actually behaves like held
    // or value block.
    resultData->preExtrapolation = preExtrap;
    resultData->postExtrapolation = postExtrap;

    // Perform any relevant breakdowns and reduction of knot vectors.
    // 
    // There are three cases:
    // 1. First time is at the interval min exactly. Do nothing.
    // 2. Interval min is bracketed by first time and second time.
    //    Breakdown at min and remove the first knot.
    // 3. Interval min is before the first time. Breakdown at min.
    if (minFinite && resultData->times.front() != interval.GetMin()) {
        const bool removeFirstKnot =
            resultData->times.front() < interval.GetMin();

        GfInterval localAffectedInterval;
        std::string reason;
        const bool success = Ts_Breakdown(resultData,
                                          interval.GetMin(),
                                          /* testOnly */ false,
                                          &localAffectedInterval,
                                          &reason);
        TF_VERIFY(success,
                  "We should be able to breakdown, otherwise something "
                  "has gone wrong earlier in Ts_Truncate; breakdown "
                  "failed due to %s", reason.c_str());

        if (removeFirstKnot) {
            while (resultData->times.front() < interval.GetMin()) {
                resultData->RemoveKnotAtTime(resultData->times.front());
            }
        }

        TF_VERIFY(resultData->times.front() == interval.GetMin(),
                  "Min side is not correctly truncated to the interval min.");
    }

    // Max post-processing is symmetric to min post-processing.
    if (maxFinite && resultData->times.back() != interval.GetMax() &&
        interval.GetMax() != interval.GetMin())
    {
        const bool removeLastKnot =
            resultData->times.back() > interval.GetMax();

        GfInterval localAffectedInterval;
        std::string reason;
        const bool success = Ts_Breakdown(resultData,
                                          interval.GetMax(),
                                          /* testOnly */ false,
                                          &localAffectedInterval,
                                          &reason);
        TF_VERIFY(success,
                  "We should be able to breakdown, otherwise something "
                  "has gone wrong earlier in Ts_Truncate; breakdown "
                  "failed due to %s", reason.c_str());

        if (removeLastKnot) {
            while (resultData->times.back() > interval.GetMax()) {
                resultData->RemoveKnotAtTime(resultData->times.back());
            }
        }

        TF_VERIFY(resultData->times.back() == interval.GetMax(),
                  "Max side is not correctly truncated to the interval max.");
    }
    resultData->preExtrapolation = minFinite ? preFallback : preExtrap;
    resultData->postExtrapolation = maxFinite ? postFallback : postExtrap;

    return resultData;
}

PXR_NAMESPACE_CLOSE_SCOPE
