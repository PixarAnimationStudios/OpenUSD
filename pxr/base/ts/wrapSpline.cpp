//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/ts/typeHelpers.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/valueFromPython.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/make_constructor.hpp"
#include "pxr/external/boost/python/operators.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;


static TsSpline* _WrapInit(
    const std::string &typeName)
{
    const TfType tfType = Ts_GetTypeFromTypeName(typeName);
    if (!tfType)
    {
        TF_CODING_ERROR("Invalid spline type name '%s'", typeName.c_str());
        return nullptr;
    }

    return new TsSpline(tfType);
}

static std::string _WrapStr(
    const TsSpline &spline)
{
    return TfStringify(spline);
}

static std::string _WrapGetValueTypeName(
    const TsSpline &spline)
{
    return Ts_GetTypeNameFromType(spline.GetValueType());
}

static GfInterval _WrapSetKnot(
    TsSpline &spline, const TsKnot &knot)
{
    GfInterval affectedInterval;
    spline.SetKnot(knot, &affectedInterval);
    return affectedInterval;
}

static object _WrapGetKnot(
    const TsSpline &spline, const TsTime time)
{
    TsKnot knot;
    if (spline.GetKnot(time, &knot))
    {
        return object(knot);
    }

    return object();
}

static void _WrapRemoveKnot(
    TsSpline &spline, const TsTime time)
{
    spline.RemoveKnot(time);
}

static object _WrapBreakdown(
    TsSpline& spline, const TsTime time)
{
    // The C++ version returns a bool and populates a pointed-to GfInterval
    // object. The python version returns a Gf.Interval or None. The GfInterval
    // object evaluates to True in a boolean context so it can be used the
    // same way as the C++ version:
    //    if spline.Breakdown(time):
    //        ...
    GfInterval affectedInterval;
    bool status = spline.Breakdown(time, &affectedInterval);
    if (status) {
        return object(affectedInterval);
    } else {
        return object();
    }
}

struct _CanBreakdownResult: public TfPyAnnotatedBoolResult<std::string>
{
    _CanBreakdownResult(bool val, const std::string reason)
    : TfPyAnnotatedBoolResult<std::string>(val, reason)
    {}
};

static
_CanBreakdownResult
_WrapCanBreakdown(
    TsSpline& spline, const TsTime time)
{
    std::string reason;
    bool result = spline.CanBreakdown(time, &reason);
    return _CanBreakdownResult(result, reason);
}

#define WRAP_EVAL(method)                                   \
    static object _Wrap##method(                            \
        const TsSpline &spline, const TsTime time)          \
    {                                                       \
        double val = 0;                                     \
        const bool haveValue = spline.method(time, &val);   \
        return (haveValue ? object(val) : object());        \
    }

WRAP_EVAL(Eval);
WRAP_EVAL(EvalPreValue);
WRAP_EVAL(EvalDerivative);
WRAP_EVAL(EvalPreDerivative);
WRAP_EVAL(EvalHeld);
WRAP_EVAL(EvalPreValueHeld);

static object _WrapSample(
    const TsSpline &spline,
    const GfInterval& timeInterval,
    double timeScale,
    double valueScale,
    double tolerance,
    bool withSources)
{
    if (withSources) {
        TsSplineSamplesWithSources<GfVec2d> samplesWithSources;

        if (spline.Sample(timeInterval,
                          timeScale,
                          valueScale,
                          tolerance,
                          &samplesWithSources))
        {
            return object(samplesWithSources);
        }
    } else {
        TsSplineSamples<GfVec2d> samples;

        if (spline.Sample(timeInterval,
                          timeScale,
                          valueScale,
                          tolerance,
                          &samples))
        {
            return object(samples);
        }
    }

    return object();
}

void wrapSpline()
{
    using This = TsSpline;

    class_<This>("Spline", no_init)

        .def("__init__",
            make_constructor(
                &_WrapInit,
                default_call_policies(),
                (arg("typeName") = "double")))
        .def(init<const TsSpline &>())

        .def(self == self)
        .def(self != self)

        .def("__str__", &_WrapStr)

        .def("GetValueTypeName", &_WrapGetValueTypeName)
        .def("SetTimeValued", &This::SetTimeValued)
        .def("IsTimeValued", &This::IsTimeValued)
        .def("SetCurveType", &This::SetCurveType)
        .def("GetCurveType", &This::GetCurveType)

        .def("SetPreExtrapolation", &This::SetPreExtrapolation)
        .def("GetPreExtrapolation", &This::GetPreExtrapolation)
        .def("SetPostExtrapolation", &This::SetPostExtrapolation)
        .def("GetPostExtrapolation", &This::GetPostExtrapolation)

        .def("SetInnerLoopParams", &This::SetInnerLoopParams)
        .def("GetInnerLoopParams", &This::GetInnerLoopParams)

        .def("SetKnots", &This::SetKnots)
        .def("SetKnot", &_WrapSetKnot)
        .def("GetKnots", &This::GetKnots)
        .def("GetKnot", &_WrapGetKnot)

        .def("Breakdown", &_WrapBreakdown,
             arg("time"))
        .def("CanBreakdown", &_WrapCanBreakdown,
             arg("time"))

        .def("ClearKnots", &This::ClearKnots)
        .def("RemoveKnot", &_WrapRemoveKnot)

        .def("GetAntiRegressionAuthoringMode",
            &This::GetAntiRegressionAuthoringMode)
        .staticmethod("GetAntiRegressionAuthoringMode")
        .def("HasRegressiveTangents", &This::HasRegressiveTangents)
        .def("AdjustRegressiveTangents", &This::AdjustRegressiveTangents)

        .def("Eval", &_WrapEval)
        .def("EvalPreValue", &_WrapEvalPreValue)
        .def("EvalDerivative", &_WrapEvalDerivative)
        .def("EvalPreDerivative", &_WrapEvalPreDerivative)
        .def("EvalHeld", &_WrapEvalHeld)
        .def("EvalPreValueHeld", &_WrapEvalPreValueHeld)

        .def("Sample", &_WrapSample,
             (arg("timeInterval"),
              arg("timeScale"),
              arg("valueScale"),
              arg("tolerance"),
              arg("withSources") = false))

        .def("DoSidesDiffer", &This::DoSidesDiffer)

        .def("IsEmpty", &This::IsEmpty)
        .def("HasValueBlocks", &This::HasValueBlocks)
        .def("HasLoops", &This::HasLoops)
        .def("HasInnerLoops", &This::HasInnerLoops)
        .def("HasExtrapolatingLoops", &This::HasExtrapolatingLoops)

        .def("HasValueBlockAtTime", &This::HasValueBlockAtTime)

        .def("IsSupportedValueType",
            &This::IsSupportedValueType)
        .staticmethod("IsSupportedValueType")
        ;

    _CanBreakdownResult::Wrap<_CanBreakdownResult>("_CanBreakdownResult",
                                                   "reason");
    VtValueFromPython<TsSpline>();
}
