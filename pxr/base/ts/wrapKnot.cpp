//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/knot.h"
#include "pxr/base/ts/types.h"
#include "pxr/base/ts/typeHelpers.h"
#include "pxr/base/ts/valueTypeDispatch.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/pyUtils.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/make_constructor.hpp"
#include "pxr/external/boost/python/operators.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;


#define SET(knot, setter, type, obj)                            \
    if (!obj.is_none())                                         \
    {                                                           \
        extract<type> extractor(obj);                           \
        if (extractor.check())                                  \
        {                                                       \
            knot->setter(extractor());                          \
        }                                                       \
        else                                                    \
        {                                                       \
            TfPyThrowTypeError(                                 \
                TfStringPrintf("Invalid type for '%s'", #obj)); \
        }                                                       \
    }

namespace
{
    template <typename T>
    struct _Initter
    {
        void operator()(
            TsKnot* const knot,
            const object &value,
            const object &preValue,
            const object &preTanSlope,
            const object &postTanSlope)
        {
            SET(knot, SetValue, T, value);
            SET(knot, SetPreValue, T, preValue);
            SET(knot, SetPreTanSlope, T, preTanSlope);
            SET(knot, SetPostTanSlope, T, postTanSlope);
        }
    };
}

static TsKnot* _WrapInit(
    const std::string &typeName,
    const object &curveType,
    const object &time,
    const object &nextInterp,
    const object &value,
    const object &preValue,
    const object &customData,
    const object &preTanWidth,
    const object &preTanSlope,
    const object &postTanWidth,
    const object &postTanSlope,
    const object &preTanAlgorithm,
    const object &postTanAlgorithm)
{
    const TfType valueType = Ts_GetTypeFromTypeName(typeName);
    if (!valueType)
    {
        TfPyThrowTypeError(
            TfStringPrintf("Invalid knot type name '%s'", typeName.c_str()));
        return nullptr;
    }

    TsKnot *knot = new TsKnot(valueType);

    // Set fixed-type parameters.
    SET(knot, SetCurveType, TsCurveType, curveType);  // deprecated
    SET(knot, SetTime, TsTime, time);
    SET(knot, SetNextInterpolation, TsInterpMode, nextInterp);
    SET(knot, SetCustomData, VtDictionary, customData);
    SET(knot, SetPreTanWidth, TsTime, preTanWidth);
    SET(knot, SetPostTanWidth, TsTime, postTanWidth);
    SET(knot, SetPreTanAlgorithm, TsTangentAlgorithm, preTanAlgorithm);
    SET(knot, SetPostTanAlgorithm, TsTangentAlgorithm, postTanAlgorithm);

    // Set T-typed parameters.
    TsDispatchToValueTypeTemplate<_Initter>(
        valueType, knot, value, preValue,
        preTanSlope, postTanSlope);

    return knot;
}

static bool _WrapUpdateTangents(
    TsKnot& knot, object prevKnot, object nextKnot, TsCurveType curveType)
{
    std::optional<TsKnot> optPrevKnot, optNextKnot;

    if (!prevKnot.is_none()) {
        extract<TsKnot> extractor(prevKnot);
        if (extractor.check())
        {
            optPrevKnot = extractor();
        }
        else
        {
            TfPyThrowTypeError("prevKnot must be a Ts.Knot or None");
            return false;
        }
    }

    if (!nextKnot.is_none()) {
        extract<TsKnot> extractor(nextKnot);
        if (extractor.check())
        {
            optNextKnot = extractor();
        }
        else
        {
            TfPyThrowTypeError("nextKnot must be a Ts.Knot or None");
            return false;
        }
    }

    return knot.UpdateTangents(optPrevKnot, optNextKnot, curveType);
}

static std::string _WrapGetValueTypeName(
    const TsKnot &knot)
{
    return Ts_GetTypeNameFromType(knot.GetValueType());
}


namespace
{
    template <typename T>
    struct _Bundler
    {
        void operator()(
            const double valueIn,
            VtValue* const valueOut)
        {
            *valueOut = VtValue(static_cast<T>(valueIn));
        }
    };
}

// For all spline value types, allow T-typed fields to be set from any Python
// arithmetic type.  This is because Python has no native floating-point types
// other than float, which maps to C++ double, and we need a way to set T-typed
// fields for spline types other than double.  This means that we are allowing
// narrowing conversions, which we do not allow in C++.
//
#define WRAP_SETTER(field)                           \
    +[](TsKnot &knot, const double value)            \
    {                                                \
        VtValue vt;                                  \
        TsDispatchToValueTypeTemplate<_Bundler>(     \
            knot.GetValueType(), value, &vt);        \
        knot.Set##field(vt);                         \
    }

// For all spline value types, return T-typed fields as VtValue.  These will
// convert to Python floats.
//
#define WRAP_GETTER(field)                           \
    +[](const TsKnot &knot)                          \
    {                                                \
        VtValue vt;                                  \
        knot.Get##field(&vt);                        \
        return vt;                                   \
    }


void wrapKnot()
{
    using This = TsKnot;

    class_<This>("Knot", no_init)
        .def("__init__", make_constructor(
                &_WrapInit,
                default_call_policies(),
                (arg("typeName") = "double",
                 arg("curveType") = object(),
                 arg("time") = object(),
                 arg("nextInterp") = object(),
                 arg("value") = object(),
                 arg("preValue") = object(),
                 arg("customData") = object(),
                 arg("preTanWidth") = object(),
                 arg("preTanSlope") = object(),
                 arg("postTanWidth") = object(),
                 arg("postTanSlope") = object(),
                 arg("preTanAlgorithm") = object(),
                 arg("postTanAlgorithm") = object())))

        .def(init<const TsKnot &>())

        .def(self == self)
        .def(self != self)

        .def("SetTime", &This::SetTime)
        .def("GetTime", &This::GetTime)

        .def("SetNextInterpolation", &This::SetNextInterpolation)
        .def("GetNextInterpolation", &This::GetNextInterpolation)

        .def("GetValueTypeName", &_WrapGetValueTypeName)
        .def("SetValue", WRAP_SETTER(Value))
        .def("GetValue", WRAP_GETTER(Value))

        .def("IsDualValued", &This::IsDualValued)
        .def("SetPreValue", WRAP_SETTER(PreValue))
        .def("GetPreValue", WRAP_GETTER(PreValue))
        .def("ClearPreValue", &This::ClearPreValue)

        .def("SetCurveType", &This::SetCurveType)       // deprecated
        .def("GetCurveType", &This::GetCurveType)       // deprecated

        .def("SetPreTanWidth", &This::SetPreTanWidth)
        .def("GetPreTanWidth", &This::GetPreTanWidth)
        .def("SetPreTanSlope", WRAP_SETTER(PreTanSlope))
        .def("GetPreTanSlope", WRAP_GETTER(PreTanSlope))
        .def("SetPreTanAlgorithm", &This::SetPreTanAlgorithm)
        .def("GetPreTanAlgorithm", &This::GetPreTanAlgorithm)

        .def("SetPostTanWidth", &This::SetPostTanWidth)
        .def("GetPostTanWidth", &This::GetPostTanWidth)
        .def("SetPostTanSlope", WRAP_SETTER(PostTanSlope))
        .def("GetPostTanSlope", WRAP_GETTER(PostTanSlope))
        .def("SetPostTanAlgorithm", &This::SetPostTanAlgorithm)
        .def("GetPostTanAlgorithm", &This::GetPostTanAlgorithm)

        .def("SetCustomData", &This::SetCustomData)
        .def("GetCustomData", &This::GetCustomData)
        .def("SetCustomDataByKey", &This::SetCustomDataByKey)
        .def("GetCustomDataByKey", &This::GetCustomDataByKey)

        .def("UpdateTangents", &_WrapUpdateTangents,
             (arg("prevKnot"),
              arg("nextKnot"),
              arg("curveType") = TsCurveTypeBezier))

        ;
}
