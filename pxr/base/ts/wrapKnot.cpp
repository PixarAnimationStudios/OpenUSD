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
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/pyUtils.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <memory>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

#define SET(knot, setter, type, obj)                            \
    if (!obj.is_none())                                         \
    {                                                           \
        type val;                                               \
        if (nb::try_cast<type>(obj, val))                       \
        {                                                       \
            knot->setter(val);                                  \
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
            const nb::handle &value,
            const nb::handle &preValue,
            const nb::handle &preTanSlope,
            const nb::handle &postTanSlope)
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
    const nb::handle &curveType,
    const nb::handle &time,
    const nb::handle &nextInterp,
    const nb::handle &value,
    const nb::handle &preValue,
    const nb::handle &customData,
    const nb::handle &preTanWidth,
    const nb::handle &preTanSlope,
    const nb::handle &postTanWidth,
    const nb::handle &postTanSlope,
    const nb::handle &preTanAlgorithm,
    const nb::handle &postTanAlgorithm)
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
    TsKnot& knot, nb::handle prevKnot, nb::handle nextKnot, TsCurveType curveType)
{
    std::optional<TsKnot> optPrevKnot, optNextKnot;

    if (!prevKnot.is_none()) {
        TsKnot k;
        if (nb::try_cast<TsKnot>(prevKnot, k))
        {
            optPrevKnot = k;
        }
        else
        {
            TfPyThrowTypeError("prevKnot must be a Ts.Knot or None");
            return false;
        }
    }

    if (!nextKnot.is_none()) {
        TsKnot k;
        if (nb::try_cast<TsKnot>(nextKnot, k))
        {
            optNextKnot = k;
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

void wrapKnot(nanobind::module_& m)
{
    using This = TsKnot;

    nb::class_<This>(m, "Knot")
        .def("__init__",
             [](This* self,
                const std::string& typeName,
                nb::handle curveType,
                nb::handle time,
                nb::handle nextInterp,
                nb::handle value,
                nb::handle preValue,
                nb::handle customData,
                nb::handle preTanWidth,
                nb::handle preTanSlope,
                nb::handle postTanWidth,
                nb::handle postTanSlope,
                nb::handle preTanAlgorithm,
                nb::handle postTanAlgorithm) {
                 std::unique_ptr<TsKnot> k(_WrapInit(
                     typeName,
                     curveType, time, nextInterp,
                     value, preValue, customData,
                     preTanWidth, preTanSlope,
                     postTanWidth, postTanSlope,
                     preTanAlgorithm, postTanAlgorithm));
                 if (k) {
                     new (self) TsKnot(std::move(*k));
                 }
             },
             nb::arg("typeName") = "double",
             nb::arg("curveType") = nb::none(),
             nb::arg("time") = nb::none(),
             nb::arg("nextInterp") = nb::none(),
             nb::arg("value") = nb::none(),
             nb::arg("preValue") = nb::none(),
             nb::arg("customData") = nb::none(),
             nb::arg("preTanWidth") = nb::none(),
             nb::arg("preTanSlope") = nb::none(),
             nb::arg("postTanWidth") = nb::none(),
             nb::arg("postTanSlope") = nb::none(),
             nb::arg("preTanAlgorithm") = nb::none(),
             nb::arg("postTanAlgorithm") = nb::none())

        .def(nb::init<const TsKnot &>())

        .def(nb::self == nb::self)
        .def(nb::self != nb::self)

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
             nb::arg("prevKnot").none(),
             nb::arg("nextKnot").none(),
             nb::arg("curveType") = TsCurveTypeBezier)

        ;
}
