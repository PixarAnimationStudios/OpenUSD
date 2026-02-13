//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

// Test the TsSpline::Diff methods

#include "pxr/pxr.h"
#include "pxr/base/ts/spline.h"

#include <iostream>
#include <optional>
#include <set>

PXR_NAMESPACE_USING_DIRECTIVE

// Get a convenient infinite value.
constexpr double inf = std::numeric_limits<double>::infinity();

// Constants for constructing GfInterval objects
constexpr bool OPEN = false;
constexpr bool CLOSED = true;

// Common test cases
static TsSpline twoKnotBezier;
static TsSpline simpleSpline;
static TsSpline palindrome;
static TsSpline longLoop;
static TsSpline extrapLinearSimple;
static TsSpline extrapLinearLooped;
static TsSpline extrapResetSimple;

////////////////////////////////////////////////////////////////
// Helper methods

// Test a condition and return false from the routine that uses it.
#define TEST_EMPTY(interval)                            \
    if (!((interval).IsEmpty())) {                      \
        TF_RUNTIME_ERROR(                               \
            #interval " -> %s, expected empty.",        \
            TfStringify(interval).c_str());             \
        return false;                                   \
    }

#define TEST_EQUAL(interval, expected)          \
    if ((interval) != (expected)) {             \
        TF_RUNTIME_ERROR(                       \
            #interval " -> %s, expected %s.",   \
            TfStringify(interval).c_str(),      \
            TfStringify(expected).c_str());     \
        return false;                           \
    }

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

// Quickly create a float-valued knot. Constructing a populated TsKnot
// requires multiple lines of C++ code. This is a quick one-liner.
static
TsKnot Kf(float time, float value,
         float preWidth, float preSlope,
         float postWidth, float postSlope,
         TsInterpMode interp)
{
    TsKnot knot(TfType::Find<float>());
    knot.SetTime(time);
    knot.SetValue(value);
    knot.SetPreTanWidth(preWidth);
    knot.SetPreTanSlope(preSlope);
    knot.SetPostTanWidth(postWidth);
    knot.SetPostTanSlope(postSlope);
    knot.SetNextInterpolation(interp);
    return knot;
};

static
void InitTestCases()
{
    // ================ TwoKnotBezier ================
    // Test a spline without any inner looping. This is a clone of
    // "TwoKnotBezier" from the museum
    twoKnotBezier.SetKnots(
        {  // t    v    pre-tan   post-tan  interp
            K(1.0, 1.0, 0.0, 0.0, 0.5, 1.0, TsInterpCurve),
            K(5.0, 2.0, 0.5, 0.0, 0.0, 0.0, TsInterpCurve)
        });

    // ================ simpleSpline ================
    // Living up to its name, all the knots have integral values.
    simpleSpline.SetKnots(
        {  // t    v    pre-tan   post-tan  interp
            K(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, TsInterpHeld),
            K(1.0, 1.0, 0.0, 0.0, 0.0, 0.0, TsInterpLinear),
            K(2.0, 3.0, 0.0, 0.0, 0.0, 0.0, TsInterpLinear),
            K(3.0, 2.0, 0.0, 0.0, 0.0, 0.0, TsInterpLinear),
            K(4.0, 4.0, 0.0, 0.0, 0.0, 0.0, TsInterpHeld)
        });

    // ================ palindrome ================
    // The curve is the same forwards and backwards.
    palindrome.SetKnots(
        {  // t    v    pre-tan   post-tan  interp
            K(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, TsInterpLinear),
            K(1.0, 1.0, 0.0, 0.0, 0.0, 0.0, TsInterpLinear),
            K(2.0, 3.0, 0.0, 0.0, 0.0, 0.0, TsInterpLinear),
            K(3.0, 1.0, 0.0, 0.0, 0.0, 0.0, TsInterpLinear),
            K(4.0, 0.0, 0.0, 0.0, 0.0, 0.0, TsInterpLinear)
        });
    palindrome.SetPreExtrapolation(TsExtrapLoopReset);
    palindrome.SetPostExtrapolation(TsExtrapLoopReset);

    // ================ LongLoop ================
    // LongLoop has inner looping that extends before and after the first and
    // last knot. The first and last knot are also shadowed by the looping.
    longLoop = simpleSpline;
    longLoop.SetInnerLoopParams(TsLoopParams{1.0, 3.0, 2, 2, 2.0});

    // ================ ExtrapLinearSimple ================
    // Clone simpleSpline but add linear extrapolation
    extrapLinearSimple = simpleSpline;
    extrapLinearSimple.SetPreExtrapolation(TsExtrapLinear);
    extrapLinearSimple.SetPostExtrapolation(TsExtrapLinear);

    // ================ ExtrapLinearLooped ================
    // Clone longLoop but add linear extrapolation
    extrapLinearLooped = longLoop;
    extrapLinearLooped.SetPreExtrapolation(TsExtrapLinear);
    extrapLinearLooped.SetPostExtrapolation(TsExtrapLinear);

    // ================ ExtrapResetSimple ================
    // Clone simpleSpline but add extrapolation reset looping.
    extrapResetSimple = simpleSpline;
    extrapResetSimple.SetPreExtrapolation(TsExtrapLoopReset);
    extrapResetSimple.SetPostExtrapolation(TsExtrapLoopReset);
}

static
bool TestKnotDiffs()
{
    TsSpline copy = simpleSpline;
    TsKnot knot;

    // Identical splines should return an empty interval.
    TEST_EMPTY(simpleSpline.Diff(copy));
    TEST_EMPTY(copy.Diff(simpleSpline));

    // Get a knot
    copy.GetKnot(2.0, &knot);

    // Change one segment.
    knot.SetNextInterpolation(TsInterpHeld);
    copy.SetKnot(knot);
    GfInterval expected1(2.0, 3.0, CLOSED, OPEN);
    TEST_EQUAL(simpleSpline.Diff(copy), expected1);
    TEST_EQUAL(copy.Diff(simpleSpline), expected1);

    // Change two segments
    double value;
    knot.GetValue(&value);
    knot.SetValue(value + 1);
    copy.SetKnot(knot);
    GfInterval expected2(1.0, 3.0, CLOSED, OPEN);
    TEST_EQUAL(simpleSpline.Diff(copy), expected2);
    TEST_EQUAL(copy.Diff(simpleSpline), expected2);

    // Compare only part of the change
    GfInterval compareInterval(2.5, +inf);
    GfInterval expected3(2.5, 3.0, CLOSED, OPEN);
    TEST_EQUAL(simpleSpline.Diff(copy, compareInterval), expected3);
    TEST_EQUAL(copy.Diff(simpleSpline, compareInterval), expected3);

    // Change a tangent, should change the whole segment.
    copy = twoKnotBezier;
    copy.GetKnot(1.0, &knot);
    knot.SetPostTanWidth(knot.GetPreTanWidth() * 2);
    copy.SetKnot(knot);
    GfInterval expected4(1.0, 5.0, CLOSED, OPEN);
    TEST_EQUAL(twoKnotBezier.Diff(copy), expected4);
    TEST_EQUAL(copy.Diff(twoKnotBezier), expected4);

    return true;
}

static
bool TestLoopedDiffs()
{
    TsSpline copy = longLoop;
    TsKnot knot;

    // Identical splines should return an empty interval.
    TEST_EMPTY(longLoop.Diff(copy));
    TEST_EMPTY(copy.Diff(longLoop));

    // Get the loop parameters.
    TsLoopParams lp = copy.GetInnerLoopParams();

    // The loop prototype interval is [1..3). Change the knot at 2.
    copy.GetKnot(2.0, &knot);
    knot.SetValue(2.0);
    copy.SetKnot(knot);
    // That should have changed the entire looped interval. Note that
    // the interval returned by TsLoopParams::GetLoopedInterval() is
    // closed on the right, but it should probably be open.
    GfInterval expected1 = lp.GetLoopedInterval();
    expected1.SetMax(expected1.GetMax(), OPEN);
    TEST_EQUAL(longLoop.Diff(copy), expected1);
    TEST_EQUAL(copy.Diff(longLoop), expected1);

    // Reset copy and change the first knot. Since the first knot is
    // duplicated at the end of each loop, changing the first knot
    // should change everything, including the extrapolation regions.
    copy = longLoop;
    copy.GetKnot(1.0, &knot);
    knot.SetValue(2.0);
    copy.SetKnot(knot);

    GfInterval expected2 = GfInterval::GetFullInterval();
    TEST_EQUAL(longLoop.Diff(copy), expected2);
    TEST_EQUAL(copy.Diff(longLoop), expected2);

    // Reset copy and change the loop params instead of the knots.
    copy = longLoop;
    lp.numPostLoops += 1;
    copy.SetInnerLoopParams(lp);
    // This should change from the end of the old looped interval to +inf.
    // Note: If you visualize the modified spline, only the interval from
    // [7..9) is actually different, but the post-extrap segment in longLoop
    // covers [7..+inf) but in copy it covers [9..+inf), so it is different.
    GfInterval expected3(7, +inf, CLOSED, OPEN);
    TEST_EQUAL(longLoop.Diff(copy), expected3);
    TEST_EQUAL(copy.Diff(longLoop), expected3);

    return true;
}

static
bool TestExtrapDiffs()
{
    TsSpline copy = extrapLinearLooped;
    TsKnot knot;

    // Change the pre-extrapolation, so everything before time -3
    copy.SetPreExtrapolation(TsExtrapValueBlock);
    GfInterval expected1(-inf, -3, OPEN, OPEN);
    TEST_EQUAL(extrapLinearLooped.Diff(copy), expected1);
    TEST_EQUAL(copy.Diff(extrapLinearLooped), expected1);

    // Reset copy and change the knot at time 2. This will change the
    // slope of the segment from [1..2) and [2..3) which will change
    // the slope of the linear extrapolation.
    copy = extrapLinearLooped;
    copy.GetKnot(2.0, &knot);
    knot.SetValue(0.0);
    copy.SetKnot(knot);

    // Note that infinite intervals are always open ended.
    GfInterval expected2(-inf, +inf, OPEN, OPEN);
    TEST_EQUAL(extrapLinearLooped.Diff(copy), expected2);
    TEST_EQUAL(copy.Diff(extrapLinearLooped), expected2);

    // Repeat the above test with a non-looped spline. Changing the knot at 2
    // should only change [1..3) (but see extrapResetSimple below).
    copy = extrapLinearSimple;
    copy.GetKnot(2.0, &knot);
    knot.SetValue(0.0);
    copy.SetKnot(knot);
    
    GfInterval expected3(1, 3, CLOSED, OPEN);
    TEST_EQUAL(extrapLinearSimple.Diff(copy), expected3);
    TEST_EQUAL(copy.Diff(extrapLinearSimple), expected3);

    // Repeat the above test with a looping extrapolation. Changing the knot at
    // 2 should change ..., [-7..-5), [-3..-1), [1..3), [5..7), [9..11), ...
    copy = extrapResetSimple;
    copy.GetKnot(2.0, &knot);
    knot.SetValue(0.0);
    copy.SetKnot(knot);

    // An infinite query should return an infinite result
    GfInterval expected4(-inf, +inf, OPEN, OPEN);
    TEST_EQUAL(extrapResetSimple.Diff(copy), expected4);
    TEST_EQUAL(copy.Diff(extrapResetSimple), expected4);

    // But finite queries should return finite results.
    TEST_EQUAL(extrapResetSimple.Diff(copy, GfInterval(0, 4)),
               GfInterval(1, 3, CLOSED, OPEN));
    TEST_EQUAL(extrapResetSimple.Diff(copy, GfInterval(0, 10)),
               GfInterval(1, 10, CLOSED, CLOSED));
    TEST_EQUAL(extrapResetSimple.Diff(copy, GfInterval(10, 12)),
               GfInterval(10, 11, CLOSED, OPEN));
    TEST_EQUAL(extrapResetSimple.Diff(copy, GfInterval(-5, 0)),
               GfInterval(-3, -1, CLOSED, OPEN));

    // Change the pre-extrapolation for copy which should then differ in
    // (-inf..0), [1..3), [5..7), [9..11), ...
    copy.SetPreExtrapolation(TsExtrapLoopRepeat);

    // Again, an infinite query should return an infinite result. (Reuse
    // expected4 from above.)
    TEST_EQUAL(extrapResetSimple.Diff(copy), expected4);
    TEST_EQUAL(copy.Diff(extrapResetSimple), expected4);

    // But finite queries should return finite results.
    TEST_EQUAL(extrapResetSimple.Diff(copy, GfInterval(0, 4)),
               GfInterval(1, 3, CLOSED, OPEN));
    TEST_EQUAL(extrapResetSimple.Diff(copy, GfInterval(0, 10)),
               GfInterval(1, 10, CLOSED, CLOSED));
    TEST_EQUAL(extrapResetSimple.Diff(copy, GfInterval(10, 12)),
               GfInterval(10, 11, CLOSED, OPEN));
    TEST_EQUAL(extrapResetSimple.Diff(copy, GfInterval(-5, 0)),
               GfInterval(-5, 0, CLOSED, OPEN));

    // Start over and test some semi-infinite queries
    copy = extrapResetSimple;
    copy.SetPreExtrapolation(TsExtrapLoopRepeat);

    TEST_EQUAL(extrapResetSimple.Diff(copy), GfInterval(-inf, 0, OPEN, OPEN));
    TEST_EQUAL(extrapResetSimple.Diff(copy, GfInterval(-inf, 2)),
               GfInterval(-inf, 0, OPEN, OPEN));
    TEST_EMPTY(extrapResetSimple.Diff(copy, GfInterval(2, +inf)));

    copy = extrapResetSimple;
    copy.SetPostExtrapolation(TsExtrapLoopRepeat);

    TEST_EQUAL(extrapResetSimple.Diff(copy), GfInterval(4, +inf, CLOSED, OPEN));
    TEST_EMPTY(extrapResetSimple.Diff(copy, GfInterval(-inf, 2)));
    TEST_EQUAL(extrapResetSimple.Diff(copy, GfInterval(2, +inf)),
               GfInterval(4, +inf, CLOSED, OPEN));

    // Verify that splines with different looping modes but the same set
    // of segments compare equal.
    copy = palindrome;
    copy.SetPreExtrapolation(TsExtrapLoopRepeat);
    copy.SetPostExtrapolation(TsExtrapLoopOscillate);

    TEST_EMPTY(palindrome.Diff(copy));
    TEST_EMPTY(copy.Diff(palindrome));

    return true;
}

static
bool TestEdgeCases()
{
    // Test completely empty splines (not even a Ts_SplineData pointer)
    TsSpline spline;
    TsSpline copy;

    TEST_EMPTY(spline.Diff(copy));
    TEST_EMPTY(copy.Diff(spline));

    // Compare empty and value blocked splines
    //             t    v    pre-tan   post-tan  interp
    copy.SetKnot(K(0.0, 0.0, 0.0, 0.0, 0.0, 0.0, TsInterpValueBlock));

    // They differ at all time because copy has 2 segments (-inf .. 0) and
    // [0 .. +inf).
    GfInterval expected1 = GfInterval::GetFullInterval();

    TEST_EQUAL(spline.Diff(copy), expected1);
    TEST_EQUAL(copy.Diff(spline), expected1);

    // Empty double and float splines should compare equal
    copy = TsSpline(TfType::Find<float>());

    TEST_EMPTY(spline.Diff(copy));
    TEST_EMPTY(copy.Diff(spline));

    // Looping and extrapolation parameters still leave it empty
    copy.SetInnerLoopParams(TsLoopParams{1.0, 2.0, 3, 4, 5.0});
    copy.SetPreExtrapolation(TsExtrapLoopRepeat);
    copy.SetPostExtrapolation(TsExtrapLoopReset);

    TEST_EMPTY(spline.Diff(copy));
    TEST_EMPTY(copy.Diff(spline));

    // Double and float splines with the same knots should also compare equal
    // because the spline segments will be equal.
    spline = TsSpline(TfType::Find<double>());
    copy = TsSpline(TfType::Find<float>());

    //               t    v    pre-tan   post-tan  interp
    spline.SetKnot(K(0.0, 0.0, 0.0, 0.0, 1.0, 0.0, TsInterpCurve));
    spline.SetKnot(K(3.0, 1.0, 1.0, 0.0, 0.0, 0.0, TsInterpCurve));
    
    //              t    v    pre-tan   post-tan  interp
    copy.SetKnot(Kf(0.0, 0.0, 0.0, 0.0, 1.0, 0.0, TsInterpCurve));
    copy.SetKnot(Kf(3.0, 1.0, 1.0, 0.0, 0.0, 0.0, TsInterpCurve));

    TEST_EMPTY(spline.Diff(copy));
    TEST_EMPTY(copy.Diff(spline));

    // But change the curve type and they should be different.
    spline = twoKnotBezier;
    copy = spline;
    copy.SetCurveType(TsCurveTypeHermite);

    GfInterval expected2(1.0, 5.0, CLOSED, OPEN);
    TEST_EQUAL(spline.Diff(copy), expected2);
    TEST_EQUAL(copy.Diff(spline), expected2);

    // Test tangent changes on linear segments
    spline = simpleSpline;
    copy = spline;
    TsKnot knot;
    copy.GetKnot(2.0, &knot);
    knot.SetPreTanWidth(1.0);
    copy.SetKnot(knot);

    GfInterval expected3(1.0, 2.0, CLOSED, OPEN);
    TEST_EQUAL(spline.Diff(copy), expected3);
    TEST_EQUAL(copy.Diff(spline), expected3);

    // Test changing the tangent width on a hermite curve. The tangent algorithm
    // should change it back.
    spline = twoKnotBezier;
    spline.SetCurveType(TsCurveTypeHermite);
    copy = spline;
    copy.GetKnot(1.0, &knot);
    knot.SetPostTanWidth(knot.GetPostTanWidth() + 1);
    copy.SetKnot(knot);

    TEST_EMPTY(spline.Diff(copy));
    TEST_EMPTY(copy.Diff(spline));

    return true;
}

int main(int argc, char* argv[])
{
    InitTestCases();
    if (!TestKnotDiffs()) {
        std::cout << "TestKnotDiffs:        FAILED!" << std::endl;
        return 1;
    }
    std::cout << "TestKnotDiffs:        Passed" << std::endl;

    if (!TestLoopedDiffs()) {
        std::cout << "TestLoopedDiffs:      FAILED!" << std::endl;
        return 1;
    }
    std::cout << "TestLoopedDiffs:      Passed" << std::endl;

    if (!TestExtrapDiffs()) {
        std::cout << "TestExtrapDiffs:      FAILED!" << std::endl;
        return 1;
    }
    std::cout << "TestExtrapDiffs:      Passed" << std::endl;

    if (!TestEdgeCases()) {
        std::cout << "TestEdgeCases:        FAILED!" << std::endl;
        return 1;
    }
    std::cout << "TestEdgeCases:        Passed" << std::endl;

    return 0;
}
