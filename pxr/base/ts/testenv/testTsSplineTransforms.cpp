//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/ts/knot.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/tsTest_Museum.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/stringUtils.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

// Get a convenient infinite value.
constexpr double inf = std::numeric_limits<double>::infinity();

// Constants for constructing GfInterval objects
constexpr bool OPEN = false;
constexpr bool CLOSED = true;

// Note: result, testId, and splineId are expected to be defined local
// variables. Must be used inside a loop where "continue" is valid.
#define TEST_ASSERT(condition, ...)             \
if (!(condition)) {                             \
    std::cerr << testId << ": "                 \
              << splineId << ": "               \
              << TfStringPrintf(__VA_ARGS__)    \
              << std::endl;                     \
    result = false;                             \
    continue;                                   \
}

// Verify that the splines evaluate equivalently across interval.
// This should be kept semantically in sync with testTsSplineBaking.cpp's
// VerifySplineEquivalence.
bool VerifySplineEquivalence(const std::string& testId,
                             const std::string& splineId,
                             const TsSpline& spline1,
                             const TsSpline& spline2,
                             const GfInterval& truncInterval,
                             const GfInterval& checkInterval,
                             double epsilon)
{
    // This variable is updated by TEST_ASSERT
    bool result = true;
    
    // Evaluate both splines across interval and ensure that the
    // results are close if not identical.
    double maxError = -1;
    double maxErrorTime = checkInterval.GetMin() - 1.0;

    // Evaluate at 101 points across the time interval.
    for (int i = 0; i < 101; ++i) {
        double t = GfLerp(i/100.0, checkInterval.GetMin(),
                          checkInterval.GetMax());
        bool valid1, valid2;
        double v1, v2;

        if (i == 100 && checkInterval.IsMaxOpen()) {
            valid1 = spline1.EvalPreValue(t, &v1);
            valid2 = spline2.EvalPreValue(t, &v2);
        } else {
            valid1 = spline1.Eval(t, &v1);
            valid2 = spline2.Eval(t, &v2);
        }

        if (!valid1 && !valid2) {
            // No value from both splines.
            continue;
        }

        TEST_ASSERT(valid1 && valid2,
                    "Value-block mismatch or failure at time=%g"
                    " for truncInterval=%s, checkInterval=%s\n"
                    "spline1=%s and spline2=%s",
                    t, TfStringify(truncInterval).c_str(),
                    TfStringify(checkInterval).c_str(),
                    (valid1
                        ? TfStringify(v1).c_str()
                        : "n/a"),
                    (valid2
                        ? TfStringify(v2).c_str()
                        : "n/a"));

        const double error = std::abs(v1 - v2);
        if (error > maxError) {
            maxError = error;
            maxErrorTime = t;
        }
        
        TEST_ASSERT(GfIsClose(v1, v2, epsilon),
                    "Values are not close at time=%g"
                    " for truncInterval=%s, checkInterval=%s\n"
                    "spline1=%.12g and spline2=%.12g",
                    t, TfStringify(truncInterval).c_str(),
                    TfStringify(checkInterval).c_str(),
                    v1, v2);
    }

    // Complete lack of values causes maxError to never change
    // from its original value of -1.
    if (result) {
        maxError = 0;
    }

    std::cout << testId << ": "
                << splineId << ": "
                << "maxError = " << maxError
                << " at time = " << maxErrorTime
                << " for truncInterval " << truncInterval
                << ", checkInterval " << checkInterval
                << std::endl;

    if (!result) {
        std::cerr << "Failing splines:\n"
                    << "spline1:\n" << spline1 << "\n"
                    << "spline2:\n" << spline2 << std::endl;
    }
        
    return result;
}

bool
TestTruncateBaseCases()
{
    // Initialize knots to contain one non-dual valued knot, one dual valued
    // knot.
    std::vector<TsKnot> knots;
    TsKnot knot(Ts_GetType<double>());
    knot.SetTime(3);
    knot.SetValue(5.0);
    knots.push_back(knot);
    knot.SetPreValue(2.0);
    knots.push_back(knot);

    // Initialize extrapolations
    std::vector<std::pair<TsExtrapolation, std::string>> extraps = {
        {TsExtrapolation(TsExtrapValueBlock), "ValueBlock"},
        {TsExtrapolation(TsExtrapHeld), "Held"},
        {TsExtrapolation(TsExtrapLinear), "Linear"},
        {TsExtrapolation(TsExtrapLoopRepeat), "LoopRepeat"},
        {TsExtrapolation(TsExtrapLoopReset), "LoopReset"},
        {TsExtrapolation(TsExtrapLoopOscillate), "LoopOscillate"},
    };
    TsExtrapolation sloped(TsExtrapSloped);
    sloped.slope = 5.0;
    extraps.push_back({sloped, "Sloped"});

    // Initialize one-knot splines from the matrix of knots x extraps.
    std::vector<std::pair<TsSpline, std::string>> splines;
    for (const std::pair<TsExtrapolation, std::string>& extrap: extraps) {
        for (const TsKnot& knot : knots) {
            TsSpline spline(Ts_GetType<double>());
            spline.SetKnot(knot);
            spline.SetPreExtrapolation(extrap.first);
            spline.SetPostExtrapolation(extrap.first);

            std::string splineId =
                "BaseCaseSpline_Extrap" + extrap.second
                + (knot.IsDualValued() ? "_OneKnot" : "_OneKnotDV");
            splines.push_back({spline, splineId});
        }
    }

    // Empty spline case
    splines.push_back({TsSpline(), "BaseCaseSpline_Empty"});

    // Run tests
    bool result = true;
    const std::string testId = "TestTruncateBaseCases";
    const std::vector<GfInterval> intervals = {
        {-inf, +inf, OPEN, OPEN},
        {1.0, +inf, CLOSED, OPEN},
        {-inf, 5.0, OPEN, OPEN},
        {1.0, 5.0, CLOSED, OPEN},
        {3.0, 5.0, CLOSED, OPEN},
        {4.0, 5.0, CLOSED, OPEN},
        {1.0, 3.0, CLOSED, OPEN},
        {1.0, 2.0, CLOSED, OPEN},
    };
    for (const auto& splineInfo : splines) {
        const TsSpline& spline = splineInfo.first;
        const std::string splineId = splineInfo.second;
        for (const GfInterval& truncInterval : intervals) {
            const TsSpline truncated = spline.GetTruncated(truncInterval);

            GfInterval checkInterval = truncInterval;
            if (!checkInterval.IsMinFinite()) {
                checkInterval.SetMin(
                    std::min(checkInterval.GetMax() - 100, -100.0),
                    CLOSED);
            }
            if (!checkInterval.IsMaxFinite()) {
                checkInterval.SetMax(checkInterval.GetMin() + 200, OPEN);
            }
            if (!VerifySplineEquivalence(testId, splineId,
                                         spline, truncated,
                                         truncInterval,
                                         checkInterval, 1e-10))
            {
                result = false;
                continue;
            }
        }
    }

    return result;
}

bool
TestTruncateMuseumCases()
{
    bool result = true;
    const std::string testId = "TestTruncateMuseumCases";

    // Multiply min, max against knot spans of each spline to determine
    // the interval to truncate to.
    const std::vector<GfInterval> scales = {
        {0, 1, CLOSED, OPEN},
        {0.1, 0.9, CLOSED, OPEN},
        {0.6, 1.5, CLOSED, OPEN},
        {-0.5, 0.4, CLOSED, OPEN},
        {-1, 2, CLOSED, OPEN},
        {-3.4, -0.1, CLOSED, OPEN},
        {10.2, 12.2, CLOSED, OPEN},
        {-inf, 0.8, OPEN, OPEN},
        {3.5, +inf, CLOSED, OPEN},
        {-inf, +inf, OPEN, OPEN}
    };

    // These splines are subject to _BreakdownBezier's de-regression
    // round-trip due to regressive/near-vertical tangent drift
    const std::vector<std::string> regressiveNames = {
        "CenterVertical",
        "Cusp",
        "FourThirdOneThird",
        "FringeVert",
        "OneThirdFourThird",
        "RegressiveLoop",
        "RegressivePostC",
        "RegressivePostFringe",
        "RegressivePostG",
        "RegressivePostJ",
        "RegressivePreC",
        "RegressivePreFringe",
        "RegressivePreG",
        "RegressivePreJ",
        "RegressiveS",
        "RegressiveSBothOut",
        "RegressiveSPostOut",
        "RegressiveSPreOut",
        "RegressiveSStandard",
        "VerticalTorture",
    };

    for (const std::string& name : TsTest_Museum::GetAllNames()) {
        const std::string splineId = name;

        const TsSpline spline =
            TsTest_Museum::GetSplineByName(name, Ts_GetType<double>());
        const TsKnotMap knots = spline.GetKnots();

        if (knots.size() < 2) {
            // We'll check degenerate cases separately
            continue;
        }

        const bool isMaybeRegressive = 
            std::find(regressiveNames.begin(), regressiveNames.end(),
                      name) != regressiveNames.end();
        const double epsilon = isMaybeRegressive ? 0.01 : 1.0e-10;

        const GfInterval knotSpan = knots.GetTimeSpan();
        const double spanSize = knotSpan.GetSize();

        for (const GfInterval& scale : scales) {
            const double min =
                !scale.IsMinFinite()
                ? -inf
                : knotSpan.GetMin() + scale.GetMin() * spanSize;
            const double max =
                !scale.IsMaxFinite()
                ? +inf
                : knotSpan.GetMax() + (scale.GetMax()-1) * spanSize;

            const GfInterval truncInterval(
                min, max, scale.IsMinClosed(), scale.IsMaxClosed());

            const TsSpline truncated = spline.GetTruncated(truncInterval);
            const TsKnotMap truncKnots = truncated.GetKnots();

            TEST_ASSERT(!truncKnots.empty(),
                        "Truncated spline has no knots");

            GfInterval checkInterval = truncInterval;
            if (!checkInterval.IsMinFinite()) {
                checkInterval.SetMin(
                    std::min(checkInterval.GetMax() - 100, -100.0),
                    CLOSED);
            }
            if (!checkInterval.IsMaxFinite()) {
                checkInterval.SetMax(checkInterval.GetMin() + 200, OPEN);
            }
            if (!VerifySplineEquivalence(testId, splineId,
                                         spline, truncated,
                                         truncInterval,
                                         checkInterval, epsilon))
            {
                result = false;
                continue;
            }
        }

    }

    return result;
}

bool
TestTruncateSpecificCases()
{
    std::string testId = "TestTruncateSpecificCases";

    TsSpline splineOsc;
    splineOsc.SetPostExtrapolation(TsExtrapLoopOscillate);
    {
        TsKnot k1, k2;
        k1.SetTime(5);
        k1.SetValue(10.0);
        k1.SetNextInterpolation(TsInterpCurve);
        k2.SetTime(10);
        k2.SetValue(5.0);
        splineOsc.SetKnots({k1, k2});
    }
    const GfInterval oscInterval(10, 12.5, CLOSED, OPEN);
    const TsSpline truncOsc = splineOsc.GetTruncated(oscInterval);
    if (!VerifySplineEquivalence(testId, "splineOsc", splineOsc, truncOsc,
                                 oscInterval, oscInterval, 1e-10)) {
        return false;
    }

    TsSpline splineReset;
    splineReset.SetPreExtrapolation(TsExtrapLoopReset);
    splineReset.SetPostExtrapolation(TsExtrapLoopReset);
    {
        TsKnot k1, k2, k3;
        k1.SetTime(0);
        k1.SetValue(0.0);
        k1.SetPreTanWidth(2.0);
        k1.SetPostTanWidth(2.0);
        k1.SetNextInterpolation(TsInterpCurve);
        k2.SetTime(5);
        k2.SetValue(3.0);
        k2.SetPreTanWidth(2.0);
        k2.SetPostTanWidth(2.0);
        k2.SetNextInterpolation(TsInterpCurve);
        k3.SetTime(10);
        k3.SetValue(0.0);
        k3.SetPreTanWidth(2.0);
        k3.SetPostTanWidth(2.0);
        k3.SetNextInterpolation(TsInterpCurve);
        splineReset.SetKnots({k1, k2, k3});
    }
    const GfInterval resetInterval(-inf, 11, OPEN, OPEN);
    const TsSpline truncReset = splineReset.GetTruncated(resetInterval);
    const GfInterval checkInterval(-100, 11, OPEN, OPEN);
    if (!VerifySplineEquivalence(testId, "splineReset", splineReset,
                                 truncReset, resetInterval, checkInterval,
                                 1e-10)) {
        return false;
    }
    return true;
}

bool
TestTruncate()
{
    return TestTruncateBaseCases()
        && TestTruncateMuseumCases()
        && TestTruncateSpecificCases();
}

template <typename T>
TsSpline GetTestSpline() {
    const TfType valueType = Ts_GetType<T>();
    TsSpline spline(valueType);

    // Set extrapolations
    {
        TsExtrapolation preExtrap(TsExtrapSloped);
        preExtrap.slope = -0.5;
        spline.SetPreExtrapolation(preExtrap);

        TsExtrapolation postExtrap(TsExtrapLoopOscillate);
        postExtrap.loopBoundaryTime = 2.0;
        spline.SetPostExtrapolation(postExtrap);
    }

    // Set knots
    {
        TsKnot k1(valueType);
        TsKnot k2(valueType);
        TsKnot k3(valueType);

        k1.SetTime(0.0);
        k1.SetValue(T(2.0));
        k1.SetPostTanWidth(0.25);
        k1.SetPostTanSlope(T(0.5));
        k1.SetNextInterpolation(TsInterpCurve);

        k2.SetTime(2.0);
        k2.SetPreValue(T(3.0));
        k2.SetValue(T(4.0));
        k2.SetPreTanWidth(0.5);
        k2.SetPreTanSlope(T(-0.5));
        k2.SetNextInterpolation(TsInterpLinear);

        k3.SetTime(4.0);
        k3.SetValue(T(2.0));

        spline.SetKnots({k1, k2, k3});
    }

    return spline;
}

bool TestGetTimeScaled()
{
    const double scaleFactor = 3.0;
    const double scaleOffset = 5.0;

    const TsSpline dSpline = GetTestSpline<double>();
    const TsExtrapolation dPreExtrap = dSpline.GetPreExtrapolation();
    const TsExtrapolation dPostExtrap = dSpline.GetPostExtrapolation();

    // Check positive scale and offset of double spline
    const TsSpline dScaled = dSpline.GetTimeScaled(scaleFactor,
                                                   scaleOffset);
    {
        // Check scaled extrapolation
        const TsExtrapolation pre = dScaled.GetPreExtrapolation();
        const TsExtrapolation post = dScaled.GetPostExtrapolation();
        TF_AXIOM(pre.mode == dPreExtrap.mode);
        TF_AXIOM(pre.slope == dPreExtrap.slope / scaleFactor);
        TF_AXIOM(post.mode == dPostExtrap.mode);
        TF_AXIOM(post.loopBoundaryTime.value() ==
                 dPostExtrap.loopBoundaryTime.value()
                 * scaleFactor + scaleOffset);
    }
    {
        // Check knots
        TsKnotMap expectedKnots;
        for (const TsKnot& dKnot : dSpline.GetKnots()) {
            TsKnot expected = dKnot;
            expected.SetTime(dKnot.GetTime() * scaleFactor + scaleOffset);
            expected.SetPreTanWidth(dKnot.GetPreTanWidth() * scaleFactor);
            expected.SetPostTanWidth(dKnot.GetPostTanWidth() * scaleFactor);

            double value;
            TF_AXIOM(dKnot.GetPreTanSlope(&value));
            expected.SetPreTanSlope(value / scaleFactor);
            TF_AXIOM(dKnot.GetPostTanSlope(&value));
            expected.SetPostTanSlope(value / scaleFactor);

            expectedKnots.insert(expected);
        }
        TF_AXIOM(expectedKnots == dScaled.GetKnots());
    }

    // Check negative scale and offset of double spline
    const TsSpline dNegScaled = dSpline.GetTimeScaled(-scaleFactor,
                                                      scaleOffset);
    {
        // Check negative scaled extrapolation
        const TsExtrapolation pre = dNegScaled.GetPreExtrapolation();
        const TsExtrapolation post = dNegScaled.GetPostExtrapolation();
        TF_AXIOM(pre.mode == dPostExtrap.mode);
        TF_AXIOM(pre.loopBoundaryTime.value() ==
                 dPostExtrap.loopBoundaryTime.value()
                 * (-scaleFactor) + scaleOffset);
        TF_AXIOM(post.mode == dPreExtrap.mode);
        TF_AXIOM(post.slope == -dPreExtrap.slope / scaleFactor);
    }
    {
        // Build expected knots and check them
        TsKnot k1, k2, k3;

        k3.SetTime(4.0 * (-scaleFactor) + scaleOffset);
        k3.SetValue(2.0);
        k3.SetNextInterpolation(TsInterpLinear);

        k2.SetTime(2.0 * (-scaleFactor) + scaleOffset);
        k2.SetValue(3.0);
        k2.SetPreValue(4.0);
        k2.SetPostTanWidth(0.5 * scaleFactor);
        k2.SetPostTanSlope(0.5 / scaleFactor);
        k2.SetNextInterpolation(TsInterpCurve);

        k1.SetTime(scaleOffset);
        k1.SetValue(2.0);
        k1.SetPreTanWidth(0.25 * scaleFactor);
        k1.SetPreTanSlope(-0.5 / scaleFactor);

        TF_AXIOM(TsKnotMap({k3, k2, k1}) == dNegScaled.GetKnots());
    }

    const TsSpline tSpline = GetTestSpline<GfTimeCode>();
    const TsExtrapolation tPreExtrap = tSpline.GetPreExtrapolation();
    const TsExtrapolation tPostExtrap = tSpline.GetPostExtrapolation();

    // Check positive scale and offset of GfTimeCode spline
    const TsSpline tScaled = tSpline.GetTimeScaled(scaleFactor,
                                                   scaleOffset);
    {
        // Check scaled extrapolation
        const TsExtrapolation pre = tScaled.GetPreExtrapolation();
        const TsExtrapolation post = tScaled.GetPostExtrapolation();
        TF_AXIOM(pre == tPreExtrap);
        TF_AXIOM(post.mode == tPostExtrap.mode);
        TF_AXIOM(post.loopBoundaryTime.value() ==
                 tPostExtrap.loopBoundaryTime.value()
                 * scaleFactor + scaleOffset);
    }
    {
        // Check knots
        TsKnotMap expectedKnots;
        for (const TsKnot& tKnot : tSpline.GetKnots()) {
            TsKnot expected = tKnot;
            expected.SetTime(tKnot.GetTime() * scaleFactor + scaleOffset);
            expected.SetPreTanWidth(tKnot.GetPreTanWidth() * scaleFactor);
            expected.SetPostTanWidth(tKnot.GetPostTanWidth() * scaleFactor);

            GfTimeCode value;
            TF_AXIOM(tKnot.GetValue(&value));
            expected.SetValue(value * scaleFactor + scaleOffset);
            if (tKnot.IsDualValued()) {
                TF_AXIOM(tKnot.GetPreValue(&value));
                expected.SetPreValue(value * scaleFactor + scaleOffset);
            }

            expectedKnots.insert(expected);
        }
        TF_AXIOM(expectedKnots == tScaled.GetKnots());
    }

    // Check negative scale and offset of GfTimeCode spline
    const TsSpline tNegScaled = tSpline.GetTimeScaled(-scaleFactor,
                                                      scaleOffset);
    {
        // Check negative scaled extrapolation
        const TsExtrapolation pre = tNegScaled.GetPreExtrapolation();
        const TsExtrapolation post = tNegScaled.GetPostExtrapolation();
        TF_AXIOM(pre.mode == tPostExtrap.mode);
        TF_AXIOM(pre.loopBoundaryTime.value() ==
                 tPostExtrap.loopBoundaryTime.value()
                 * (-scaleFactor) + scaleOffset);
        TF_AXIOM(post == tPreExtrap);
    }
    {
        // Build expected knots and check them
        const TfType t = Ts_GetType<GfTimeCode>();
        TsKnot k1(t);
        TsKnot k2(t);
        TsKnot k3(t);

        k3.SetTime(4.0 * (-scaleFactor) + scaleOffset);
        k3.SetValue(GfTimeCode(2.0 * (-scaleFactor) + scaleOffset));
        k3.SetNextInterpolation(TsInterpLinear);

        k2.SetTime(2.0 * (-scaleFactor) + scaleOffset);
        k2.SetValue(GfTimeCode(3.0 * (-scaleFactor) + scaleOffset));
        k2.SetPreValue(GfTimeCode(4.0 * (-scaleFactor) + scaleOffset));
        k2.SetPostTanWidth(0.5 * scaleFactor);
        k2.SetPostTanSlope(GfTimeCode(-0.5));
        k2.SetNextInterpolation(TsInterpCurve);

        k1.SetTime(scaleOffset);
        k1.SetValue(GfTimeCode(2.0 * (-scaleFactor) + scaleOffset));
        k1.SetPreTanWidth(0.25 * scaleFactor);
        k1.SetPreTanSlope(GfTimeCode(+0.5));

        TF_AXIOM(TsKnotMap({k3, k2, k1}) == tNegScaled.GetKnots());
    }

    const TsSpline durSpline = GetTestSpline<GfDuration>();
    const TsExtrapolation durPreExtrap = durSpline.GetPreExtrapolation();
    const TsExtrapolation durPostExtrap = durSpline.GetPostExtrapolation();

    // Check positive scale and offset of GfDuration spline
    const TsSpline durScaled = durSpline.GetTimeScaled(scaleFactor,
                                                       scaleOffset);
    {
        // Check scaled extrapolation
        const TsExtrapolation pre = durScaled.GetPreExtrapolation();
        const TsExtrapolation post = durScaled.GetPostExtrapolation();
        TF_AXIOM(pre == durPreExtrap);
        TF_AXIOM(post.mode == durPostExtrap.mode);
        TF_AXIOM(post.loopBoundaryTime.value() ==
                 durPostExtrap.loopBoundaryTime.value()
                 * scaleFactor + scaleOffset);
    }
    {
        // Check knots
        TsKnotMap expectedKnots;
        for (const TsKnot& durKnot : durSpline.GetKnots()) {
            TsKnot expected = durKnot;
            expected.SetTime(durKnot.GetTime() * scaleFactor + scaleOffset);
            expected.SetPreTanWidth(durKnot.GetPreTanWidth() * scaleFactor);
            expected.SetPostTanWidth(durKnot.GetPostTanWidth() * scaleFactor);

            GfDuration value;
            TF_AXIOM(durKnot.GetValue(&value));
            expected.SetValue(value * scaleFactor);
            if (durKnot.IsDualValued()) {
                TF_AXIOM(durKnot.GetPreValue(&value));
                expected.SetPreValue(value * scaleFactor);
            }

            expectedKnots.insert(expected);
        }
        TF_AXIOM(expectedKnots == durScaled.GetKnots());
    }

    // Check negative scale and offset of GfDuration spline
    const TsSpline durNegScaled = durSpline.GetTimeScaled(-scaleFactor,
                                                      scaleOffset);
    {
        // Check negative scaled extrapolation
        const TsExtrapolation pre = durNegScaled.GetPreExtrapolation();
        const TsExtrapolation post = durNegScaled.GetPostExtrapolation();
        TF_AXIOM(pre.mode == durPostExtrap.mode);
        TF_AXIOM(pre.loopBoundaryTime.value() ==
                 durPostExtrap.loopBoundaryTime.value()
                 * (-scaleFactor) + scaleOffset);
        TF_AXIOM(post == durPreExtrap);
    }
    {
        // Build expected knots and check them
        const TfType t = Ts_GetType<GfDuration>();
        TsKnot k1(t);
        TsKnot k2(t);
        TsKnot k3(t);

        k3.SetTime(4.0 * (-scaleFactor) + scaleOffset);
        k3.SetValue(GfDuration(2.0 * (-scaleFactor)));
        k3.SetNextInterpolation(TsInterpLinear);

        k2.SetTime(2.0 * (-scaleFactor) + scaleOffset);
        k2.SetValue(GfDuration(3.0 * (-scaleFactor)));
        k2.SetPreValue(GfDuration(4.0 * (-scaleFactor)));
        k2.SetPostTanWidth(0.5 * scaleFactor);
        k2.SetPostTanSlope(GfDuration(-0.5));
        k2.SetNextInterpolation(TsInterpCurve);

        k1.SetTime(scaleOffset);
        k1.SetValue(GfDuration(2.0 * (-scaleFactor)));
        k1.SetPreTanWidth(0.25 * scaleFactor);
        k1.SetPreTanSlope(GfDuration(+0.5));

        TF_AXIOM(TsKnotMap({k3, k2, k1}) == durNegScaled.GetKnots());
    }
    return true;
}

bool TestConcatenate()
{
    // Concatenation of a single spline should yield a copy of the original.
    const TfType doubleType = Ts_GetType<double>();
    TsSpline spline(doubleType);
    TsKnot knot(doubleType);
    knot.SetTime(5.0);
    knot.SetValue(15.0);
    spline.SetKnot(knot);
    TF_AXIOM(TsSpline::Concatenate({spline}) == spline);

    // Check that all knots, both directly preserved and the result of
    // combining boundary knots, appear in the result spline.
    std::vector<TsSpline> splines;
    TsSpline s1(doubleType);
    TsSpline s2(doubleType);
    TsSpline s3(doubleType);
    s1.SetPreExtrapolation(TsExtrapLoopOscillate);
    s3.SetPreExtrapolation(TsExtrapLoopRepeat);
    s3.SetPostExtrapolation(TsExtrapLinear);

    TsKnot k1, k2, k3, k4, k5, k6;
    k1.SetTime(1.0);
    k1.SetValue(5.0);
    k1.SetPostTanWidth(0.5);
    k1.SetNextInterpolation(TsInterpCurve);

    k2.SetTime(3.0);
    k2.SetValue(4.0);
    k2.SetNextInterpolation(TsInterpValueBlock);
    k2.SetPreTanWidth(0.25);

    k3.SetTime(3.0);
    k3.SetValue(3.0);
    k3.SetNextInterpolation(TsInterpHeld);

    k4.SetTime(3.0);
    k4.SetValue(2.0);
    k4.SetPreValue(6.0);
    k4.SetPostTanSlope(0.2);
    k4.SetNextInterpolation(TsInterpLinear);

    k5.SetTime(5.0);
    k5.SetNextInterpolation(TsInterpCurve);
    k5.SetValue(0.0);
    k5.SetPostTanWidth(1.0);

    k6.SetTime(10.0);
    k6.SetValue(10.0);
    k6.SetPreTanWidth(1.0);

    s1.SetKnots({k1, k2});
    s2.SetKnots({k3}); // knot 3 will get overwritten entirely
    s3.SetKnots({k4, k5, k6});

    const TsSpline result = TsSpline::Concatenate({s1, s2, s3});
    TF_AXIOM(result.GetPreExtrapolation().mode == TsExtrapLoopOscillate);
    TF_AXIOM(result.GetPostExtrapolation().mode == TsExtrapLinear);

    // Check that we got the expected knots
    const TsKnotMap resultKnots = result.GetKnots();

    TF_AXIOM(resultKnots.size() == 4);
    TsKnot kBoundary = k4;
    kBoundary.SetPreValue(4.0);     // value of k2
    kBoundary.SetPreTanWidth(0.25); // pre tan width of k2
    kBoundary.SetPreTanSlope(0.0);  // pre tan slope of k2
    const std::vector<TsKnot> expectedKnots = {k1, kBoundary, k5, k6};
    size_t i = 0;
    auto it = resultKnots.begin();
    while (i < expectedKnots.size()) {
        TF_AXIOM(*it == expectedKnots[i]);
        i++;
        it++;
    }

    // Check that loopBoundaryTime is set for looping extrapolation so that
    // concatenating additional knots doesn't change the originally looped
    // region.
    TsSpline s1Looped = s1;
    s1Looped.SetPreExtrapolation(TsExtrapLoopReset);
    TsSpline s3Looped = s3;
    s3Looped.SetPostExtrapolation(TsExtrapLoopReset);

    const TsSpline resultLooped = TsSpline::Concatenate({s1Looped, s3Looped});
    TF_AXIOM(resultLooped.GetPreExtrapolation().mode == TsExtrapLoopReset);
    TF_AXIOM(resultLooped.GetPreExtrapolation().loopBoundaryTime.value()
             == 3.0);
    TF_AXIOM(resultLooped.GetPostExtrapolation().mode == TsExtrapLoopReset);
    TF_AXIOM(resultLooped.GetPostExtrapolation().loopBoundaryTime.value()
             == 3.0);

    return true;
}

int main(int argc, const char **argv)
{
    bool success = TestTruncate() && TestGetTimeScaled() && TestConcatenate();
    return success ? 0 : 1;
}
