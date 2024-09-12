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
            TF_CODING_ERROR("Unexpected type for '%s'", #obj);  \
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
            const object &preTanMayaHeight,
            const object &postTanSlope,
            const object &postTanMayaHeight)
        {
            SET(knot, SetValue, T, value);
            SET(knot, SetPreValue, T, preValue);
            SET(knot, SetPreTanSlope, T, preTanSlope);
            SET(knot, SetMayaPreTanHeight, T, preTanMayaHeight);
            SET(knot, SetPostTanSlope, T, postTanSlope);
            SET(knot, SetMayaPostTanHeight, T, postTanMayaHeight);
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
    const object &preTanMayaWidth,
    const object &preTanSlope,
    const object &preTanMayaHeight,
    const object &postTanWidth,
    const object &postTanMayaWidth,
    const object &postTanSlope,
    const object &postTanMayaHeight)
{
    const TfType valueType = Ts_GetTypeFromTypeName(typeName);
    if (!valueType)
    {
        TF_CODING_ERROR("Invalid knot type name '%s'", typeName.c_str());
        return nullptr;
    }

    // Python-owned knots are always double-typed.
    TsKnot *knot = new TsKnot(valueType);

    // Set fixed-type parameters.
    SET(knot, SetCurveType, TsCurveType, curveType);
    SET(knot, SetTime, TsTime, time);
    SET(knot, SetNextInterpolation, TsInterpMode, nextInterp);
    SET(knot, SetCustomData, VtDictionary, customData);
    SET(knot, SetPreTanWidth, TsTime, preTanWidth);
    SET(knot, SetMayaPreTanWidth, TsTime, preTanMayaWidth);
    SET(knot, SetPostTanWidth, TsTime, postTanWidth);
    SET(knot, SetMayaPostTanWidth, TsTime, postTanMayaWidth);

    // Set T-typed parameters.
    TsDispatchToValueTypeTemplate<_Initter>(
        valueType, knot,
        value, preValue,
        preTanSlope, preTanMayaHeight,
        postTanSlope, postTanMayaHeight);

    return knot;
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
                 arg("preTanMayaWidth") = object(),
                 arg("preTanSlope") = object(),
                 arg("preTanMayaHeight") = object(),
                 arg("postTanWidth") = object(),
                 arg("postTanMayaWidth") = object(),
                 arg("postTanSlope") = object(),
                 arg("postTanMayaHeight") = object())))

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

        .def("SetCurveType", &This::SetCurveType)
        .def("GetCurveType", &This::GetCurveType)

        .def("IsPreTanMayaForm", &This::IsPreTanMayaForm)
        .def("SetPreTanWidth", &This::SetPreTanWidth)
        .def("GetPreTanWidth", &This::GetPreTanWidth)
        .def("SetMayaPreTanWidth", &This::SetMayaPreTanWidth)
        .def("GetMayaPreTanWidth", &This::GetMayaPreTanWidth)
        .def("SetPreTanSlope", WRAP_SETTER(PreTanSlope))
        .def("GetPreTanSlope", WRAP_GETTER(PreTanSlope))
        .def("SetMayaPreTanHeight", WRAP_SETTER(MayaPreTanHeight))
        .def("GetMayaPreTanHeight", WRAP_GETTER(MayaPreTanHeight))

        .def("IsPostTanMayaForm", &This::IsPostTanMayaForm)
        .def("SetPostTanWidth", &This::SetPostTanWidth)
        .def("GetPostTanWidth", &This::GetPostTanWidth)
        .def("SetMayaPostTanWidth", &This::SetMayaPostTanWidth)
        .def("GetMayaPostTanWidth", &This::GetMayaPostTanWidth)
        .def("SetPostTanSlope", WRAP_SETTER(PostTanSlope))
        .def("GetPostTanSlope", WRAP_GETTER(PostTanSlope))
        .def("SetMayaPostTanHeight", WRAP_SETTER(MayaPostTanHeight))
        .def("GetMayaPostTanHeight", WRAP_GETTER(MayaPostTanHeight))

        .def("SetCustomData", &This::SetCustomData)
        .def("GetCustomData", &This::GetCustomData)
        .def("SetCustomDataByKey", &This::SetCustomDataByKey)
        .def("GetCustomDataByKey", &This::GetCustomDataByKey)

        ;
}
