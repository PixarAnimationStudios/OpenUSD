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

// Writes the pre values of the given output data.
void _SetKnotDataPreValues(
    const Ts_Segment& segment,
    Ts_DoubleKnotData& data
) {
    data.time = segment.p1[0];

    data.preTanAlgorithm = TsTangentAlgorithmNone;
    data.preTanWidth = segment.p1[0] - segment.t1[0];
    data.preTanSlope =
        data.preTanWidth == 0
        ? 0
        : (segment.p1[1] - segment.t1[1]) / data.preTanWidth;

    data.preValue = segment.p1[1];
}

// This function requires that the pre-values on the given knot data have
// already been correctly set. If they have not yet been set, this
// function will write values with potentially incorrect semantics.
void _SetKnotDataValues(
    const Ts_Segment& segment,
    Ts_DoubleKnotData& data
) {
    data.postTanAlgorithm = TsTangentAlgorithmNone;
    data.postTanWidth = segment.t0[0] - segment.p0[0];
    data.postTanSlope =
        data.postTanWidth == 0
        ? 0
        : (segment.t0[1] - segment.p0[1]) / data.postTanWidth;

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

                // Get the slope from extrapolation segments.
                // slope = (y1-y0)/(x1-x0)
                double slope = 0.0;
                if (segment.p0[0] == -inf) {
                    slope = segment.p0[1];
                } else if (segment.p1[0] == +inf) {
                    slope = segment.p1[1];
                } else if (segment.interp == Ts_SegmentInterp::Linear) {
                    slope = (segment.p1[1] - segment.p0[1])
                        / (segment.p1[0] - segment.p0[0]);
                }

                // Add 0.0 so that -0 displays as 0 instead.
                if (timeInterval.GetMin() > x0) {
                    x0 = timeInterval.GetMin();
                    y0 = -(slope * (x1-x0) - y1) + 0.0;
                }
                if (timeInterval.GetMax() < x1) {
                    x1 = timeInterval.GetMax();
                    y1 = slope * (x1 - x0) + y0 + 0.0;
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

        bakedData->SetKnotFromDouble(&d, VtDictionary());
        d = Ts_TypedKnotData<double>();

        _SetKnotDataPreValues(segment, d);
        ++it;
    }

    // Step the iterator forward if possible to get information about the
    // post-side of the last knot.
    if (segment.p1[0] != +inf) {
        bool success = it.StepForward();
        TF_VERIFY(success,
                  "Ts_SegmentIterator should be able to step forward "
                  "when the current segment's end time is not +inf");
        _SetKnotDataValues(segment, d);
        bakedData->SetKnotFromDouble(&d, VtDictionary());
    }

    // reset the inner loop params. Baked data no longer has inner loops.
    bakedData->loopParams = TsLoopParams();

    return bakedData;
}


PXR_NAMESPACE_CLOSE_SCOPE
