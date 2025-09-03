//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/knot.h"
#include "pxr/base/gf/math.h"
#include "pxr/base/tf/diagnosticLite.h"

#include <iostream>
#include <optional>
#include <limits>

PXR_NAMESPACE_USING_DIRECTIVE

struct Expected
{
    TsTime time;
    std::optional<double> preValue;
    std::optional<double> value;
    std::optional<double> preDerivative;
    std::optional<double> derivative;
    std::optional<double> preHeld;
    std::optional<double> held;
};

template <typename T>
void Expect(const TsSpline& spline, const Expected& expected)
{
    TF_AXIOM(spline.GetValueType() == Ts_GetType<T>());

    // numeric_limits<T>::epsilon() returns the difference between 1.0 and the
    // next larger representable value, i.e., 1 ulp when the exponent is 0. We
    // have values in the range 8-16 so multiplying by 8 represents ~1 ulp for
    // these values.
    const double epsilon = 8 * std::numeric_limits<T>::epsilon();

    T value;
    if (spline.EvalPreValue(expected.time, &value)) {
        TF_AXIOM(expected.preValue);
        if (!GfIsClose(value, *expected.preValue, epsilon)) {
            std::cout.precision(std::numeric_limits<T>::max_digits10);
            std::cout << "Value mismatch for EvalPreValue<"
                      << TfType::Find<T>().GetTypeName()
                      << ">(" << expected.time << "):\n"
                      << "    value = " << value << "\n"
                      << "    expected = " << *expected.preValue << "\n"
                      << "    epsilon = " << epsilon << std::endl;
        }
        TF_AXIOM(GfIsClose(value, *expected.preValue, epsilon));
    } else {
        TF_AXIOM(!expected.preValue);
    }

    if (spline.Eval(expected.time, &value)) {
        TF_AXIOM(expected.value);
        if (!GfIsClose(value, *expected.value, epsilon)) {
            std::cout.precision(std::numeric_limits<T>::max_digits10);
            std::cout << "Value mismatch for Eval<"
                      << TfType::Find<T>().GetTypeName()
                      << ">(" << expected.time << "):\n"
                      << "    value = " << value << "\n"
                      << "    expected = " << *expected.value << "\n"
                      << "    epsilon = " << epsilon << std::endl;
        }
        TF_AXIOM(GfIsClose(value, *expected.value, epsilon));
    } else {
        TF_AXIOM(!expected.value);
    }

    if (spline.EvalPreDerivative(expected.time, &value)) {
        TF_AXIOM(expected.preDerivative);
        if (!GfIsClose(value, *expected.preDerivative, epsilon)) {
            std::cout.precision(std::numeric_limits<T>::max_digits10);
            std::cout << "Value mismatch for EvalPreDerivative<"
                      << TfType::Find<T>().GetTypeName()
                      << ">(" << expected.time << "):\n"
                      << "    value = " << value << "\n"
                      << "    expected = " << *expected.preDerivative << "\n"
                      << "    epsilon = " << epsilon << std::endl;
        }
        TF_AXIOM(GfIsClose(value, *expected.preDerivative, epsilon));
    } else {
        TF_AXIOM(!expected.preDerivative);
    }

    if (spline.EvalDerivative(expected.time, &value)) {
        TF_AXIOM(expected.derivative);
        if (!GfIsClose(value, *expected.derivative, epsilon)) {
            std::cout.precision(std::numeric_limits<T>::max_digits10);
            std::cout << "Value mismatch for EvalDerivative<"
                      << TfType::Find<T>().GetTypeName()
                      << ">(" << expected.time << "):\n"
                      << "    value = " << value << "\n"
                      << "    expected = " << *expected.derivative << "\n"
                      << "    epsilon = " << epsilon << std::endl;
        }
        TF_AXIOM(GfIsClose(value, *expected.derivative, epsilon));
    } else {
        TF_AXIOM(!expected.derivative);
    }

    if (spline.EvalPreValueHeld(expected.time, &value)) {
        TF_AXIOM(expected.preHeld);
        if (!GfIsClose(value, *expected.preHeld, epsilon)) {
            std::cout.precision(std::numeric_limits<T>::max_digits10);
            std::cout << "Value mismatch for EvalPreValueHeld<"
                      << TfType::Find<T>().GetTypeName()
                      << ">(" << expected.time << "):\n"
                      << "    value = " << value << "\n"
                      << "    expected = " << *expected.preHeld << "\n"
                      << "    epsilon = " << epsilon << std::endl;
        }
        TF_AXIOM(GfIsClose(value, *expected.preHeld, epsilon));
    } else {
        TF_AXIOM(!expected.preHeld);
    }

    if (spline.EvalHeld(expected.time, &value)) {
        TF_AXIOM(expected.held);
        if (!GfIsClose(value, *expected.held, epsilon)) {
            std::cout.precision(std::numeric_limits<T>::max_digits10);
            std::cout << "Value mismatch for EvalHeld<"
                      << TfType::Find<T>().GetTypeName()
                      << ">(" << expected.time << "):\n"
                      << "    value = " << value << "\n"
                      << "    expected = " << *expected.held << "\n"
                      << "    epsilon = " << epsilon << std::endl;
        }
        TF_AXIOM(GfIsClose(value, *expected.held, epsilon));
    } else {
        TF_AXIOM(!expected.held);
    }

        
}
    

template <typename T>
TsTypedKnot<T> CreateKnot(TsTime time,
                          TsInterpMode interpMode,
                          double preValue,
                          double value,
                          TsTime preTanWidth,
                          double preTanSlope,
                          TsTime postTanWidth,
                          double postTanSlope)
{
    TsTypedKnot<T> knot;
    knot.SetTime(time);
    knot.SetNextInterpolation(interpMode);
    knot.SetPreValue(T(preValue));
    knot.SetValue(T(value));
    knot.SetPreTanWidth(preTanWidth);
    knot.SetPreTanSlope(T(preTanSlope));
    knot.SetPostTanWidth(postTanWidth);
    knot.SetPostTanSlope(T(postTanSlope));

    return knot;
}
    
template <typename T>
void TestSplineEval()
{
    TF_AXIOM(TsSpline::IsSupportedValueType(Ts_GetType<T>()));

    // Spline with a held, blocked, linear, and curved segment.
    TsSpline spline;
    TsTypedKnot<T> k1 = CreateKnot<T>(0.0, TsInterpHeld,
                                      5.0, 10.0,
                                      1.0, 1.0,
                                      1.0, -1.0);
    
    TsTypedKnot<T> k2 = CreateKnot<T>(4.0, TsInterpValueBlock,
                                      3.0, 6.0,
                                      1.0, 1.0,
                                      1.0, -1.0);
    
    TsTypedKnot<T> k3 = CreateKnot<T>(8.0, TsInterpLinear,
                                      8.0, 8.0,
                                      1.0, 1.0,
                                      1.0, -2.0);
    
    TsTypedKnot<T> k4 = CreateKnot<T>(12.0, TsInterpCurve,
                                      0.0, 4.0,
                                      1.0, 1.0,
                                      2.0, 1.0);
    
    TsTypedKnot<T> k5 = CreateKnot<T>(16.0, TsInterpLinear,
                                      8.0, 10.0,
                                      2.0, 0.0,
                                      1.0, -1.0);

    spline.SetKnots(TsKnotMap{k1, k2, k3, k4, k5});

    TF_AXIOM(spline.GetValueType() == Ts_GetType<T>());
    TF_AXIOM(spline.GetCurveType() == TsCurveTypeBezier);

    // Reduce line length in the initial values below and make look more like
    // python. :-)
    const auto& None = std::nullopt;
    
    // Expected values for the non-curved segments of the spline
    Expected nonCurved[] = {
        {0, 5.0, 10.0, 0.0, 0.0, 5.0, 10.0},
        {2, 10.0, 10.0, 0.0, 0.0, 10.0, 10.0},
        {4, 10.0, None, 0.0, None, 10.0, None},
        {6, None, None, None, None, None, None},
        {8, None, 8.0, None, -2.0, None, 8.0},
        {10, 4.0, 4.0, -2.0, -2.0, 8.0, 8.0},
        {12, 0.0, 4.0, -2.0, 1.0, 8.0, 4.0},
    };

    // Expected values for curved Bezier segments of the spline
    Expected bezier[] = {
        {12, 0.0, 4.0, -2.0, 1.0, 8.0, 4.0},
        {13, 5.195309037843946,  5.195309037843946,
             1.4154939577019203, 1.4154939577019203, 4.0, 4.0},
        {14, 6.75, 6.75, 1.5, 1.5, 4.0, 4.0},
        {15, 7.771738865743875,  7.771738865743875,
             0.5358790778895212, 0.5358790778895212, 4.0, 4.0},
        {16, 8.0, 10.0, 0.0, 0.0, 4.0, 10.0},
    };

    // Expected values for curved Hermite segments of the spline
    Expected hermite[] = {
        {12, 0.0, 4.0, -2.0, 1.0, 8.0, 4.0},
        {13, 5.1875, 5.1875, 1.3125, 1.3125, 4.0, 4.0},
        {14, 6.5, 6.5, 1.25, 1.25, 4.0, 4.0},
        {15, 7.5625, 7.5625, 0.8125, 0.8125, 4.0, 4.0},
        {16, 8.0, 10.0, 0.0, 0.0, 4.0, 10.0},
    };
        
    for (TsCurveType curveType : {TsCurveTypeBezier, TsCurveTypeHermite}) {
        spline.SetCurveType(curveType);

        
        std::cout << std::string(72, '=') << "\n"
                  << "Testing " << spline << std::endl;
        
        for (const auto& expect : nonCurved) {
            Expect<T>(spline, expect);
        }

        if (curveType == TsCurveTypeBezier) {
            for (const auto& expect : bezier) {
                Expect<T>(spline, expect);
            }
        } else {
            for (const auto& expect : hermite) {
                Expect<T>(spline, expect);
            }
        }
    }
}

int main()
{
    TestSplineEval<double>();
    TestSplineEval<float>();
    TestSplineEval<GfHalf>();

    return 0;
}
