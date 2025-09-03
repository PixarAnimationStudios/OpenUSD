//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TS_EVAL_H
#define PXR_BASE_TS_EVAL_H

#include "pxr/pxr.h"
#include "pxr/base/ts/api.h"
#include "pxr/base/ts/types.h"

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

struct Ts_SplineData;


enum Ts_EvalAspect
{
    Ts_EvalValue,
    Ts_EvalHeldValue,
    Ts_EvalDerivative
};

enum Ts_EvalLocation
{
    Ts_EvalPre,
    Ts_EvalAtTime,  // AtTime is implemented identically to Post,
                    // but the intent of Post is a limit, while AtTime is exact.
    Ts_EvalPost
};


// Evaluates a spline's value or derivative at a given time.  An empty return
// value means there is no value or derivative at all.
//
TS_API
std::optional<double>
Ts_Eval(
    const Ts_SplineData *data,
    TsTime time,
    Ts_EvalAspect aspect,
    Ts_EvalLocation location);

// Ts_Breakdown is used for both TsSpline::Breakdown and CanBreakdown. It
// inserts (or reports whether it could insert) a knot at \c atTime while trying
// to maintain the shape of the curve as much as possible.
//
// \param data is a pointer to the spline's internal, untyped Ts_SplineData
// \param atTime is the time at which to insert a knot
// \param testOnly is a flag to test for allowed breakdown without making
//        any changes.
// \param affectedIntervalOut is a pointer to a GfInterval that receives the
//        time interval in the spline that was modified and, for example,
//        may need to be redrawn in a GUI.
// \param reason is a pointer to a string into which a failure message will
//        be stored if the breakdown operation fails.
//
// \return true if the breakdown operation succeeded or for CanBreakdown if the
// operation would succeed. Returns false if the operation did or would fail.
TS_API
bool
Ts_Breakdown(
    Ts_SplineData* const data,
    TsTime atTime,
    bool testOnly,
    GfInterval *affectedIntervalOut,
    std::string* reason);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
