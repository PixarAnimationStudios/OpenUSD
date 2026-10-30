//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/points.h"
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
_CreateWidthsAttr(UsdGeomPoints &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateWidthsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}
        
static UsdAttribute
_CreateIdsAttr(UsdGeomPoints &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateIdsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int64Array), writeSparsely);
}

static std::string
_Repr(const UsdGeomPoints &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.Points(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomPoints(nb::module_ &m)
{
    typedef UsdGeomPoints This;

    nb::class_<This, UsdGeomPointBased> cls(m, "Points");

    cls
        .def(nb::init<UsdPrim>(), nb::arg("prim")=UsdPrim())
        .def(nb::init<UsdSchemaBase const&>(), nb::arg("schemaObj"))
        .def(TfTypePythonClass())

        .def_static("Get", &This::Get, nb::arg("stage"), nb::arg("path"))

        .def_static("Define", &This::Define, nb::arg("stage"), nb::arg("path"))

        .def_static("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetWidthsAttr",
             &This::GetWidthsAttr)
        .def("CreateWidthsAttr",
             &_CreateWidthsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetIdsAttr",
             &This::GetIdsAttr)
        .def("CreateIdsAttr",
             &_CreateIdsAttr,
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
_ComputeExtent(TfPyObjWrapper points, TfPyObjWrapper widths) {
  
    // Convert from python objects to VtValue
    VtVec3fArray extent;
    VtValue pointsAsVtValue = UsdPythonToSdfType(points, 
        SdfValueTypeNames->Float3Array);
    VtValue widthsAsVtValue = UsdPythonToSdfType(widths, 
        SdfValueTypeNames->FloatArray);

    // Check Proper conversion to VtVec3fArray
    if (!pointsAsVtValue.IsHolding<VtVec3fArray>()) {
        TF_CODING_ERROR("Improper value for 'points'");
        return TfPyObjWrapper();
    }

    if (!widthsAsVtValue.IsHolding<VtFloatArray>()) {
        TF_CODING_ERROR("Improper value for 'widths'");
        return TfPyObjWrapper();
    }

    // Convert from VtValue to VtVec3fArray
    VtVec3fArray pointsArray = pointsAsVtValue.UncheckedGet<VtVec3fArray>();
    VtFloatArray widthsArray = widthsAsVtValue.UncheckedGet<VtFloatArray>();

    if (UsdGeomPoints::ComputeExtent(pointsArray, widthsArray, &extent)) {
        return UsdVtValueToPython(VtValue(extent));
    } else {
        return TfPyObjWrapper();
    }
}

WRAP_CUSTOM {
    _class
        .def("GetWidthsInterpolation", &UsdGeomPoints::GetWidthsInterpolation)
        .def("SetWidthsInterpolation", &UsdGeomPoints::SetWidthsInterpolation,
             nb::arg("interpolation"))

        .def("GetPointCount", &UsdGeomPoints::GetPointCount,
            nb::arg("timeCode")=UsdTimeCode::Default())

        ;
    _class.attr("ComputeExtent") = nb::cpp_function(
        &_ComputeExtent,
        nb::name("ComputeExtent"),
        nb::arg("points"), nb::arg("widths"));
}

} // anonymous namespace
