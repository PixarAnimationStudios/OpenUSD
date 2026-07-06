//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/ts/knot.h"
#include "pxr/base/ts/raii.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/tsTest_Museum.h"
#include "pxr/base/ts/tsTest_TsEvaluator.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/enum.h"

#include <iostream>
#include <fstream>

PXR_NAMESPACE_USING_DIRECTIVE

// Constants for constructing GfInterval objects
constexpr bool OPEN = false;

std::string FormatKnotMap(const TsKnotMap& knots)
{
    std::ostringstream str;

    str << "{\n";
    for (const TsKnot& knot : knots) {
        str << " " << knot << "\n";
    }
    str << "}";

    return str.str();
}

// Set the max side of the interval to open if the spline's post extrapolation
// resolves to value-block. Baked-out knots do not convey interpolations of
// value block resulting from extrapolation.
GfInterval
NormalizeInterval(const TsSpline& spline, const GfInterval& interval)
{
    GfInterval normInterval = interval;
    if (spline.GetPostExtrapolation().mode == TsExtrapValueBlock ||
        !spline.IsPostExtrapolationValid())
    {
        normInterval.SetMax(interval.GetMax(), OPEN);
    }
    return normInterval;
}


// Simplify access to the Museum
class SplineSrc
{
    TsTest_TsEvaluator _evaluator;
    std::vector<std::string> _names;
    std::map<std::string, TsSpline> _splines;
    std::map<std::string, GfInterval> _timeIntervals;
    
public:
    SplineSrc()
    {
        _names = TsTest_Museum::GetAllNames();
        // _names = {"InnerAndExtrapLoops"};

        for (const auto& name : _names) {
            const TsSpline spline = TsTest_Museum::GetSplineByName(
                name, Ts_GetType<double>());
            const TsKnotMap knots = spline.GetKnots();
            GfInterval knotInterval = knots.GetTimeSpan();

            if (spline.HasInnerLoops()) {
                knotInterval |= spline.GetInnerLoopParams().GetLoopedInterval();
            }

            _splines[name] = spline;
            _timeIntervals[name] = knotInterval;
        }
    }

    const std::vector<std::string>&
    AllNames() {
        return _names;
    }

    GfInterval
    TimeInterval(const std::string& name) {
        return TfMapLookupByValue(_timeIntervals, name, GfInterval());
    }

    TsSpline Get(const std::string& name,
                 const TfType& valueType = Ts_GetType<double>())
    {
        return TsTest_Museum::GetSplineByName(name, valueType);
    }
};

// Note that result, testId, and splineId are expected to be defined local
// variables. TEST_ASSERT must also be used inside a loop where "continue" is
// valid.
#define TEST_ASSERT(condition, ...)             \
if (!(condition)) {                             \
    std::cerr << testId << ": "                 \
              << splineId << ": "               \
              << TfStringPrintf(__VA_ARGS__)    \
              << std::endl;                     \
    result = false;                             \
    continue;                                   \
}

class BakeTest
{
public:
    SplineSrc splineSrc;
    std::vector<TfType> valueTypes = {
        Ts_GetType<double>(),
        Ts_GetType<float>(),
        Ts_GetType<GfHalf>()
    };

    std::map<TfType, double> epsilons;

    BakeTest()
    {
        epsilons[Ts_GetType<double>()] = 1.0e-10;
        epsilons[Ts_GetType<float>()] = 1.0e-5;
        epsilons[Ts_GetType<GfHalf>()] = 7.0e-3;
    }

    // Verify that the splines evaluate equivalently across interval.
    // This should be kept semantically in sync with
    // testTsSplineTransforms.cpp's VerifySplineEquivalence.
    bool VerifySplineEquivalence(const std::string& testId,
                                 const std::string& splineId,
                                 const TsSpline& spline1,
                                 const TsSpline& spline2,
                                 const GfInterval& interval,
                                 double epsilon)
    {
        // This variable is updated by TEST_ASSERT
        bool result = true;
        
        // Evaluate both splines across interval and ensure that the
        // results are close if not identical.
        double maxError = -1;
        double maxErrorTime = interval.GetMin() - 1.0;

        // Evaluate at 101 points across the time interval.
        for (int i = 0; i < 101; ++i) {
            double t = GfLerp(i/100.0, interval.GetMin(), interval.GetMax());
            bool valid1, valid2;
            double v1, v2;

            if (i == 100 && interval.IsMaxOpen()) {
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
                        " for interval=%s\n"
                        "spline1=%s and spline2=%s",
                        t, TfStringify(interval).c_str(),
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
                        " for interval=%s\n"
                        "spline1=%.12g and spline2=%.12g",
                        t, TfStringify(interval).c_str(),
                        v1, v2);
        }

        std::cout << testId << ": "
                  << splineId << ": "
                  << "maxError = " << maxError
                  << " at time = " << maxErrorTime
                  << " for interval " << interval
                  << std::endl;

        if (!result) {
            std::cerr << "Failing splines:\n"
                      << "spline1:\n" << spline1 << "\n"
                      << "spline2:\n" << spline2 << std::endl;
        }
            
        return result;
    }

    bool TestBakeInnerLoops()
    {
        // These variables are used by TEST_ASSERT
        bool result = true;
        const std::string testId = "TestBakeInnerLoops";
        
        for (const std::string& name : splineSrc.AllNames()) {
            for (const TfType& valueType : valueTypes) {
                // Also used by TEST_ASSERT
                const std::string splineId =
                    name + "<" + valueType.GetTypeName() + ">";

                TsSpline spline1 = splineSrc.Get(name, valueType);
                TsSpline spline2 = spline1;

                TsKnotMap knots1 = spline1.GetKnots();

                spline2.BakeInnerLoops();
                TsKnotMap knots2 = spline2.GetKnots();

                if (spline1.HasInnerLoops()) {
                    TEST_ASSERT(spline1 != spline2,
                                "Spline is unchanged after BakeInnerLoops\n%s",
                                TfStringify(spline2).c_str());
                } else {
                    TEST_ASSERT(spline1 == spline2,
                                "Spline is changed after BakeInnerLoops\n"
                                "spline1 =\n%s\n"
                                "spline2 =\n%s",
                                TfStringify(spline1).c_str(),
                                TfStringify(spline2).c_str());
                }

                const auto lp = spline2.GetInnerLoopParams();
                TEST_ASSERT(lp == TsLoopParams(),
                            "Baked spline has non-default inner loop params:\n"
                            "  start=%g, end=%g, numPreLoops=%d, numPostLoops=%d,"
                            " valueOffset=%g",
                            lp.protoStart, lp.protoEnd, lp.numPreLoops,
                            lp.numPreLoops, lp.valueOffset);

                // Verify that shared data and copy on write worked and that we
                // didn't change spline1 at all
                TEST_ASSERT(spline1 == splineSrc.Get(name, valueType),
                            "Original spline was changed by BakeInnerLoops"
                            " on copy of spline\n"
                            "spline1 =\n%s",
                            TfStringify(spline1).c_str());

                // Verify the interval that includes all knots in spline1 and
                // spline2
                GfInterval verifyInterval = knots1.GetTimeSpan() |
                                            knots2.GetTimeSpan();
                if (!VerifySplineEquivalence(testId, splineId,
                                             spline1, spline2,
                                             NormalizeInterval(
                                                spline1,
                                                verifyInterval),
                                             epsilons[valueType]))
                {
                    // Error message has already been reported.
                    return false;
                }
            }
        }

        return result;
    }

    bool TestGetKnotsWithInnerLoopsBaked()
    {
        // These variables are used by TEST_ASSERT
        bool result = true;
        const std::string testId = "TestGetKnotsWithInnerLoopsBaked";
        
        for (const std::string& name : splineSrc.AllNames()) {
            for (const TfType& valueType : valueTypes) {
                // Also used by TEST_ASSERT
                const std::string splineId =
                    name + "<" + valueType.GetTypeName() + ">";

                TsSpline spline1 = splineSrc.Get(name, valueType);
                TsSpline spline2 = splineSrc.Get(name, valueType);
                TsSpline spline3 = TsSpline(valueType);

                TsKnotMap knots1 = spline1.GetKnots();
                TsKnotMap bakedKnots = spline1.GetKnotsWithInnerLoopsBaked();

                // Verify that the spline did not change.
                TEST_ASSERT(spline1 == splineSrc.Get(name, valueType),
                            "Spline changed when GetKnotsWithInnerLoopsBaked"
                            " was called:\n"
                            "spline1 = \n"
                            "%s",
                            TfStringify(spline1).c_str());

                spline2.BakeInnerLoops();
                TsKnotMap knots2 = spline2.GetKnots();

                // Verify that BakeInnerLoops and GetKnotsWithInnerLoopsBaked
                // generate the same knots.
                TEST_ASSERT(bakedKnots == knots2,
                            "GetKnotsWithInnerLoopsBaked returned different"
                            " results than BakeInnerLoops/GetKnots\n"
                            "KnotMap1 = \n"
                            "%s\n"
                            "KnotMap2 = \n"
                            "%s",
                            FormatKnotMap(bakedKnots).c_str(),
                            FormatKnotMap(knots2).c_str());

                // Put the knots back into a spline and verify that it evaluates
                // correctly.
                {
                    // Do not de-regress as we insert them into a new spline. If
                    // the original was regressive, the copy should be as well.
                    TsAntiRegressionAuthoringSelector
                        selector(TsAntiRegressionNone);
                    spline3.SetKnots(bakedKnots);
                }

                spline3.SetPreExtrapolation(spline1.GetPreExtrapolation());
                spline3.SetPostExtrapolation(spline1.GetPostExtrapolation());

                // Verify the interval that includes all knots in spline1 and
                // spline2
                GfInterval verifyInterval = knots1.GetTimeSpan() |
                                            bakedKnots.GetTimeSpan();
                if (!VerifySplineEquivalence(testId, splineId,
                                             spline1, spline3,
                                             NormalizeInterval(
                                                spline1,
                                                verifyInterval),
                                             epsilons[valueType]))
                {
                    // Error message has already been reported.
                    return false;
                }
            }
        }

        return result;
    }

    bool TestGetKnotsWithLoopsBaked()
    {
        // These variables are used by TEST_ASSERT
        bool result = true;
        const std::string testId = "TestGetKnotsWithLoopsBaked";
        
        for (const std::string& name : splineSrc.AllNames()) {
            const GfInterval interval = splineSrc.TimeInterval(name);
            
            for (const TfType& valueType : valueTypes) {
                // Also used by TEST_ASSERT
                const std::string splineId =
                    name + "<" + valueType.GetTypeName() + ">";

                TsSpline spline1 = splineSrc.Get(name, valueType);

                // The middle 50% of the time interval of the knots.
                GfInterval shortInterval = GfInterval(
                    GfLerp(0.25, interval.GetMin(), interval.GetMax()),
                    GfLerp(0.75, interval.GetMin(), interval.GetMax()));

                // 200% of the time interval of the knots.
                GfInterval longInterval = GfInterval(
                    GfLerp(-0.5, interval.GetMin(), interval.GetMax()),
                    GfLerp( 1.5, interval.GetMin(), interval.GetMax()));


                TsKnotMap shortKnots =
                    spline1.GetKnotsWithLoopsBaked(shortInterval);
                TsKnotMap mediumKnots = 
                    spline1.GetKnotsWithLoopsBaked(interval);
                TsKnotMap longKnots =
                    spline1.GetKnotsWithLoopsBaked(longInterval);

                TsSpline shortSpline(valueType);
                TsSpline mediumSpline(valueType);
                TsSpline longSpline(valueType);
                {
                    // Do not de-regress as we insert them into a new spline. If
                    // the original was regressive, the copy should be as well.
                    TsAntiRegressionAuthoringSelector
                        selector(TsAntiRegressionNone);
                    
                    shortSpline.SetKnots(shortKnots);
                    mediumSpline.SetKnots(mediumKnots);
                    longSpline.SetKnots(longKnots);
                }

                longSpline.SetPreExtrapolation(spline1.GetPreExtrapolation());
                longSpline.SetPostExtrapolation(spline1.GetPostExtrapolation());

                if (!VerifySplineEquivalence(testId, splineId,
                                             spline1, shortSpline,
                                             NormalizeInterval(
                                                spline1,
                                                shortInterval),
                                             epsilons[valueType])
                    || !VerifySplineEquivalence(testId, splineId,
                                                spline1, mediumSpline,
                                                NormalizeInterval(
                                                    spline1,
                                                    interval),
                                                epsilons[valueType])
                    || !VerifySplineEquivalence(testId, splineId,
                                                spline1, longSpline,
                                                NormalizeInterval(
                                                    spline1,
                                                    longInterval),
                                                epsilons[valueType]))
                {
                    // Error message has already been reported.
                    return false;
                }
            }
        }

        return result;
    }
};

int main(int argc, const char **argv)
{
    BakeTest bakeTest;

    if (!bakeTest.TestBakeInnerLoops() ||
        !bakeTest.TestGetKnotsWithInnerLoopsBaked() ||
        !bakeTest.TestGetKnotsWithLoopsBaked())
    {
        return 1;
    }

    return 0;
}
