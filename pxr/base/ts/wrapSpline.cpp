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
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/vt/valueFromPython.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/make_constructor.hpp"
#include "pxr/external/boost/python/operators.hpp"

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

// There are several places where the API takes a type or type name. _ExtractTfType
// unifies them an makes them consistent.
//
// Return true if typeObj is a string or a TfType object. Set resultType to the
// resulting type (the unknown type if the type is not a supporrted spline value
// type). If resultType is the unknown type then reason is populated with the
// reason that the type could not be processed. If typeObj is something else
// entirely (a float, a Ts.Spline, etc.) then return false and do not modify
// either resultType or reason.
static bool _ExtractTfType(const object& typeObj,
                           TfType* resultType,
                           std::string* reason)
{
    bool looksLikeAType = false;
    
    extract<std::string> stringExtractor(typeObj);
    if (stringExtractor.check()) {
        looksLikeAType = true;
        
        const std::string typeName = stringExtractor();
        *resultType = Ts_GetTypeFromTypeName(typeName);

        if (!(*resultType)) {
            *reason = TfStringPrintf("Invalid spline type name '%s'",
                                     typeName.c_str());
        }
    } else {
        extract<TfType> typeExtractor(typeObj);
        if (typeExtractor.check()) {
            looksLikeAType = true;
            
            *resultType = typeExtractor();

            if (!TsSpline::IsSupportedValueType(*resultType)) {
                *reason = TfStringPrintf("Invalid spline type '%s'",
                                         resultType->GetTypeName().c_str());
                *resultType = TfType::GetUnknownType();
            }
        }
    }

    // Did we find something that looks like a type?
    return looksLikeAType;
}

static TsSpline* _WrapInit(
    const object &initArg)
{
    // Constructed with None? (Note that None is the default arg.)
    if (initArg.is_none()) {
        return new TsSpline();
    }

    // Copy constructor?
    extract<TsSpline> splineExtractor(initArg);
    if (splineExtractor.check()) {
        const TsSpline otherSpline = splineExtractor();
        return new TsSpline(otherSpline);
    }

    // Called with a type?
    TfType tfType;
    std::string reason;

    if (_ExtractTfType(initArg, &tfType, &reason)) {
        // The argument looked like a type (_ExtractTfType returned true) but is
        // it a supported type?
        if (tfType) {
            // yes
            return new TsSpline(tfType);
        }
        // no, the error message is in reason;
        TfPyThrowValueError(reason);
    } else {
        // The constructor was called with the wrong type!
        TfPyThrowTypeError("Invalid spline initialization argument. Expected"
                           " None, type name string, Tf.Type, or Ts.Spline.");
    }

    return nullptr;
}

static std::string _WrapStr(
    const TsSpline &spline)
{
    return TfStringify(spline);
}

static bool
_WrapSetValueType(
    TsSpline& spline, object valueType)
{
    TfType tfType;
    std::string reason;

    if (_ExtractTfType(valueType, &tfType, &reason)) {
        if (tfType) {
            return spline.SetValueType(tfType);
        }
        // Not a valid valueType, the error message is in reason.
        TfPyThrowValueError(reason);
    }

    TfPyThrowTypeError("Invalid argument type for SetValueType."
                       " A type name or Tf.Type is expected.");
    return false;
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

// An bool annotated by a string and returned from various Can* methods.
struct _TestResult: public TfPyAnnotatedBoolResult<std::string>
{
    _TestResult(bool val, const std::string reason)
    : TfPyAnnotatedBoolResult<std::string>(val, reason)
    {}
};

static
_TestResult
_WrapCanSetValueType(
    TsSpline& spline, object valueType)
{
    std::string reason;
    TfType tfType;

    if (_ExtractTfType(valueType, &tfType, &reason)) {
        if (tfType) {
            bool result = spline.CanSetValueType(tfType, &reason);
            return _TestResult(result, reason);
        }
        // reason has been populated by _ExtractTfType
    } else {
        // We don't want to raise type errors in a query method. Just say no.
        reason = "Invalid argument type for valueType. A type name or"
                 " Tf.Type is expected.";
    }
    return _TestResult(false, reason);
}

static
_TestResult
_WrapCanSetKnot(
    TsSpline& spline, const TsKnot& knot)
{
    std::string reason;
    bool result = spline.CanSetKnot(knot, &reason);
    return _TestResult(result, reason);
}

static
_TestResult
_WrapCanBreakdown(
    TsSpline& spline, const TsTime time)
{
    std::string reason;
    bool result = spline.CanBreakdown(time, &reason);
    return _TestResult(result, reason);
}

#define WRAP_EVAL(method)                                   \
    static object _Wrap##method(                            \
        const TsSpline &spline, const TsTime time)          \
    {                                                       \
        VtValue val;                                        \
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

static GfInterval _WrapDiff(
    const TsSpline& self,
    const TsSpline& other,
    const GfInterval& compareInterval)
{
    return self.Diff(other, compareInterval);
}

void wrapSpline()
{
    using This = TsSpline;

    // Convert a Python sequence of TsSpline objects to a std::vector<TsSpline>
    TfPyContainerConversions::from_python_sequence<
        std::vector<TsSpline>,
        TfPyContainerConversions::variable_capacity_policy >();

    // Do not generate compiler warnings for the deprecated inner looping
    // methods
    ARCH_PRAGMA_PUSH
    ARCH_PRAGMA_DEPRECATED
    class_<This>("Spline", no_init)

        .def("__init__",
            make_constructor(
                &_WrapInit,
                default_call_policies(),
                (arg("typeName") = object())))
        .def(init<const TsSpline &>())

        .def(self == self)
        .def(self != self)

        .def("__str__", &_WrapStr)

        .def("CanSetValueType", &_WrapCanSetValueType,
             arg("valueType"))
        .def("SetValueType", &_WrapSetValueType,
             arg("valueType"))
        .def("GetValueTypeName", &_WrapGetValueTypeName)
        .def("GetValueType", &This::GetValueType)
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
        .def("CanSetKnot", &_WrapCanSetKnot,
             arg("knot"))
        .def("SetKnot", &_WrapSetKnot)
        .def("GetKnots", &_WrapGetKnots)
        .def("GetKnots", &_WrapGetKnots_WithInterval)
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

        .def("Diff", &_WrapDiff,
             (arg("other"),
              arg("compareInterval") = GfInterval::GetFullInterval()))

        .def("BakeInnerLoops", &This::BakeInnerLoops)
        .def("GetKnotsWithInnerLoopsBaked", &This::GetKnotsWithInnerLoopsBaked)

        .def("GetKnotsWithLoopsBaked", &This::GetKnotsWithLoopsBaked,
             arg("interval"))

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

        .def("GetTruncated",
             &This::GetTruncated)
        .def("GetTimeScaled",
             &This::GetTimeScaled)
        .def("Concatenate",
             &This::Concatenate)
        .staticmethod("Concatenate")
        ;
    ARCH_PRAGMA_POP

    _TestResult::Wrap<_TestResult>("_TestResult", "reason");
    VtValueFromPython<TsSpline>();
}
