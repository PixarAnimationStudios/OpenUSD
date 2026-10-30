//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/xformCommonAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/wrapTypeHelpers.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/tuple.h>

namespace nb = nanobind;

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

static std::string
_Repr(const UsdGeomXformCommonAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.XformCommonAPI(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomXformCommonAPI(nb::module_ &m)
{
    typedef UsdGeomXformCommonAPI This;

    nb::class_<This, UsdAPISchemaBase> cls(m, "XformCommonAPI");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        .def("__repr__", ::_Repr)
    ;

    _CustomWrapCode(cls);
}

// ===================================================================== //
// Feel free to add custom code below this line, it will be preserved by 
// the code generator.  The entry point for your custom code should look
// minimally like the following:
//
// WRAP_CUSTOM {
//     _class
//         .def("MyCustomMethod", ...)
//     ;
// }
//
// Of course any other ancillary or support code may be provided.
// 
// Just remember to wrap code in the appropriate delimiters:
// 'namespace {', '}'.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/base/tf/pyEnum.h"

namespace {

static nb::tuple 
_GetXformVectors(
    UsdGeomXformCommonAPI self, 
    const UsdTimeCode &time)
{
    GfVec3d translation;
    GfVec3f rotation, scale, pivot;
    UsdGeomXformCommonAPI::RotationOrder rotationOrder;

    bool result = self.GetXformVectors(&translation, &rotation, &scale, 
                                       &pivot, &rotationOrder, time);

    return result ? nb::make_tuple(translation, rotation, scale, pivot, rotationOrder)
                  : nb::tuple();
}

static nb::tuple 
_GetXformVectorsByAccumulation(
    UsdGeomXformCommonAPI self, 
    const UsdTimeCode &time)
{
    GfVec3d translation;
    GfVec3f rotation, scale, pivot;
    UsdGeomXformCommonAPI::RotationOrder rotationOrder;

    bool result = self.GetXformVectorsByAccumulation(&translation, &rotation, &scale, 
                                                     &pivot, &rotationOrder, time);

    return result ? nb::make_tuple(translation, rotation, scale, pivot, rotationOrder)
                  : nb::tuple();
}

static nb::tuple
_CreateXformOps1(
    UsdGeomXformCommonAPI self,
    UsdGeomXformCommonAPI::RotationOrder rotOrder,
    UsdGeomXformCommonAPI::OpFlags op1,
    UsdGeomXformCommonAPI::OpFlags op2,
    UsdGeomXformCommonAPI::OpFlags op3,
    UsdGeomXformCommonAPI::OpFlags op4)
{
    UsdGeomXformCommonAPI::Ops ops = self.CreateXformOps(
        rotOrder, op1, op2, op3, op4);
    return nb::make_tuple(
        ops.translateOp,
        ops.pivotOp,
        ops.rotateOp,
        ops.scaleOp,
        ops.inversePivotOp);
}

static nb::tuple
_CreateXformOps2(
    UsdGeomXformCommonAPI self,
    UsdGeomXformCommonAPI::OpFlags op1,
    UsdGeomXformCommonAPI::OpFlags op2,
    UsdGeomXformCommonAPI::OpFlags op3,
    UsdGeomXformCommonAPI::OpFlags op4)
{
    UsdGeomXformCommonAPI::Ops ops = self.CreateXformOps(op1, op2, op3, op4);
    return nb::make_tuple(
        ops.translateOp,
        ops.pivotOp,
        ops.rotateOp,
        ops.scaleOp,
        ops.inversePivotOp);
}

WRAP_CUSTOM {
    using This = UsdGeomXformCommonAPI;

    {
        TfNbWrapEnum<This::RotationOrder>(_class);
        TfNbWrapEnum<This::OpFlags>(_class);
    }

    _class
        .def("SetXformVectors", &This::SetXformVectors,
            nb::arg("translation"),
             nb::arg("rotation"),
             nb::arg("scale"),
             nb::arg("pivot"),
             nb::arg("rotationOrder"),
             nb::arg("time"))

        .def("GetXformVectors", &_GetXformVectors, 
            nb::arg("time"))

        .def("GetXformVectorsByAccumulation", &_GetXformVectorsByAccumulation, 
            nb::arg("time"))

        .def("SetTranslate", &This::SetTranslate,
            nb::arg("translation"),
             nb::arg("time")=UsdTimeCode::Default())

        .def("SetPivot", &This::SetPivot,
            nb::arg("pivot"),
             nb::arg("time")=UsdTimeCode::Default())

        .def("SetRotate", &This::SetRotate,
            nb::arg("rotation"),
             nb::arg("rotationOrder")=This::RotationOrderXYZ,
             nb::arg("time")=UsdTimeCode::Default())

        .def("SetScale", &This::SetScale,
            nb::arg("scale"),
             nb::arg("time")=UsdTimeCode::Default())

        .def("GetResetXformStack", &This::GetResetXformStack)

        .def("SetResetXformStack", &This::SetResetXformStack,
            nb::arg("resetXformStack"))

        .def("CreateXformOps", _CreateXformOps1,
            nb::arg("rotationOrder"),
             nb::arg("op1")=UsdGeomXformCommonAPI::OpNone,
             nb::arg("op2")=UsdGeomXformCommonAPI::OpNone,
             nb::arg("op3")=UsdGeomXformCommonAPI::OpNone,
             nb::arg("op4")=UsdGeomXformCommonAPI::OpNone)

        .def("CreateXformOps", _CreateXformOps2,
            nb::arg("op1")=UsdGeomXformCommonAPI::OpNone,
             nb::arg("op2")=UsdGeomXformCommonAPI::OpNone,
             nb::arg("op3")=UsdGeomXformCommonAPI::OpNone,
             nb::arg("op4")=UsdGeomXformCommonAPI::OpNone)

        .def_static("GetRotationTransform", &This::GetRotationTransform,
            nb::arg("rotation"), nb::arg("rotationOrder"))

        .def_static("ConvertRotationOrderToOpType",
            &This::ConvertRotationOrderToOpType,
            nb::arg("rotationOrder"))

        .def_static("ConvertOpTypeToRotationOrder",
            &This::ConvertOpTypeToRotationOrder,
            nb::arg("opType"))

        .def_static("CanConvertOpTypeToRotationOrder",
            &This::CanConvertOpTypeToRotationOrder,
            nb::arg("opType"))
        ;
}

}
