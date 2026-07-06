//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// Test the internal Ts_SegmentIterator

#include "pxr/pxr.h"
#include "pxr/base/ts/iterator.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/splineData.h"

#include "pxr/base/tf/enum.h"
#include "pxr/base/gf/vec2d.h"

#include <iostream>
#include <optional>
#include <set>

PXR_NAMESPACE_USING_DIRECTIVE

// Iterator test direction. Ts_SegmentIterator currently only iterates forward,
// but the sub-iterators that it uses all iterate in either direction to support
// oscillating extrapolation loops.
enum Dir {Fwd, Rev};

// Get a convenient infinite value.
constexpr double inf = std::numeric_limits<double>::infinity();

// Constants for constructing GfInterval objects
constexpr bool OPEN = false;
constexpr bool CLOSED = true;

// Global test state
static bool verbose = false;
static std::string testSplineName;
static std::set<std::string> testSplineNames;

////////////////////////////////////////////////////////////////
// Helper methods

// Quickly create a knot. Constructing a populated TsKnot requires multiple lines
// of C++ code. This is a quick one-liner.
static
TsKnot K(double time, double value,
         double preWidth, double preSlope,
         double postWidth, double postSlope,
         TsInterpMode interp)
{
    TsKnot knot;
    knot.SetTime(time);
    knot.SetValue(value);
    knot.SetPreTanWidth(preWidth);
    knot.SetPreTanSlope(preSlope);
    knot.SetPostTanWidth(postWidth);
    knot.SetPostTanSlope(postSlope);
    knot.SetNextInterpolation(interp);
    return knot;
};

// A spline with a name and segment values.
class TestCase
{
public:
    std::string name;

    TsSpline spline;

    // The expected segments that an iterator should produce in the absence
    // of any extrapolation looping. The GenSegments function below will
    // return a reduced or expanded version of these segments for a particular
    // range.
    std::vector<Ts_Segment> segments;

    // If extrapolation repeat looping is in effect, these are the expected offsets
    // for each iteration.
    double preExtrapValueOffset = 0.0;
    double postExtrapValueOffset = 0.0;
};

// Our test cases. simpleInnerLoop and simpleSpline are duplicates of the same
// named splines in the "museum" but the others are unique to this test.
static TestCase oneKnot;
static TestCase oneKnotExtrapLoop;
static TestCase simpleInnerLoop;
static TestCase simpleInnerLoopDualValued;
static TestCase twoKnotBezier;
static TestCase simpleSpline;
static TestCase simpleSplineDualValued;
static TestCase longLoop;
static TestCase extrapValueBlock;
static TestCase extrapHeld;
static TestCase extrapLinear;
static TestCase extrapSloped;
static TestCase extrapReset;
static TestCase extrapResetDualValued;
static TestCase extrapResetLoopBoundary;
static TestCase extrapResetInvalidLoopBoundary;
static TestCase extrapRepeat;
static TestCase extrapRepeatDualValuedLoopBoundary;
static TestCase extrapOscillate;
static TestCase extrapOscillateLoopBoundary;

// Given a vector of segments for the regular, non-extrap-looped section of a
// spline, generate a vector of segments for the given time interval.  Note that
// this does not compute the segment values from the knots. It just filters and
// possibly repeats-with-offsets values from the testCase.segments vector.
static
std::vector<Ts_Segment> GenSegments(const TestCase& testCase,
                                    GfInterval interval,
                                    Dir direction)
{
    // Copy existing testCase.segments values into a new vector that accounts
    // for the extrapolation looping and limiting to the input time interval.
    // Note that this same method is used to generate tests for all four
    // of the segment iterator types.
    std::vector<Ts_Segment> result;

    if (testCase.segments.empty()) {
        return result;
    }

    const Ts_Segment& seg0 = testCase.segments.front();
    const Ts_Segment& seg1 = testCase.segments.back();

    const size_t numKnots = testCase.spline.GetKnots().size();

    // Get the first and last knot time. For extrap segments, the first and last
    // times are infinite so get the knot time from the other end of the
    // segment.
    const double segTime0 = (std::isfinite(seg0.p0[0])
                             ? seg0.p0[0]
                             : seg0.p1[0]);
    const double segTime1 = (std::isfinite(seg1.p1[0])
                             ? seg1.p1[0]
                             : seg1.p0[0]);
    const double segTimeSpan = segTime1 - segTime0;

    const TsExtrapolation preExtrap = testCase.spline.GetPreExtrapolation();
    const TsExtrapolation postExtrap = testCase.spline.GetPostExtrapolation();

    // Calculate extrap loop informaton
    double preLoopEndTime = segTime1;
    double postLoopStartTime = segTime0;
    bool preExtrapLoop = preExtrap.IsLooping() && segTimeSpan > 0;
    bool postExtrapLoop = postExtrap.IsLooping() && segTimeSpan > 0;

    const TsKnotMap knots = testCase.spline.GetKnots();
    if (preExtrapLoop && preExtrap.loopBoundaryTime.has_value()) {
        const double boundary = preExtrap.loopBoundaryTime.value();
        if (boundary == segTime0 || knots.find(boundary) == knots.end()) {
            preExtrapLoop = false;
        } else {
            preLoopEndTime = boundary;
        }
    }
    if (postExtrapLoop && postExtrap.loopBoundaryTime.has_value()) {
        const double boundary = postExtrap.loopBoundaryTime.value();
        if (boundary == segTime1 || knots.find(boundary) == knots.end()) {
            postExtrapLoop = false;
        } else {
            postLoopStartTime = boundary;
        }
    }

    const double minTime = interval.GetMin();
    const double maxTime = interval.GetMax();
    const double preSpan  = preLoopEndTime - segTime0;
    const double postSpan = segTime1 - postLoopStartTime;

    const int minIter = (preSpan > 0 && preExtrapLoop
                         ? int(std::floor((minTime - segTime0) / preSpan))
                         : 0);
    const int maxIter = (postSpan > 0 && postExtrapLoop
                         ? int(std::ceil((maxTime - segTime1) / postSpan))
                         : 0);

    for (int iter = minIter; iter <= maxIter; ++iter) {
        // Calculate loop region
        double regionStartTime = segTime0;
        double regionEndTime = segTime1;
        if (iter < 0) {
            regionEndTime = preLoopEndTime;
        } else if (iter > 0) {
            regionStartTime = postLoopStartTime;
        }
        const GfInterval regionInterval =
            GfInterval(regionStartTime, regionEndTime, CLOSED, OPEN);
        double span = regionEndTime - regionStartTime;

        const double segValueOffset = (iter < 0
                                       ? testCase.preExtrapValueOffset
                                       : testCase.postExtrapValueOffset);

        const bool oscillating = (iter < 0
                                  ? preExtrap.mode == TsExtrapLoopOscillate
                                  : postExtrap.mode == TsExtrapLoopOscillate);
        bool reversed = oscillating && (iter % 2 != 0);

        if (reversed) {
            GfVec2d shift1(iter * span + regionStartTime,
                           iter * segValueOffset);
            double timeShift2 = regionEndTime;

            const double t1 = -(interval.GetMin() - shift1[0]) + timeShift2;
            const double t0 = -(interval.GetMax() - shift1[0]) + timeShift2;
            GfInterval iterInterval = GfInterval(t0, t1, CLOSED, OPEN);

            for (auto segIt = testCase.segments.rbegin();
                 segIt != testCase.segments.rend();
                 ++segIt)
            {
                Ts_Segment seg = *segIt;
                // Skip extrap segments if we're looping and the spline has
                // more than one knot.
                if ((numKnots > 1 && ((!std::isfinite(seg.p0[0]) &&
                                       preExtrapLoop) ||
                                      (!std::isfinite(seg.p1[0]) &&
                                       postExtrapLoop))) ||
                    (iter != 0 && (!std::isfinite(seg.p0[0]) ||
                                   !std::isfinite(seg.p1[0]))))
                {
                    continue;
                }

                // Only include segments within the region interval for
                // extrapolation looping.
                if (iter != 0 &&
                    !regionInterval.Intersects(
                        GfInterval(seg.p0[0], seg.p1[0], CLOSED, OPEN)))
                {
                    continue;
                }

                // If this segment is in the iteration interval, include it in
                // the result.
                if (iterInterval.Intersects(
                        GfInterval(seg.p0[0], seg.p1[0], CLOSED, OPEN)))
                {
                    result.push_back(-(seg - timeShift2) + shift1);
                }
            }
        } else {
            GfVec2d shift1(iter * span,
                           iter * segValueOffset);

            // Shift the interval for this iteration
            GfInterval iterInterval = interval - GfInterval(shift1[0]);

            for (const auto& seg : testCase.segments) {
                // Skip extrap segments if we're looping and the spline has
                // more than one knot.
                if ((numKnots > 1 && ((!std::isfinite(seg.p0[0]) &&
                                       preExtrapLoop) ||
                                      (!std::isfinite(seg.p1[0]) &&
                                       postExtrapLoop))) ||
                    (iter != 0 && (!std::isfinite(seg.p0[0]) ||
                                   !std::isfinite(seg.p1[0]))))
                {
                    continue;
                }

                // Only include segments within the region interval for
                // extrapolation looping.
                if (iter != 0 &&
                    !regionInterval.Intersects(
                        GfInterval(seg.p0[0], seg.p1[0], CLOSED, OPEN)))
                {
                    continue;
                }

                // If this segment is in the iteration interval, include it in
                // the result.
                if (iterInterval.Intersects(
                        GfInterval(seg.p0[0], seg.p1[0], CLOSED, OPEN)))
                {
                    result.push_back(seg + shift1);
                }
            }
        }
    }

    if (direction == Rev) {
        std::reverse(result.begin(), result.end());
    }

    return result;
}

static
void InitTestCases()
{
    // In all the cases below, the segments array is hand calculated.

    const TsExtrapolation linearEx(TsExtrapLinear);

    // ================ OneKnot ================
    // Test a spline with one knot.
    oneKnot.name = "OneKnot";
    oneKnot.spline.SetKnots(
        {
            K(1.0, 1.0, 0.0, 0.0, 0.0, 0.0, TsInterpHeld)
        });
    oneKnot.segments =
        {
            {{-inf, 0.0},
             {0.0, 0.0},
             {0.0, 0.0},
             {1.0, 1.0},
             Ts_SegmentInterp::PreExtrap},
            {{1.0, 1.0},
             {0.0, 0.0},
             {0.0, 0.0},
             {+inf, 0.0},
             Ts_SegmentInterp::PostExtrap}
        };
    oneKnot.spline.SetPreExtrapolation(TsExtrapHeld);
    oneKnot.spline.SetPostExtrapolation(TsExtrapHeld);

    // ================ OneKnotExtrapLoop ================
    // Test a spline with one knot with looping extrap.
    oneKnotExtrapLoop = oneKnot;
    oneKnotExtrapLoop.name = "OneKnotExtrapLoop";
    oneKnotExtrapLoop.spline.SetPreExtrapolation(TsExtrapLoopReset);
    oneKnotExtrapLoop.spline.SetPostExtrapolation(TsExtrapLoopOscillate);

    // ================ TwoKnotBezier ================
    // Test a spline without any inner looping. This is a clone of
    // "TwoKnotBezier" from the museum
    twoKnotBezier.name = "TwoKnotBezier";
    twoKnotBezier.spline.SetKnots(
        {
            K(1.0, 1.0, 0.0, 0.0, 0.5, 1.0, TsInterpCurve),
            K(5.0, 2.0, 0.5, 0.0, 0.0, 0.0, TsInterpCurve)
        });
    twoKnotBezier.segments =
        {
            {{-inf, 0.0},
             {0.0, 0.0},
             {0.0, 0.0},
             {1.0, 1.0},
             Ts_SegmentInterp::PreExtrap},
            {{1.0, 1.0},
             {1.5, 1.5},
             {4.5, 2.0},
             {5.0, 2.0},
             Ts_SegmentInterp::Bezier},
            {{5.0, 2.0},
             {0.0, 0.0},
             {0.0, 0.0},
             {+inf, 0.0},
             Ts_SegmentInterp::PostExtrap},
        };
    testSplineNames.insert(twoKnotBezier.name);

    // ================ SimpleInnerLoop ================
    // This spline is a duplicate of the "SimpleInnerLoop" spline from
    // the museum. It has an inner loop prototype from 137 to 155 and
    // a total inner looping region from 119 to 173
    simpleInnerLoop.name = "SimpleInnerLoop";
    simpleInnerLoop.spline.SetKnots(
        {  //  t      v     pre-tan     post-tan       interp
            K(112,  8.8,  0.0,   0.0,  0.9,  15.0,  TsInterpCurve),
            K(137,  0.0,  1.3,  -5.3,  1.8,  -5.3,  TsInterpCurve),
            K(145,  8.5,  1.0,  12.5,  1.2,  12.5,  TsInterpCurve),
            K(155, 20.2,  0.7, -15.7,  0.8, -15.7,  TsInterpCurve),
            K(181, 38.2,  2.0,  -9.0,  0.0,   0.0,  TsInterpCurve)
        });
    simpleInnerLoop.spline.SetInnerLoopParams(
        TsLoopParams{137, 155, 1, 1, 20.2});

    // The segments are kind of a pain to compute by hand. Other
    // splines have easier values.
    Ts_Segment proto1{{137.0, 0.0},
                      {138.8, -9.54},
                      {144.0, -4.0},
                      {145.0, 8.5},
                      Ts_SegmentInterp::Bezier};
    Ts_Segment proto2{{145.0, 8.5},
                      {146.2, 23.5},
                      {153.7, 27.09},
                      {155.0, 20.2},
                      Ts_SegmentInterp::Bezier};
    GfVec2d offset(18.0, 20.2);

    Ts_Segment pre1 = proto1 - offset;
    Ts_Segment pre2 = proto2 - offset;
    Ts_Segment post1 = proto1 + offset;
    Ts_Segment post2 = proto2 + offset;

    Ts_Segment knots0{{112, 8.8},
                      {112 + 0.9, 8.8 + 0.9 * 15.0},
                      {pre1.p0[0] - 1.3, pre1.p0[1] + 1.3 * 5.3},
                      {pre1.p0[0], pre1.p0[1]},
                      Ts_SegmentInterp::Bezier};
    Ts_Segment knots1{{post2.p1[0], post2.p1[1]},
                      {post2.p1[0] + 1.8, post2.p1[1] - 1.8 * 5.3},
                      {181.0 - 2.0, 38.2 + 2.0 * 9.0},
                      {181.0, 38.2},
                      Ts_SegmentInterp::Bezier};
    Ts_Segment preExtrap{{-inf, 0.0},   // time, slope
                         {0.0, 0.0},    // unused
                         {0.0, 0.0},    // unused
                         {112, 8.8},
                         Ts_SegmentInterp::PreExtrap};
    Ts_Segment postExtrap{{181.0, 38.2},
                          {0.0, 0.0},     // unused
                          {0.0, 0.0},     // unused
                          {+inf, 0.0},    // time, slope
                          Ts_SegmentInterp::PostExtrap};

    simpleInnerLoop.segments =
        {preExtrap,   // [0]
         knots0,      // [1]
         pre1,        // [2]
         pre2,        // [3]
         proto1,      // [4]
         proto2,      // [5]
         post1,       // [6]
         post2,       // [7]
         knots1,      // [8]
         postExtrap}; // [9]

    testSplineNames.insert(simpleInnerLoop.name);

    // ================ SimpleInnerLoopDualValued ================
    simpleInnerLoopDualValued = simpleInnerLoop;
    simpleInnerLoopDualValued.name = "SimpleInnerLoopDualValued";

    // Make the knots at the loop range endpoints dual-valued
    TsKnot knotStart;
    TsKnot knotEnd;
    simpleInnerLoopDualValued.spline.GetKnot(137.0, &knotStart);
    simpleInnerLoopDualValued.spline.GetKnot(155.0, &knotEnd);
    knotStart.SetPreValue(0.0);
    knotEnd.SetPreValue(20.2);
    simpleInnerLoopDualValued.spline.SetKnot(knotStart);
    simpleInnerLoopDualValued.spline.SetKnot(knotEnd);

    // Remove the earliest knot so that the first segment's p1 is
    // dual valued.
    simpleInnerLoopDualValued.spline.RemoveKnot(112.0);

    // Remove the first segment
    Ts_Segment& origSegment = simpleInnerLoopDualValued.segments[0];
    Ts_Segment& destSegment = simpleInnerLoopDualValued.segments[1];
    destSegment.p0 = origSegment.p0;
    destSegment.t0 = origSegment.t0;
    destSegment.t1 = origSegment.t1;
    destSegment.interp = origSegment.interp;
    simpleInnerLoopDualValued.segments.erase(
        simpleInnerLoopDualValued.segments.begin());

    testSplineNames.insert(simpleInnerLoopDualValued.name);

    // ================ simpleSpline ================
    // Living up to its name, all the knots have simple, easy to hand compute values.
    simpleSpline.name = "SimpleSpline";
    simpleSpline.spline.SetKnots(
        {  //  t      v     pre-tan     post-tan       interp
            K(0.0,  0.0,  0.0,   0.0,  0.0,   0.0,  TsInterpHeld),
            K(1.0,  1.0,  0.0,   0.0,  0.0,   0.0,  TsInterpLinear),
            K(2.0,  2.5,  0.0,   0.0,  0.0,   0.0,  TsInterpLinear),
            K(3.0,  2.0,  0.0,   0.0,  0.0,   0.0,  TsInterpLinear),
            K(4.0,  4.0,  0.0,   0.0,  0.0,   0.0,  TsInterpHeld)
        });

    simpleSpline.segments =
        {
            {{-inf, 0.0},
             {0.0, 0.0},
             {0.0, 0.0},
             {0.0, 0.0},
             Ts_SegmentInterp::PreExtrap},
            {{0.0, 0.0},
             {0.0, 0.0},
             {1.0, 1.0},
             {1.0, 1.0},
             Ts_SegmentInterp::Held},
            {{1.0, 1.0},
             {1.0, 1.0},
             {2.0, 2.5},
             {2.0, 2.5},
             Ts_SegmentInterp::Linear},
            {{2.0, 2.5},
             {2.0, 2.5},
             {3.0, 2.0},
             {3.0, 2.0},
             Ts_SegmentInterp::Linear},
            {{3.0, 2.0},
             {3.0, 2.0},
             {4.0, 4.0},
             {4.0, 4.0},
             Ts_SegmentInterp::Linear},
            {{4.0, 4.0},
             {0.0, 0.0},
             {0.0, 0.0},
             {+inf, 0.0},
             Ts_SegmentInterp::PostExtrap}
        };

    testSplineNames.insert(simpleSpline.name);

    // ================ SimpleSplineDualValued ================
    simpleSplineDualValued = simpleSpline;
    simpleSplineDualValued.name = "SimpleSpline";

    // Make the knots at times 1.0, 3.0, 4.0 dual valued.
    TsKnot knot1, knot3, knot4;
    simpleSplineDualValued.spline.GetKnot(1.0, &knot1);
    simpleSplineDualValued.spline.GetKnot(3.0, &knot3);
    simpleSplineDualValued.spline.GetKnot(4.0, &knot4);
    knot1.SetPreValue(5.0); // Pre-value is reported in the segment but doesn't
                            // affect eval of spline since prev interp is held
    knot3.SetPreValue(2.0); // Set pre-value to value, it's still "dual valued"
    knot4.SetPreValue(2.0); // Pre-value will affect the corresponding segment.
    simpleSplineDualValued.spline.SetKnot(knot1);
    simpleSplineDualValued.spline.SetKnot(knot3);
    simpleSplineDualValued.spline.SetKnot(knot4);

    simpleSplineDualValued.segments[1].p1[1] = 5.0;
    simpleSplineDualValued.segments[1].t1[1] = 5.0;
    simpleSplineDualValued.segments[4].p1[1] = 2.0;
    simpleSplineDualValued.segments[4].t1[1] = 2.0;

    testSplineNames.insert(simpleSplineDualValued.name);

    // ================ LongLoop ================
    // LongLoop has inner looping that extends before and after the first and
    // last knot. The first and last knot are also shadowed by the looping.
    longLoop = simpleSpline;
    longLoop.name = "LongLoop";
    longLoop.spline.SetInnerLoopParams(TsLoopParams{1.0, 3.0, 2, 2, 2.0});

    longLoop.segments =
        {
            {{-inf, 0.0},                   ////////////////
             {0.0, 0.0},                    //
             {0.0, 0.0},                    //  0: pre-extrap
             {-3.0, -3.0},                  //
             Ts_SegmentInterp::PreExtrap},  //
            {{-3.0, -3.0},                  /////////////////
             {-3.0, -3.0},                  //
             {-2.0, -1.5},                  //  1: iter -2
             {-2.0, -1.5},                  //
             Ts_SegmentInterp::Linear},     //
            {{-2.0, -1.5},                  //
             {-2.0, -1.5},                  //  2:
             {-1.0, -1.0},                  //
             {-1.0, -1.0},                  //
             Ts_SegmentInterp::Linear},     //
            {{-1.0, -1.0},                  ////////////////
             {-1.0, -1.0},                  //
             {0.0, 0.5},                    //  3: iter -1
             {0.0, 0.5},                    //
             Ts_SegmentInterp::Linear},     //
            {{0.0, 0.5},                    //
             {0.0, 0.5},                    //  4:
             {1.0, 1.0},                    //
             {1.0, 1.0},                    //
             Ts_SegmentInterp::Linear},     //
            {{1.0, 1.0},                    ////////////////
             {1.0, 1.0},                    //
             {2.0, 2.5},                    //  5: prototype
             {2.0, 2.5},                    //
             Ts_SegmentInterp::Linear},     //
            {{2.0, 2.5},                    //
             {2.0, 2.5},                    //  6:
             {3.0, 3.0},                    //
             {3.0, 3.0},                    //
             Ts_SegmentInterp::Linear},     //
            {{3.0, 3.0},                    ////////////////
             {3.0, 3.0},                    //
             {4.0, 4.5},                    //  7: iter 1
             {4.0, 4.5},                    //
             Ts_SegmentInterp::Linear},     //
            {{4.0, 4.5},                    //
             {4.0, 4.5},                    //  8:
             {5.0, 5.0},                    //
             {5.0, 5.0},                    //
             Ts_SegmentInterp::Linear},     //
            {{5.0, 5.0},                    ////////////////
             {5.0, 5.0},                    //
             {6.0, 6.5},                    //  9: iter 2
             {6.0, 6.5},                    //
             Ts_SegmentInterp::Linear},     //
            {{6.0, 6.5},                    //
             {6.0, 6.5},                    // 10:
             {7.0, 7.0},                    //
             {7.0, 7.0},                    //
             Ts_SegmentInterp::Linear},     //
            {{7.0, 7.0},                    ////////////////
             {0.0, 0.0},                    //
             {0.0, 0.0},                    // 11: post-extrap
             {+inf, 0.0},                   //
             Ts_SegmentInterp::PostExtrap}, //
        };

    testSplineNames.insert(longLoop.name);

    // ================ ExtrapValueBlock ================
    // Clone longLoop but add value-block extrapolation.
    extrapValueBlock = longLoop;
    extrapValueBlock.name = "ExtrapValueBlock";
    extrapValueBlock.segments.front().interp = Ts_SegmentInterp::ValueBlock;
    extrapValueBlock.segments.back().interp = Ts_SegmentInterp::ValueBlock;

    extrapValueBlock.spline.SetPreExtrapolation(TsExtrapValueBlock);
    extrapValueBlock.spline.SetPostExtrapolation(TsExtrapValueBlock);

    testSplineNames.insert(extrapValueBlock.name);

    // ================ ExtrapHeld ================
    // Clone longLoop but add held extrapolation
    extrapHeld = longLoop;
    extrapHeld.name = "ExtrapHeld";

    extrapHeld.spline.SetPreExtrapolation(TsExtrapHeld);
    extrapHeld.spline.SetPostExtrapolation(TsExtrapHeld);

    testSplineNames.insert(extrapHeld.name);

    // ================ ExtrapLinear ================
    // Clone longLoop but add linear extrapolation
    extrapLinear = longLoop;
    extrapLinear.name = "ExtrapLinear";
    extrapLinear.segments.front().p0[1] = 1.5;  // pre-extrap slope
    extrapLinear.segments.back().p1[1] = 0.5;  // post-extrap slope

    extrapLinear.spline.SetPreExtrapolation(TsExtrapLinear);
    extrapLinear.spline.SetPostExtrapolation(TsExtrapLinear);

    testSplineNames.insert(extrapLinear.name);

    // ================ ExtrapSloped ================
    // Clone longLoop but add sloped extrapolation
    extrapSloped = longLoop;
    extrapSloped.name = "ExtrapSloped";
    extrapSloped.segments.front().p0[1] = 1.0;  // pre-extrap slope
    extrapSloped.segments.back().p1[1] = 1.0;  // post-extrap slope

    TsExtrapolation sloped(TsExtrapSloped, 1.0);

    extrapSloped.spline.SetPreExtrapolation(sloped);
    extrapSloped.spline.SetPostExtrapolation(sloped);

    testSplineNames.insert(extrapSloped.name);

    // ================ ExtrapReset ================
    // Clone longLoop but add extrapolation reset looping.
    extrapReset = longLoop;
    extrapReset.name = "ExtrapReset";
    extrapReset.spline.SetPreExtrapolation(TsExtrapLoopReset);
    extrapReset.spline.SetPostExtrapolation(TsExtrapLoopReset);

    testSplineNames.insert(extrapReset.name);

    // ================ ExtrapResetDualValued ================
    extrapResetDualValued = extrapReset;
    extrapResetDualValued.name = "ExtrapResetDualValued";

    // Set the "middle knot" of the inner loop to dual valued
    TsKnot knotExtrapResetDV;
    extrapResetDualValued.spline.GetKnot(2.0, &knotExtrapResetDV);
    knotExtrapResetDV.SetPreValue(2.5);
    extrapResetDualValued.spline.SetKnot(knotExtrapResetDV);

    // ================ ExtrapResetLoopBoundary ================
    extrapResetLoopBoundary = simpleSpline;
    extrapResetLoopBoundary.name = "ExtrapResetLoopBoundary";
    {
        TsExtrapolation ex(TsExtrapLoopReset);
        ex.loopBoundaryTime = 2.0;
        extrapResetLoopBoundary.spline.SetPreExtrapolation(ex);
        ex.loopBoundaryTime = 3.0;
        extrapResetLoopBoundary.spline.SetPostExtrapolation(ex);
    }
    testSplineNames.insert(extrapResetLoopBoundary.name);

    // ================ ExtrapResetInvalidLoopBoundary ================
    extrapResetInvalidLoopBoundary = simpleSpline;
    extrapResetInvalidLoopBoundary.name = "ExtrapResetInvalidLoopBoundary";
    {
        TsExtrapolation ex(TsExtrapLoopReset);
        ex.loopBoundaryTime = 0.0; // held
        extrapResetInvalidLoopBoundary.spline.SetPreExtrapolation(ex);
        ex.loopBoundaryTime = 3.5; // value block
        extrapResetInvalidLoopBoundary.spline.SetPostExtrapolation(ex);
    }
    extrapResetInvalidLoopBoundary.segments.back().interp =
        Ts_SegmentInterp::ValueBlock;
    testSplineNames.insert(extrapResetInvalidLoopBoundary.name);

    // ================ ExtrapRepeatDualValuedLoopBoundary ================
    extrapRepeatDualValuedLoopBoundary = simpleSpline;
    extrapRepeatDualValuedLoopBoundary.name =
        "ExtrapRepeatDualValuedLoopBoundary";
    {
        TsExtrapolation ex(TsExtrapLoopRepeat);
        ex.loopBoundaryTime = 2.0;
        extrapRepeatDualValuedLoopBoundary.spline.SetPreExtrapolation(ex);
        ex.loopBoundaryTime = 2.0;
        extrapRepeatDualValuedLoopBoundary.spline.SetPostExtrapolation(ex);
    }

    {
        // Set the knot at time 0.0 to be dual valued
        TsKnot knot0;
        extrapRepeatDualValuedLoopBoundary.spline.GetKnot(0.0, &knot0);
        knot0.SetPreValue(-0.5);
        extrapRepeatDualValuedLoopBoundary.spline.SetKnot(knot0);
        extrapRepeatDualValuedLoopBoundary.segments[0].t1[1] = -0.5;
        extrapRepeatDualValuedLoopBoundary.segments[0].p1[1] = -0.5;

        // Set the knot at time 2.0 to be dual valued
        TsKnot knot2;
        extrapRepeatDualValuedLoopBoundary.spline.GetKnot(2.0, &knot2);
        knot2.SetPreValue(1.5);
        extrapRepeatDualValuedLoopBoundary.spline.SetKnot(knot2);
        extrapRepeatDualValuedLoopBoundary.segments[2].t1[1] = 1.5;
        extrapRepeatDualValuedLoopBoundary.segments[2].p1[1] = 1.5;

        // Set the knot at time 4.0 to be dual valued
        TsKnot knot4;
        extrapRepeatDualValuedLoopBoundary.spline.GetKnot(4.0, &knot4);
        knot4.SetPreValue(4.0);
        knot4.SetValue(6.0);
        extrapRepeatDualValuedLoopBoundary.spline.SetKnot(knot4);
        extrapRepeatDualValuedLoopBoundary.segments[5].t0[1] = 6.0;
        extrapRepeatDualValuedLoopBoundary.segments[5].p0[1] = 6.0;
    }

    extrapRepeatDualValuedLoopBoundary.preExtrapValueOffset = 2.0;
    extrapRepeatDualValuedLoopBoundary.postExtrapValueOffset = 3.5;
    testSplineNames.insert(extrapRepeatDualValuedLoopBoundary.name);

    // ================ ExtrapOscillateLoopBoundary ================
    extrapOscillateLoopBoundary = simpleSpline;
    extrapOscillateLoopBoundary.name = "ExtrapOscillateLoopBoundary";
    {
        TsExtrapolation ex(TsExtrapLoopOscillate);
        ex.loopBoundaryTime = 1.0;
        extrapOscillateLoopBoundary.spline.SetPreExtrapolation(ex);
        ex.loopBoundaryTime = 2.0;
        extrapOscillateLoopBoundary.spline.SetPostExtrapolation(ex);
    }
    testSplineNames.insert(extrapOscillateLoopBoundary.name);

    // ================ ExtrapRepeat ================
    // Same but change extrapolation to repeat looping.
    extrapRepeat = extrapReset;
    extrapRepeat.name = "ExtrapRepeat";
    extrapRepeat.spline.SetPreExtrapolation(TsExtrapLoopRepeat);
    extrapRepeat.spline.SetPostExtrapolation(TsExtrapLoopRepeat);
    extrapRepeat.preExtrapValueOffset = 10.0;
    extrapRepeat.postExtrapValueOffset = 10.0;

    testSplineNames.insert(extrapRepeat.name);

    // ================ ExtrapOscillate ================
    // Same but change to oscillation looping.
    extrapOscillate = extrapReset;
    extrapOscillate.name = "ExtrapOscillate";
    extrapOscillate.spline.SetPreExtrapolation(TsExtrapLoopOscillate);
    extrapOscillate.spline.SetPostExtrapolation(TsExtrapLoopOscillate);

    testSplineNames.insert(extrapOscillate.name);

}

void ReportMismatch(const std::string& title,
                    const std::optional<Ts_Segment> expectedSeg,
                    const std::optional<Ts_Segment> iterSeg)
{
    std::cout << title << ":\n";
    if (expectedSeg) {
        std::cout << "  expected: "
                  << *expectedSeg
                  << std::endl;
    } else {
        std::cout << "  expected: AtEnd" << std::endl;
    }

    if (iterSeg) {
        std::cout << "  iterated: "
                  << *iterSeg
                  << std::endl;
    } else {
        std::cout << "  iterated: AtEnd\n";
    }
    std::cout << std::flush;
}

// Run a test using StepForward or StepBackward, depending on the value of
// the given `backward` arg. This checks that we get the expected segments
// in the expected order within the interval.
template <typename ITER>
static
bool RunStepTest(
    const TestCase& testCase,
    const GfInterval& iterInterval,
    const Dir direction,
    const std::string& titleIn,
    const std::vector<Ts_Segment>& expectedIn,
    bool backward)
{
    bool result = true;

    ITER iter;

    if constexpr(std::is_same_v<ITER, Ts_SegmentIterator>) {
        iter = ITER(testCase.spline, iterInterval);
    } else {
        iter = ITER(testCase.spline, iterInterval, (direction == Rev));
    }

    const std::vector<Ts_Segment> expected = backward
        ? std::vector<Ts_Segment>(expectedIn.rbegin(), expectedIn.rend())
        : expectedIn;
    auto expectedIter = expected.begin();

    const std::string title = titleIn + 
        (backward ? " (StepBackward)" : " (StepForward)");

    bool stepSuccess = true;
    if (backward) {
        // Step forward into AtEnd territory before running the test
        size_t i = 0;
        while (stepSuccess && !iter.AtEnd()) {
            stepSuccess = iter.StepForward();
            i++;
        }
        if (stepSuccess && i > 0) {
            bool success = iter.StepBackward();
            if (!success) {
                std::cout << title << "; "
                          << "StepBackward failed after successful StepForward"
                          << std::endl;
                return false;
            }
            i++;
        }
    }

    while (expectedIter != expected.end() && (!iter.AtEnd() || !stepSuccess))
    {
        const Ts_Segment iterSeg = *iter;
        const Ts_Segment expectedSeg = *expectedIter;
        if (verbose) {
            std::cout << "    " << iterSeg << std::endl;
        }

        if (*iter != *expectedIter) {
            ReportMismatch(title, expectedSeg, iterSeg);
            result = false;
        }

        stepSuccess = backward ? iter.StepBackward()
                               : iter.StepForward();
        ++expectedIter;
    }

    stepSuccess = true;
    while (!iter.AtEnd()) {
        const Ts_Segment iterSeg = *iter;
        if (verbose) {
            std::cout << "    " << iterSeg << std::endl;
        }

        ReportMismatch(title, std::nullopt, iterSeg);

        result = false;
        stepSuccess = backward ? iter.StepBackward()
                               : iter.StepForward();
    }

    while (expectedIter != expected.end()) {
        const Ts_Segment expectedSeg = *expectedIter;

        ReportMismatch(title, expectedSeg, std::nullopt);

        result = false;
        ++expectedIter;
    }
    return result;
}

// Run a test given testCase with a particular iterator type.  iterInterval is
// the input time range to pass to the iterator, but sub-iterators like
// Ts_SegmentPrototypeIterator or Ts_SegmentKnotIterator only ever produce
// segments for their particular domain. That limited domain is passed in
// domainInterval so that GenSegments can produce the correctly limited expected
// results.
template <typename ITER>
static
bool DoOneTest(const TestCase& testCase,
               const GfInterval& iterInterval,
               const GfInterval& domainInterval,
               const Dir direction)
{
    bool result = true;
    if (!testSplineName.empty() && testSplineName != testCase.name) {
        // Skip this test
        return result;
    }

    ITER iter;

    if constexpr(std::is_same_v<ITER, Ts_SegmentIterator>) {
        iter = ITER(testCase.spline, iterInterval);
    } else {
        iter = ITER(testCase.spline, iterInterval, (direction == Rev));
    }

    const std::string title =
        TfStringPrintf("Testing %s: Spline = %s%s",
                       typeid(ITER).name(),
                       testCase.name.c_str(),
                       (direction == Rev ? " (reversed)" : ""));

    if (verbose) {
        std::cout << "\n"
                  << title
                  << " over "
                  << iterInterval
                  << " ..." << std::endl;
    }

    // Generate the expected results from testCase.segments, iterInterval, and
    // domainInterval.
    const std::vector<Ts_Segment> expected =
        GenSegments(testCase, iterInterval & domainInterval, direction);

    auto expectedIter = expected.begin();
    while (!iter.AtEnd() && expectedIter != expected.end()) {
        const Ts_Segment iterSeg = *iter;
        const Ts_Segment expectedSeg = *expectedIter;
        if (verbose) {
            std::cout << "    " << iterSeg << std::endl;
        }

        if (*iter != *expectedIter) {
            ReportMismatch(title, expectedSeg, iterSeg);
            result = false;
        }

        ++iter;
        ++expectedIter;
    }

    while (!iter.AtEnd()) {
        const Ts_Segment iterSeg = *iter;
        if (verbose) {
            std::cout << "    " << iterSeg << std::endl;
        }

        ReportMismatch(title, std::nullopt, iterSeg);

        result = false;
        ++iter;
    }

    while (expectedIter != expected.end()) {
        const Ts_Segment expectedSeg = *expectedIter;

        ReportMismatch(title, expectedSeg, std::nullopt);

        result = false;
        ++expectedIter;
    }

    if (result) {
        result = RunStepTest<ITER>(
            testCase, iterInterval, direction,
            title, expected, /* backward */ true);
    }
    if (result) {
        result = RunStepTest<ITER>(
            testCase, iterInterval, direction,
            title, expected, /* backward */ false);
    }

    if (verbose) {
        std::cout << "    <AtEnd>" << std::endl;
    }

    std::cout << title
              << " over interval " << iterInterval
              << (result ? " Passed." : " FAILED!")
              << std::endl;

    return result;
}

// Test Ts_SegmentPrototypeIterator
static
bool ProtoTest(const TestCase& testCase,
               double minTime,
               double maxTime,
               Dir dir)
{
    const GfInterval iterInterval(minTime, maxTime,
                                  CLOSED, OPEN);

    // The domain is the inner loop prototype.
    const TsLoopParams lp = testCase.spline.GetInnerLoopParams();
    const GfInterval domainInterval = lp.GetPrototypeInterval();

    return DoOneTest<Ts_SegmentPrototypeIterator>(
        testCase, iterInterval, domainInterval, dir);
};

// Test Ts_SegmentLoopIterator
static
bool LoopTest(const TestCase& testCase,
              double minTime,
              double maxTime,
              Dir dir)
{
    const GfInterval iterInterval(minTime, maxTime,
                                  CLOSED, OPEN);

    // The domain is limited to the inner looped interval.
    const TsLoopParams lp = testCase.spline.GetInnerLoopParams();
    GfInterval domainInterval = lp.GetLoopedInterval();

    // GetLoopedInterval() returns a closed interval, but we
    // need it to be open.
    domainInterval.SetMax(domainInterval.GetMax(), OPEN);

    return DoOneTest<Ts_SegmentLoopIterator>(
        testCase, iterInterval, domainInterval, dir);
};

// Test Ts_SegmentKnotIterator
static
bool KnotTest(const TestCase& testCase,
              double minTime,
              double maxTime,
              Dir dir)
{
    const GfInterval iterInterval(minTime, maxTime,
                                  CLOSED, OPEN);

    // The domain is the region defined by knots. I.e., without any
    // extrapolation. Looping may generate knots before and/or after the
    // explicit knots. Union the looped and explicit intervals.
    //
    // Note that both GetLoopedInterval() and GetTimeSpan() return closed
    // intervals, but we need them to be open.
    const TsLoopParams lp = testCase.spline.GetInnerLoopParams();
    GfInterval domainInterval = lp.GetLoopedInterval();
    domainInterval.SetMax(domainInterval.GetMax(), OPEN);

    TsKnotMap knots = testCase.spline.GetKnots();
    GfInterval knotInterval = knots.GetTimeSpan();
    knotInterval.SetMax(knotInterval.GetMax(), OPEN);

    // Union the intervals to expand them as needed.
    domainInterval |= knotInterval;

    return DoOneTest<Ts_SegmentKnotIterator>(
        testCase, iterInterval, domainInterval, dir);
};

// Test Ts_SegmentIterator
static
bool FullTest(const TestCase& testCase,
              double minTime,
              double maxTime,
              Dir dir)
{
    const GfInterval iterInterval(minTime, maxTime,
                                  CLOSED, OPEN);

    // The domain for a full test is all time.
    GfInterval domainInterval(-inf, inf);
    return DoOneTest<Ts_SegmentIterator>(
        testCase, iterInterval, domainInterval, dir);
};


static
bool TestIterators()
{
    // Run all the tests!
    bool result =
        ProtoTest(oneKnot, -5, 5, Fwd) &&
        ProtoTest(simpleInnerLoop, 137, 155, Fwd) &&
        ProtoTest(simpleInnerLoop, 137, 155, Rev) &&
        ProtoTest(simpleInnerLoop, 0, 999, Fwd) &&
        ProtoTest(simpleInnerLoop, 0, 999, Rev) &&
        ProtoTest(simpleInnerLoop, 137, 145.00001, Fwd) &&
        ProtoTest(simpleInnerLoop, 137, 145.00001, Rev) &&
        ProtoTest(simpleInnerLoop, 137, 145, Fwd) &&
        ProtoTest(simpleInnerLoop, 137, 145, Rev) &&
        ProtoTest(simpleInnerLoop, 145, 146, Fwd) &&
        ProtoTest(simpleInnerLoop, 145, 146, Rev) &&
        ProtoTest(simpleInnerLoop, 147, 148, Fwd) &&
        ProtoTest(simpleInnerLoop, 147, 148, Rev) &&
        ProtoTest(twoKnotBezier, 0, 999, Fwd) &&
        ProtoTest(twoKnotBezier, 0, 999, Rev) &&

        LoopTest(simpleInnerLoop, 137, 155, Fwd) &&
        LoopTest(simpleInnerLoop, 137, 155, Rev) &&
        LoopTest(simpleInnerLoop, 0, 999, Fwd) &&
        LoopTest(simpleInnerLoop, 0, 999, Rev) &&
        LoopTest(simpleInnerLoop, 137, 145.00001, Fwd) &&
        LoopTest(simpleInnerLoop, 137, 145.00001, Rev) &&
        LoopTest(simpleInnerLoop, 119, 127.00001, Fwd) &&
        LoopTest(simpleInnerLoop, 119, 127.00001, Rev) &&
        LoopTest(simpleInnerLoop, 119, 127, Fwd) &&
        LoopTest(simpleInnerLoop, 119, 127, Rev) &&
        LoopTest(simpleInnerLoop, 163, 164, Fwd) &&
        LoopTest(simpleInnerLoop, 163, 164, Rev) &&
        LoopTest(simpleInnerLoop, 165, 166, Fwd) &&
        LoopTest(simpleInnerLoop, 165, 166, Rev) &&
        LoopTest(twoKnotBezier, 0, 999, Fwd) &&
        LoopTest(twoKnotBezier, 0, 999, Rev) &&

        KnotTest(simpleInnerLoop, 137, 155, Fwd) &&
        KnotTest(simpleInnerLoop, 137, 155, Rev) &&
        KnotTest(simpleInnerLoop, 0, 999, Fwd) &&
        KnotTest(simpleInnerLoop, 0, 999, Rev) &&
        KnotTest(simpleInnerLoop, 137, 145.00001, Fwd) &&
        KnotTest(simpleInnerLoop, 137, 145.00001, Rev) &&
        KnotTest(simpleInnerLoop, 119, 127.00001, Fwd) &&
        KnotTest(simpleInnerLoop, 119, 127.00001, Rev) &&
        KnotTest(simpleInnerLoop, 119, 127, Fwd) &&
        KnotTest(simpleInnerLoop, 119, 127, Rev) &&
        KnotTest(simpleInnerLoop, 119, 137, Fwd) &&
        KnotTest(simpleInnerLoop, 119, 137, Rev) &&
        KnotTest(simpleInnerLoop, 112, 181, Fwd) &&
        KnotTest(simpleInnerLoop, 112, 181, Rev) &&
        KnotTest(simpleInnerLoop, 163, 164, Fwd) &&
        KnotTest(simpleInnerLoop, 163, 164, Rev) &&
        KnotTest(simpleInnerLoop, 165, 166, Fwd) &&
        KnotTest(simpleInnerLoop, 165, 166, Rev) &&
        KnotTest(twoKnotBezier, 0, 999, Fwd) &&
        KnotTest(twoKnotBezier, 0, 999, Rev) &&
        KnotTest(simpleSplineDualValued, -5, 10, Fwd) &&
        KnotTest(simpleSplineDualValued, -5, 10, Rev) &&

        FullTest(oneKnot, -5, 5, Fwd) &&
        FullTest(oneKnotExtrapLoop, -5, 5, Fwd) &&
        FullTest(simpleInnerLoop, 137, 155, Fwd) &&
        FullTest(simpleInnerLoop, 0, 999, Fwd) &&
        FullTest(simpleInnerLoopDualValued, 0, 999, Fwd) &&
        FullTest(simpleSpline, -5, 10, Fwd) &&
        FullTest(longLoop, -5, 10, Fwd) &&
        FullTest(longLoop, 0, 999, Fwd) &&
        FullTest(extrapValueBlock, -10, 10, Fwd) &&
        FullTest(extrapHeld, -10, 10, Fwd) &&
        FullTest(extrapLinear, -10, 10, Fwd) &&
        FullTest(extrapSloped, -10, 10, Fwd) &&
        FullTest(extrapRepeat, -10, 10, Fwd) &&
        FullTest(extrapRepeat, 0, 999, Fwd) &&
        FullTest(extrapOscillate, -10, 10, Fwd) &&
        FullTest(extrapOscillate, 0, 999, Fwd) &&
        FullTest(twoKnotBezier, 0, 999, Fwd) &&
        FullTest(extrapResetDualValued, -10, 10, Fwd) &&
        FullTest(extrapResetLoopBoundary, -10, 10, Fwd) &&
        FullTest(extrapResetInvalidLoopBoundary, -10, 10, Fwd) &&
        FullTest(extrapRepeatDualValuedLoopBoundary, -10, 10, Fwd) &&
        FullTest(extrapOscillateLoopBoundary, -10, 10, Fwd) &&
        FullTest(extrapOscillateLoopBoundary, 0, 999, Fwd) &&

        true;

    return result;
}

int main(int argc, char* argv[])
{
    verbose = false;
    testSplineName = "";

    InitTestCases();

    bool error = false;
    bool help = false;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0) {
            help = true;
            continue;
        }

        if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
            continue;
        }

        if (argv[i][0] == '-') {
            error = 1;
            continue;
        }

        testSplineName = argv[i];
    }

    if (!testSplineName.empty() && testSplineNames.count(testSplineName) == 0) {
        std::cerr << "Unrecognized spline name: "
                  << testSplineName
                  << std::endl;
        error = 1;
    }

    if (help || error) {
        std::cerr << "Usage: "
                  << argv[0]
                  << " [-h] [-v] [splineName]\n\n";
        std::cerr << "    -v  Be verbose, output iterated segments.\n\n"
                  << "If the name of a spline is given, tests are limited\n"
                  << "to that spline. Legal values for splineName are:\n";
        for (const auto& name : testSplineNames) {
            std::cerr << "    " << name << "\n";
        }

        return (error ? 1 : 0);
    }

    if (!TestIterators()) {
        return 1;
    }

    return 0;
}
