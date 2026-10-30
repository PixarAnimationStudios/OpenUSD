//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/camera.h"
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
_CreateProjectionAttr(UsdGeomCamera &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateProjectionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateHorizontalApertureAttr(UsdGeomCamera &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateHorizontalApertureAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateVerticalApertureAttr(UsdGeomCamera &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateVerticalApertureAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateHorizontalApertureOffsetAttr(UsdGeomCamera &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateHorizontalApertureOffsetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateVerticalApertureOffsetAttr(UsdGeomCamera &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateVerticalApertureOffsetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateFocalLengthAttr(UsdGeomCamera &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateFocalLengthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateClippingRangeAttr(UsdGeomCamera &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateClippingRangeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}
        
static UsdAttribute
_CreateClippingPlanesAttr(UsdGeomCamera &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateClippingPlanesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float4Array), writeSparsely);
}
        
static UsdAttribute
_CreateFStopAttr(UsdGeomCamera &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateFStopAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateFocusDistanceAttr(UsdGeomCamera &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateFocusDistanceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateStereoRoleAttr(UsdGeomCamera &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateStereoRoleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateShutterOpenAttr(UsdGeomCamera &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateShutterOpenAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateShutterCloseAttr(UsdGeomCamera &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateShutterCloseAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateExposureAttr(UsdGeomCamera &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateExposureAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateExposureIsoAttr(UsdGeomCamera &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateExposureIsoAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateExposureTimeAttr(UsdGeomCamera &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateExposureTimeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateExposureFStopAttr(UsdGeomCamera &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateExposureFStopAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateExposureResponsivityAttr(UsdGeomCamera &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateExposureResponsivityAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}

static std::string
_Repr(const UsdGeomCamera &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.Camera(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomCamera(nb::module_ &m)
{
    typedef UsdGeomCamera This;

    nb::class_<This, UsdGeomXformable> cls(m, "Camera");

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

        
        .def("GetProjectionAttr",
             &This::GetProjectionAttr)
        .def("CreateProjectionAttr",
             &_CreateProjectionAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetHorizontalApertureAttr",
             &This::GetHorizontalApertureAttr)
        .def("CreateHorizontalApertureAttr",
             &_CreateHorizontalApertureAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetVerticalApertureAttr",
             &This::GetVerticalApertureAttr)
        .def("CreateVerticalApertureAttr",
             &_CreateVerticalApertureAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetHorizontalApertureOffsetAttr",
             &This::GetHorizontalApertureOffsetAttr)
        .def("CreateHorizontalApertureOffsetAttr",
             &_CreateHorizontalApertureOffsetAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetVerticalApertureOffsetAttr",
             &This::GetVerticalApertureOffsetAttr)
        .def("CreateVerticalApertureOffsetAttr",
             &_CreateVerticalApertureOffsetAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetFocalLengthAttr",
             &This::GetFocalLengthAttr)
        .def("CreateFocalLengthAttr",
             &_CreateFocalLengthAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetClippingRangeAttr",
             &This::GetClippingRangeAttr)
        .def("CreateClippingRangeAttr",
             &_CreateClippingRangeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetClippingPlanesAttr",
             &This::GetClippingPlanesAttr)
        .def("CreateClippingPlanesAttr",
             &_CreateClippingPlanesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetFStopAttr",
             &This::GetFStopAttr)
        .def("CreateFStopAttr",
             &_CreateFStopAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetFocusDistanceAttr",
             &This::GetFocusDistanceAttr)
        .def("CreateFocusDistanceAttr",
             &_CreateFocusDistanceAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetStereoRoleAttr",
             &This::GetStereoRoleAttr)
        .def("CreateStereoRoleAttr",
             &_CreateStereoRoleAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetShutterOpenAttr",
             &This::GetShutterOpenAttr)
        .def("CreateShutterOpenAttr",
             &_CreateShutterOpenAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetShutterCloseAttr",
             &This::GetShutterCloseAttr)
        .def("CreateShutterCloseAttr",
             &_CreateShutterCloseAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetExposureAttr",
             &This::GetExposureAttr)
        .def("CreateExposureAttr",
             &_CreateExposureAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetExposureIsoAttr",
             &This::GetExposureIsoAttr)
        .def("CreateExposureIsoAttr",
             &_CreateExposureIsoAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetExposureTimeAttr",
             &This::GetExposureTimeAttr)
        .def("CreateExposureTimeAttr",
             &_CreateExposureTimeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetExposureFStopAttr",
             &This::GetExposureFStopAttr)
        .def("CreateExposureFStopAttr",
             &_CreateExposureFStopAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetExposureResponsivityAttr",
             &This::GetExposureResponsivityAttr)
        .def("CreateExposureResponsivityAttr",
             &_CreateExposureResponsivityAttr,
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
    _class
        .def("GetCamera", &UsdGeomCamera::GetCamera,
             (nb::arg("time") = UsdTimeCode::Default()))
        .def("SetFromCamera", &UsdGeomCamera::SetFromCamera,
             nb::arg("camera"),
              nb::arg("time") = UsdTimeCode::Default())
        .def("ComputeLinearExposureScale",
             &UsdGeomCamera::ComputeLinearExposureScale,
             (nb::arg("time") = UsdTimeCode::Default()))
    ;
}

} // anonymous namespace 
