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
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/valueFromPython.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

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

static TsKnotMap _WrapGetKnots(
    const TsSpline &spline)
{
    return spline.GetKnots();
}

static TsKnotMap _WrapGetKnots_WithInterval(
    const TsSpline &spline,
    const GfInterval &interval)
{
    return spline.GetKnots(interval);
}

static void _WrapRemoveKnot(
    TsSpline &spline, const TsTime time)
{
    spline.RemoveKnot(time);
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
    static nb::object _Wrap##method(                        \
        const TsSpline &spline, const TsTime time)          \
    {                                                       \
        double val = 0;                                     \
        const bool haveValue = spline.method(time, &val);   \
        return (haveValue ? nb::cast(val) : nb::none());    \
    }

WRAP_EVAL(Eval);
WRAP_EVAL(EvalPreValue);
WRAP_EVAL(EvalDerivative);
WRAP_EVAL(EvalPreDerivative);
WRAP_EVAL(EvalHeld);
WRAP_EVAL(EvalPreValueHeld);

static nb::object _WrapSample(
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
            return nb::cast(samplesWithSources);
        }
    } else {
        TsSplineSamples<GfVec2d> samples;

        if (spline.Sample(timeInterval,
                          timeScale,
                          valueScale,
                          tolerance,
                          &samples))
        {
            return nb::cast(samples);
        }
    }

    return nb::none();
}

static GfInterval _WrapDiff(
    const TsSpline& self,
    const TsSpline& other,
    const GfInterval& compareInterval)
{
    return self.Diff(other, compareInterval);
}

void wrapSpline(nanobind::module_& m)
{
    using This = TsSpline;

    // Do not generate compiler warnings for the deprecated inner looping
    // methods
    ARCH_PRAGMA_PUSH
    ARCH_PRAGMA_DEPRECATED
    nb::class_<This>(m, "Spline")

        .def("__init__",
             [](This* self, const std::string& typeName) {
                 TfErrorMark mark;
                 const TfType tfType = Ts_GetTypeFromTypeName(typeName);
                 if (!tfType) {
                     TF_CODING_ERROR("Invalid spline type name '%s'",
                                     typeName.c_str());
                     if (TfPyConvertTfErrorsToPythonException(mark)) {
                         throw nb::python_error();
                     }
                 }
                 new (self) This(tfType);
             },
             nb::arg("typeName") = "double")
        .def(nb::init<const TsSpline &>())

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)

        .def("__str__", &_WrapStr)

        .def("GetValueTypeName", &_WrapGetValueTypeName)
        .def("IsTimeValued", &This::IsTimeValued)
        .def("SetCurveType", &This::SetCurveType)
        .def("GetCurveType", &This::GetCurveType)

        .def("SetPreExtrapolation", &This::SetPreExtrapolation)
        .def("GetPreExtrapolation", &This::GetPreExtrapolation)
        .def("SetPostExtrapolation", &This::SetPostExtrapolation)
        .def("GetPostExtrapolation", &This::GetPostExtrapolation)
        .def("IsPreExtrapolationValid", &This::IsPreExtrapolationValid)
        .def("IsPostExtrapolationValid", &This::IsPostExtrapolationValid)

        .def("SetInnerLoopParams", &This::SetInnerLoopParams)
        .def("GetInnerLoopParams", &This::GetInnerLoopParams)

        .def("SetKnots", &This::SetKnots)
        .def("SetKnot", &_WrapSetKnot)
        .def("GetKnots", &_WrapGetKnots)
        .def("GetKnots", &_WrapGetKnots_WithInterval)
        .def("GetKnot", [](const TsSpline& spline, TsTime time) -> nb::object {
            TsKnot knot;
            if (spline.GetKnot(time, &knot)) {
                return nb::cast(knot);
            }
            return nb::none();
        })

        .def("Breakdown", [](TsSpline& spline, TsTime time) -> nb::object {
            GfInterval affectedInterval;
            if (spline.Breakdown(time, &affectedInterval)) {
                return nb::cast(affectedInterval);
            }
            return nb::none();
        }, nb::arg("time"))
        .def("CanBreakdown", &_WrapCanBreakdown,
             nb::arg("time"))

        .def("ClearKnots", &This::ClearKnots)
        .def("RemoveKnot", &_WrapRemoveKnot)

        .def_static("GetAntiRegressionAuthoringMode",
            &This::GetAntiRegressionAuthoringMode)
        .def("HasRegressiveTangents", &This::HasRegressiveTangents)
        .def("AdjustRegressiveTangents", &This::AdjustRegressiveTangents)

        .def("Eval", &_WrapEval)
        .def("EvalPreValue", &_WrapEvalPreValue)
        .def("EvalDerivative", &_WrapEvalDerivative)
        .def("EvalPreDerivative", &_WrapEvalPreDerivative)
        .def("EvalHeld", &_WrapEvalHeld)
        .def("EvalPreValueHeld", &_WrapEvalPreValueHeld)

        .def("Sample", &_WrapSample,
             nb::arg("timeInterval"),
             nb::arg("timeScale"),
             nb::arg("valueScale"),
             nb::arg("tolerance"),
             nb::arg("withSources") = false)

        .def("Diff", &_WrapDiff,
             nb::arg("other"),
             nb::arg("compareInterval") = GfInterval::GetFullInterval())

        .def("BakeInnerLoops", &This::BakeInnerLoops)
        .def("GetKnotsWithInnerLoopsBaked", &This::GetKnotsWithInnerLoopsBaked)

        .def("GetKnotsWithLoopsBaked", &This::GetKnotsWithLoopsBaked,
             nb::arg("interval"))

        .def("DoSidesDiffer", &This::DoSidesDiffer)

        .def("IsEmpty", &This::IsEmpty)
        .def("HasValueBlocks", &This::HasValueBlocks)
        .def("HasLoops", &This::HasLoops)

        .def("HasInnerLoops", &This::HasInnerLoops)

        .def("HasExtrapolatingLoops", &This::HasExtrapolatingLoops)

        .def("HasValueBlockAtTime", &This::HasValueBlockAtTime)

        .def_static("IsSupportedValueType",
            &This::IsSupportedValueType)

        .def("GetTruncated",
             &This::GetTruncated)
        .def("GetTimeScaled",
             &This::GetTimeScaled)
        .def_static("Concatenate",
             &This::Concatenate)
        ;
    ARCH_PRAGMA_POP

    _CanBreakdownResult::WrapNb<_CanBreakdownResult>(m, "_CanBreakdownResult",
                                                     "reason");
    VtValueFromPython<TsSpline>();
}
