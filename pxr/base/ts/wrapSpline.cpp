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
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/diagnostic.h"

#include <boost/python/class.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/operators.hpp>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace boost::python;


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

        .def("DoSidesDiffer", &This::DoSidesDiffer)

        .def("IsEmpty", &This::IsEmpty)
        .def("HasValueBlocks", &This::HasValueBlocks)
        .def("HasLoops", &This::HasLoops)
        .def("HasInnerLoops", &This::HasInnerLoops)
        .def("HasExtrapolatingLoops", &This::HasExtrapolatingLoops)

        .def("HasValueBlockAtTime", &This::HasValueBlockAtTime)

        ;
}
