//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/pointBased.h"
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

        
static UsdAttribute
_CreatePointsAttr(UsdGeomPointBased &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreatePointsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateVelocitiesAttr(UsdGeomPointBased &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateVelocitiesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateAccelerationsAttr(UsdGeomPointBased &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateAccelerationsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateNormalsAttr(UsdGeomPointBased &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateNormalsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Normal3fArray), writeSparsely);
}

static std::string
_Repr(const UsdGeomPointBased &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.PointBased(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomPointBased(nb::module_ &m)
{
    typedef UsdGeomPointBased This;

    nb::class_<This, UsdGeomGprim> cls(m, "PointBased");

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

        
        .def("GetPointsAttr",
             &This::GetPointsAttr)
        .def("CreatePointsAttr",
             &_CreatePointsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetVelocitiesAttr",
             &This::GetVelocitiesAttr)
        .def("CreateVelocitiesAttr",
             &_CreateVelocitiesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetAccelerationsAttr",
             &This::GetAccelerationsAttr)
        .def("CreateAccelerationsAttr",
             &_CreateAccelerationsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetNormalsAttr",
             &This::GetNormalsAttr)
        .def("CreateNormalsAttr",
             &_CreateNormalsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

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
namespace {

static TfPyObjWrapper 
_ComputeExtent(TfPyObjWrapper points) {

    // Convert from python objects to VtValue
    VtVec3fArray extent;
    VtValue pointsAsVtValue = UsdPythonToSdfType(points, 
        SdfValueTypeNames->Float3Array);

    // Check for proper conversion to VtVec3fArray
    if (!pointsAsVtValue.IsHolding<VtVec3fArray>()) {
        TF_CODING_ERROR("Improper value for 'points'");
        return TfPyObjWrapper();
    }

    // Convert from VtValue to VtVec3fArray
    VtVec3fArray pointsArray = pointsAsVtValue.UncheckedGet<VtVec3fArray>();
    if (UsdGeomPointBased::ComputeExtent(pointsArray, &extent)) {
        return UsdVtValueToPython(VtValue(extent));
    } else {
        return TfPyObjWrapper();
    }
}

static
VtVec3fArray
_ComputePointsAtTime(
    const UsdGeomPointBased& self,
    const UsdTimeCode time,
    const UsdTimeCode baseTime)
{
    VtVec3fArray points;

    // On error we'll be returning an empty array.
    self.ComputePointsAtTime(&points, time, baseTime);

    return points;
}

static
std::vector<VtVec3fArray>
_ComputePointsAtTimes(
    const UsdGeomPointBased& self,
    const std::vector<UsdTimeCode>& times,
    const UsdTimeCode baseTime)
{
    std::vector<VtVec3fArray> points;

    // On error we'll be returning an empty array.
    self.ComputePointsAtTimes(&points, times, baseTime);

    return points;
}

WRAP_CUSTOM {
    _class
        .def("GetNormalsInterpolation",
             &UsdGeomPointBased::GetNormalsInterpolation)
        .def("SetNormalsInterpolation",
             &UsdGeomPointBased::SetNormalsInterpolation,
             nb::arg("interpolation"))

        .def("ComputePointsAtTime",
             &_ComputePointsAtTime,
             nb::arg("time"), nb::arg("baseTime"))
        .def("ComputePointsAtTimes",
             &_ComputePointsAtTimes,
             nb::arg("times"), nb::arg("baseTime"))

        ;
    _class.attr("ComputeExtent") = nb::cpp_function(
        &_ComputeExtent,
        nb::name("ComputeExtent"),
        nb::arg("points"));
}

} // anonymous namespace
