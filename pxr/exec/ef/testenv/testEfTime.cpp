//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/exec/ef/time.h"
#include "pxr/exec/ef/timeInterval.h"

PXR_NAMESPACE_USING_DIRECTIVE

int 
main()
{
    const EfTime::SplineEvaluationFlags CustomFlag = (1 << 0);

    EfTime time;

    // Test default constructed time
    time = EfTime();
    TF_AXIOM(time.GetTimeCode().IsDefault());
    TF_AXIOM(!time.GetTimeCode().IsPreTime());
    TF_AXIOM(time.GetSplineEvaluationFlags() == 0);

    // Test constructors
    time = EfTime(UsdTimeCode::Default());
    TF_AXIOM(time.GetTimeCode().IsDefault());
    TF_AXIOM(!time.GetTimeCode().IsPreTime());
    TF_AXIOM(time.GetSplineEvaluationFlags() == 0);

    time = EfTime(0.0, CustomFlag);
    TF_AXIOM(!time.GetTimeCode().IsDefault());
    TF_AXIOM(!time.GetTimeCode().IsPreTime());
    TF_AXIOM(time.GetSplineEvaluationFlags() == CustomFlag);

    time = EfTime(UsdTimeCode::PreTime(0));
    TF_AXIOM(!time.GetTimeCode().IsDefault());
    TF_AXIOM(time.GetTimeCode().IsPreTime());
    TF_AXIOM(time.GetSplineEvaluationFlags() == 0);

    time = EfTime(UsdTimeCode::PreTime(0), CustomFlag);
    TF_AXIOM(!time.GetTimeCode().IsDefault());
    TF_AXIOM(time.GetTimeCode().IsPreTime());
    TF_AXIOM(time.GetSplineEvaluationFlags() == CustomFlag);

    // Verify that passing a 0 initializes the spline evaluation flags
    time = EfTime(0.0, 0);
    TF_AXIOM(!time.GetTimeCode().IsPreTime());
    TF_AXIOM(time.GetSplineEvaluationFlags() == 0);

    // Test SetFrame
    time.SetTimeCode(1.0);
    TF_AXIOM(!time.GetTimeCode().IsDefault());
    TF_AXIOM(time.GetTimeCode().GetValue() == 1.0);

    // Test evaluation location
    time.SetTimeCode(UsdTimeCode::PreTime(1.0));
    TF_AXIOM(time.GetTimeCode().IsPreTime());

    // Test SetSplineEvaluationFlags
    time.SetSplineEvaluationFlags(CustomFlag);
    TF_AXIOM(time.GetSplineEvaluationFlags() == CustomFlag);

    // Test ==
    TF_AXIOM(EfTime() == EfTime());
    TF_AXIOM(!(EfTime(0.0) == EfTime()));
    TF_AXIOM(!(EfTime(1.0) == EfTime()));

    EfTime defaultTime;
    EfTime defaultWithFlag;
    defaultWithFlag.SetSplineEvaluationFlags(CustomFlag);
    TF_AXIOM(defaultWithFlag == defaultTime);

    // Test <
    TF_AXIOM(EfTime(0.0) < EfTime(1.0));
    TF_AXIOM(!(defaultTime < EfTime()));
    TF_AXIOM(EfTime() < EfTime(UsdTimeCode::PreTime(0.0)));
    TF_AXIOM(EfTime(UsdTimeCode::PreTime(0.0)) < EfTime(0.0));
    TF_AXIOM(EfTime() < EfTime(0.0, CustomFlag));

    // Test interval membership
    time = EfTime();
    TF_AXIOM(!EfTimeInterval(GfInterval()).Contains(time));
    TF_AXIOM(!EfTimeInterval(GfInterval(-1.0, 1.0)).Contains(time));

    time = EfTime(0.0);
    TF_AXIOM(!EfTimeInterval(GfInterval()).Contains(time));
    TF_AXIOM(EfTimeInterval(GfInterval(-1.0, 1.0)).Contains(time));

    time = EfTime(-1.0);
    TF_AXIOM(EfTimeInterval(GfInterval(-1.0, 1.0)).Contains(time));
    TF_AXIOM(!EfTimeInterval(
        GfInterval(-1.0, 1.0, /* minClosed */ false, /* maxClosed */ false))
        .Contains(time));
    TF_AXIOM(EfTimeInterval(
        GfMultiInterval(GfInterval(-1.0, 1.0))).Contains(time));
    TF_AXIOM(!EfTimeInterval(GfMultiInterval(
        GfInterval(-1.0, 1.0, /* minClosed */ false, /* maxClosed */ false)))
        .Contains(time));

    time = EfTime(1.0);
    TF_AXIOM(EfTimeInterval(GfInterval(-1.0, 1.0)).Contains(time));
    TF_AXIOM(!EfTimeInterval(
        GfInterval(-1.0, 1.0, /* minClosed */ false, /* maxClosed */ false))
        .Contains(time));
    TF_AXIOM(EfTimeInterval(
        GfMultiInterval(GfInterval(-1.0, 1.0))).Contains(time));
    TF_AXIOM(!EfTimeInterval(GfMultiInterval(
        GfInterval(-1.0, 1.0, /* minClosed */ false, /* maxClosed */ false)))
        .Contains(time));

    time = EfTime(UsdTimeCode::PreTime(-1.0));
    TF_AXIOM(!EfTimeInterval(GfInterval(-1.0, 1.0)).Contains(time));
    TF_AXIOM(!EfTimeInterval(
        GfInterval(-1.0, 1.0, /* minClosed */ false, /* maxClosed */ false))
        .Contains(time));
    TF_AXIOM(!EfTimeInterval(
        GfMultiInterval(GfInterval(-1.0, 1.0))).Contains(time));
    TF_AXIOM(!EfTimeInterval(GfMultiInterval(
        GfInterval(-1.0, 1.0, /* minClosed */ false, /* maxClosed */ false)))
        .Contains(time));

    time = EfTime(UsdTimeCode::PreTime(1.0));
    TF_AXIOM(EfTimeInterval(GfInterval(-1.0, 1.0)).Contains(time));
    TF_AXIOM(EfTimeInterval(
        GfInterval(-1.0, 1.0, /* minClosed */ false, /* maxClosed */ false))
        .Contains(time));
    TF_AXIOM(EfTimeInterval(
        GfMultiInterval(GfInterval(-1.0, 1.0))).Contains(time));
    TF_AXIOM(EfTimeInterval(GfMultiInterval(
        GfInterval(-1.0, 1.0, /* minClosed */ false, /* maxClosed */ false)))
        .Contains(time));

    time = EfTime(0.0);
    TF_AXIOM(EfTimeInterval(GfInterval(0.0, 0.0)).Contains(time));
    TF_AXIOM(!EfTimeInterval(
        GfInterval(0.0, 0.0, /* minClosed */ false, /* maxClosed */ false))
        .Contains(time));
    TF_AXIOM(!EfTimeInterval(
        GfInterval(0.0, 0.0, /* minClosed */ true, /* maxClosed */ false))
        .Contains(time));
    TF_AXIOM(!EfTimeInterval(
        GfInterval(0.0, 0.0, /* minClosed */ false, /* maxClosed */ true))
        .Contains(time));

    return 0;
}
