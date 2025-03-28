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


PXR_NAMESPACE_CLOSE_SCOPE

#endif
