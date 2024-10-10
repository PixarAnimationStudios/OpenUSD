//
// Copyright 2024 Pixar
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

PXR_NAMESPACE_USING_DIRECTIVE


#define VERIFY_GET(knot, getter, type, expected)                \
    {                                                           \
        type actual = type();                                   \
        TF_AXIOM(knot.getter(&actual));                         \
        TF_AXIOM(GfIsClose(actual, expected, 1e-3));            \
    }


template <typename T>
void TestKnotIO()
{
    // Default-constructed knot.
    TsTypedKnot<T> knot;
    TF_AXIOM(knot.GetTime() == 0);
    TF_AXIOM(knot.GetValueType() == Ts_GetType<T>());
    TF_AXIOM(knot.template IsHolding<T>());
    TF_AXIOM(knot.GetCurveType() == TsCurveTypeBezier);
    TF_AXIOM(knot.GetNextInterpolation() == TsInterpHeld);
    VERIFY_GET(knot, GetValue, T, 0);
    TF_AXIOM(!knot.IsDualValued());
    VERIFY_GET(knot, GetPreValue, T, 0);
    TF_AXIOM(knot.GetPreTanWidth() == 0);
    VERIFY_GET(knot, GetPreTanSlope, T, 0);
    TF_AXIOM(knot.GetPostTanWidth() == 0);
    VERIFY_GET(knot, GetPostTanSlope, T, 0);
    TF_AXIOM(knot.GetCustomData().empty());

    // Round-trip some values.
    TF_AXIOM(knot.SetTime(1));
    TF_AXIOM(knot.GetTime() == 1);
    TF_AXIOM(knot.SetNextInterpolation(TsInterpCurve));
    TF_AXIOM(knot.GetNextInterpolation() == TsInterpCurve);
    TF_AXIOM(knot.SetValue(T(14)));
    VERIFY_GET(knot, GetValue, T, 14);
    TF_AXIOM(knot.SetPreValue(T(-5)));
    TF_AXIOM(knot.IsDualValued());
    VERIFY_GET(knot, GetPreValue, T, -5);
    TF_AXIOM(knot.SetPreTanWidth(T(0.5)));
    TF_AXIOM(knot.GetPreTanWidth() == 0.5);
    TF_AXIOM(knot.SetPreTanSlope(T(2.3)));
    VERIFY_GET(knot, GetPreTanSlope, T, 2.3);
    TF_AXIOM(knot.SetCustomDataByKey("blah", VtValue(7)));
    TF_AXIOM(knot.GetCustomData()["blah"] == VtValue(7));

    // Clear pre-value.
    TF_AXIOM(knot.ClearPreValue());
    TF_AXIOM(!knot.IsDualValued());
    VERIFY_GET(knot, GetPreValue, T, 14);

    // Equality, assignment, and copy construction.
    TsTypedKnot<T> knot1;
    knot1.SetTime(1);
    TsTypedKnot<T> knot2;
    knot2.SetTime(2);
    TF_AXIOM(knot2 != knot1);
    knot2 = knot1;
    TF_AXIOM(knot2 == knot1);
    TsTypedKnot<T> knot3(knot2);
    TF_AXIOM(knot3 == knot2);

    // Move construction.
    TsTypedKnot<T> knotM(std::move(knot));
    TF_AXIOM(knotM.GetTime() == 1);
    VERIFY_GET(knotM, GetValue, T, 14);
    TF_AXIOM(knotM.GetCustomData()["blah"] == VtValue(7));
    TF_AXIOM(knot.GetTime() == 0);
    VERIFY_GET(knot, GetValue, T, 0);
    TF_AXIOM(knot.GetCustomData().empty());

    // Move assignment.
    TsTypedKnot<T> knotM2;
    knotM2 = std::move(knotM);
    TF_AXIOM(knotM2.GetTime() == 1);
    VERIFY_GET(knotM2, GetValue, T, 14);
    TF_AXIOM(knotM2.GetCustomData()["blah"] == VtValue(7));
    TF_AXIOM(knotM.GetTime() == 0);
    VERIFY_GET(knotM, GetValue, T, 0);
    TF_AXIOM(knotM.GetCustomData().empty());
}

template <typename T>
void TestSplineIO()
{
    TF_AXIOM(TsSpline::IsSupportedValueType(Ts_GetType<T>()));

    // Default-constructed spline.
    TsSpline spline;
    TF_AXIOM(!spline.GetValueType());
    TF_AXIOM(!spline.IsTimeValued());
    TF_AXIOM(spline.GetCurveType() == TsCurveTypeBezier);
    TF_AXIOM(spline.GetPreExtrapolation().mode == TsExtrapHeld);
    TF_AXIOM(spline.GetPostExtrapolation().mode == TsExtrapHeld);
    TF_AXIOM(spline.GetKnots().empty());
    TF_AXIOM(!spline.HasRegressiveTangents());
    T value = 0;
    TF_AXIOM(!spline.Eval(0, &value));
    TF_AXIOM(!spline.EvalPreValue(0, &value));
    TF_AXIOM(!spline.EvalDerivative(0, &value));
    TF_AXIOM(!spline.EvalPreDerivative(0, &value));
    TF_AXIOM(!spline.EvalHeld(0, &value));
    TF_AXIOM(!spline.EvalPreValueHeld(0, &value));
    TF_AXIOM(spline.IsEmpty());
    TF_AXIOM(!spline.HasValueBlocks());
    TF_AXIOM(!spline.HasLoops());
    TF_AXIOM(!spline.HasInnerLoops());
    TF_AXIOM(!spline.HasExtrapolatingLoops());
    TF_AXIOM(!spline.HasValueBlockAtTime(0));

    // Round-trip some values.
    spline.SetTimeValued(true);
    TF_AXIOM(spline.IsTimeValued());
    spline.SetPreExtrapolation(TsExtrapLinear);
    TF_AXIOM(spline.GetPreExtrapolation().mode == TsExtrapLinear);
    TF_AXIOM(spline.GetPreExtrapolation() == TsExtrapLinear);

    // Single-knot spline.
    TsTypedKnot<T> knot;
    TF_AXIOM(knot.SetTime(1));
    TF_AXIOM(knot.SetValue(T(5)));
    TF_AXIOM(spline.CanSetKnot(knot));
    TF_AXIOM(spline.SetKnot(knot));
    TF_AXIOM(!spline.IsEmpty());
    TF_AXIOM(spline.GetKnots().size() == 1);
    TF_AXIOM(*(spline.GetKnots().begin()) == knot);
    TsTypedKnot<T> knot2;
    TF_AXIOM(spline.GetKnot(1, &knot2));
    TF_AXIOM(knot2 == knot);
    TF_AXIOM(spline.Eval(0, &value) && value == 5);
    TF_AXIOM(spline.EvalPreValue(0, &value) && value == 5);
    TF_AXIOM(spline.EvalDerivative(0, &value) && value == 0);
    TF_AXIOM(spline.EvalPreDerivative(0, &value) && value == 0);
    TF_AXIOM(spline.EvalHeld(0, &value) && value == 5);
    TF_AXIOM(spline.EvalPreValueHeld(0, &value) && value == 5);

    // Equality, assignment, copy-on-write, and copy construction.
    TsSpline spline2;
    TF_AXIOM(spline2 != spline);
    spline2 = spline;
    TF_AXIOM(spline2 == spline);
    spline2.SetPostExtrapolation(TsExtrapLinear);
    TF_AXIOM(spline2 != spline);
    TsSpline spline3(spline2);
    TF_AXIOM(spline3 == spline2);

    // Setup for knot addition and removal.
    TsSpline splineAR;
    TsTypedKnot<T> knotAR1;
    TF_AXIOM(knotAR1.SetTime(1));
    TF_AXIOM(knotAR1.SetValue(T(1)));
    TsTypedKnot<T> knotAR2;
    TF_AXIOM(knotAR2.SetTime(2));
    TF_AXIOM(knotAR2.SetValue(T(2)));
    TsTypedKnot<T> knotAR3;
    TF_AXIOM(knotAR3.SetTime(3));
    TF_AXIOM(knotAR3.SetValue(T(3)));

    // Add knots indivdiually.
    TF_AXIOM(splineAR.SetKnot(knotAR1));
    TF_AXIOM(splineAR.SetKnot(knotAR2));
    TF_AXIOM(splineAR.SetKnot(knotAR3));
    TF_AXIOM(splineAR.GetKnots().size() == 3);
    TF_AXIOM(splineAR.Eval(-1, &value) && value == 1);
    TF_AXIOM(splineAR.Eval(2.5, &value) && value == 2);
    TF_AXIOM(splineAR.Eval(4, &value) && value == 3);
    splineAR.ClearKnots();
    TF_AXIOM(splineAR.IsEmpty());

    // Add knots as KnotMap.
    splineAR.SetKnots(TsKnotMap{knotAR1, knotAR2, knotAR3});
    TF_AXIOM(splineAR.GetKnots().size() == 3);
    TF_AXIOM(splineAR.Eval(-1, &value) && value == 1);
    TF_AXIOM(splineAR.Eval(2.5, &value) && value == 2);
    TF_AXIOM(splineAR.Eval(4, &value) && value == 3);

    // Remove a knot.
    splineAR.RemoveKnot(2);
    TF_AXIOM(splineAR.GetKnots().size() == 2);
    TF_AXIOM(splineAR.Eval(-1, &value) && value == 1);
    TF_AXIOM(splineAR.Eval(2.5, &value) && value == 1);
    TF_AXIOM(splineAR.Eval(4, &value) && value == 3);
}

int main()
{
    TestKnotIO<double>();
    TestKnotIO<float>();
    TestKnotIO<GfHalf>();

    TestSplineIO<double>();
    TestSplineIO<float>();
    TestSplineIO<GfHalf>();

    return 0;
}
