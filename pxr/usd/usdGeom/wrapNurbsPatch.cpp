//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/nurbsPatch.h"
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
_CreateUVertexCountAttr(UsdGeomNurbsPatch &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateUVertexCountAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateVVertexCountAttr(UsdGeomNurbsPatch &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateVVertexCountAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateUOrderAttr(UsdGeomNurbsPatch &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateUOrderAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateVOrderAttr(UsdGeomNurbsPatch &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateVOrderAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateUKnotsAttr(UsdGeomNurbsPatch &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateUKnotsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->DoubleArray), writeSparsely);
}
        
static UsdAttribute
_CreateVKnotsAttr(UsdGeomNurbsPatch &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateVKnotsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->DoubleArray), writeSparsely);
}
        
static UsdAttribute
_CreateUFormAttr(UsdGeomNurbsPatch &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateUFormAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateVFormAttr(UsdGeomNurbsPatch &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateVFormAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateURangeAttr(UsdGeomNurbsPatch &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateURangeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double2), writeSparsely);
}
        
static UsdAttribute
_CreateVRangeAttr(UsdGeomNurbsPatch &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateVRangeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double2), writeSparsely);
}
        
static UsdAttribute
_CreatePointWeightsAttr(UsdGeomNurbsPatch &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreatePointWeightsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->DoubleArray), writeSparsely);
}
        
static UsdAttribute
_CreateTrimCurveCountsAttr(UsdGeomNurbsPatch &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateTrimCurveCountsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateTrimCurveOrdersAttr(UsdGeomNurbsPatch &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateTrimCurveOrdersAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateTrimCurveVertexCountsAttr(UsdGeomNurbsPatch &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateTrimCurveVertexCountsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->IntArray), writeSparsely);
}
        
static UsdAttribute
_CreateTrimCurveKnotsAttr(UsdGeomNurbsPatch &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateTrimCurveKnotsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->DoubleArray), writeSparsely);
}
        
static UsdAttribute
_CreateTrimCurveRangesAttr(UsdGeomNurbsPatch &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateTrimCurveRangesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double2Array), writeSparsely);
}
        
static UsdAttribute
_CreateTrimCurvePointsAttr(UsdGeomNurbsPatch &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateTrimCurvePointsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double3Array), writeSparsely);
}

static std::string
_Repr(const UsdGeomNurbsPatch &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.NurbsPatch(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomNurbsPatch(nb::module_ &m)
{
    typedef UsdGeomNurbsPatch This;

    nb::class_<This, UsdGeomPointBased> cls(m, "NurbsPatch");

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

        
        .def("GetUVertexCountAttr",
             &This::GetUVertexCountAttr)
        .def("CreateUVertexCountAttr",
             &_CreateUVertexCountAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetVVertexCountAttr",
             &This::GetVVertexCountAttr)
        .def("CreateVVertexCountAttr",
             &_CreateVVertexCountAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetUOrderAttr",
             &This::GetUOrderAttr)
        .def("CreateUOrderAttr",
             &_CreateUOrderAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetVOrderAttr",
             &This::GetVOrderAttr)
        .def("CreateVOrderAttr",
             &_CreateVOrderAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetUKnotsAttr",
             &This::GetUKnotsAttr)
        .def("CreateUKnotsAttr",
             &_CreateUKnotsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetVKnotsAttr",
             &This::GetVKnotsAttr)
        .def("CreateVKnotsAttr",
             &_CreateVKnotsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetUFormAttr",
             &This::GetUFormAttr)
        .def("CreateUFormAttr",
             &_CreateUFormAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetVFormAttr",
             &This::GetVFormAttr)
        .def("CreateVFormAttr",
             &_CreateVFormAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetURangeAttr",
             &This::GetURangeAttr)
        .def("CreateURangeAttr",
             &_CreateURangeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetVRangeAttr",
             &This::GetVRangeAttr)
        .def("CreateVRangeAttr",
             &_CreateVRangeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetPointWeightsAttr",
             &This::GetPointWeightsAttr)
        .def("CreatePointWeightsAttr",
             &_CreatePointWeightsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetTrimCurveCountsAttr",
             &This::GetTrimCurveCountsAttr)
        .def("CreateTrimCurveCountsAttr",
             &_CreateTrimCurveCountsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetTrimCurveOrdersAttr",
             &This::GetTrimCurveOrdersAttr)
        .def("CreateTrimCurveOrdersAttr",
             &_CreateTrimCurveOrdersAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetTrimCurveVertexCountsAttr",
             &This::GetTrimCurveVertexCountsAttr)
        .def("CreateTrimCurveVertexCountsAttr",
             &_CreateTrimCurveVertexCountsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetTrimCurveKnotsAttr",
             &This::GetTrimCurveKnotsAttr)
        .def("CreateTrimCurveKnotsAttr",
             &_CreateTrimCurveKnotsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetTrimCurveRangesAttr",
             &This::GetTrimCurveRangesAttr)
        .def("CreateTrimCurveRangesAttr",
             &_CreateTrimCurveRangesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetTrimCurvePointsAttr",
             &This::GetTrimCurvePointsAttr)
        .def("CreateTrimCurvePointsAttr",
             &_CreateTrimCurvePointsAttr,
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

WRAP_CUSTOM {
}

} // anonymous namespace
