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
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/external/boost/python.hpp"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateProjectionAttr(UsdGeomCamera &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateProjectionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateHorizontalApertureAttr(UsdGeomCamera &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateHorizontalApertureAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateVerticalApertureAttr(UsdGeomCamera &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVerticalApertureAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateHorizontalApertureOffsetAttr(UsdGeomCamera &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateHorizontalApertureOffsetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateVerticalApertureOffsetAttr(UsdGeomCamera &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateVerticalApertureOffsetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateFocalLengthAttr(UsdGeomCamera &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFocalLengthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateClippingRangeAttr(UsdGeomCamera &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateClippingRangeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}
        
static UsdAttribute
_CreateClippingPlanesAttr(UsdGeomCamera &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateClippingPlanesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float4Array), writeSparsely);
}
        
static UsdAttribute
_CreateFStopAttr(UsdGeomCamera &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFStopAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateFocusDistanceAttr(UsdGeomCamera &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFocusDistanceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateStereoRoleAttr(UsdGeomCamera &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateStereoRoleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateShutterOpenAttr(UsdGeomCamera &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateShutterOpenAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateShutterCloseAttr(UsdGeomCamera &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateShutterCloseAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateExposureAttr(UsdGeomCamera &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateExposureAttr(
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

void wrapUsdGeomCamera()
{
    typedef UsdGeomCamera This;

    class_<This, bases<UsdGeomXformable> >
        cls("Camera");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("Define", &This::Define, (arg("stage"), arg("path")))
        .staticmethod("Define")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetProjectionAttr",
             &This::GetProjectionAttr)
        .def("CreateProjectionAttr",
             &_CreateProjectionAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetHorizontalApertureAttr",
             &This::GetHorizontalApertureAttr)
        .def("CreateHorizontalApertureAttr",
             &_CreateHorizontalApertureAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVerticalApertureAttr",
             &This::GetVerticalApertureAttr)
        .def("CreateVerticalApertureAttr",
             &_CreateVerticalApertureAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetHorizontalApertureOffsetAttr",
             &This::GetHorizontalApertureOffsetAttr)
        .def("CreateHorizontalApertureOffsetAttr",
             &_CreateHorizontalApertureOffsetAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetVerticalApertureOffsetAttr",
             &This::GetVerticalApertureOffsetAttr)
        .def("CreateVerticalApertureOffsetAttr",
             &_CreateVerticalApertureOffsetAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFocalLengthAttr",
             &This::GetFocalLengthAttr)
        .def("CreateFocalLengthAttr",
             &_CreateFocalLengthAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetClippingRangeAttr",
             &This::GetClippingRangeAttr)
        .def("CreateClippingRangeAttr",
             &_CreateClippingRangeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetClippingPlanesAttr",
             &This::GetClippingPlanesAttr)
        .def("CreateClippingPlanesAttr",
             &_CreateClippingPlanesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFStopAttr",
             &This::GetFStopAttr)
        .def("CreateFStopAttr",
             &_CreateFStopAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFocusDistanceAttr",
             &This::GetFocusDistanceAttr)
        .def("CreateFocusDistanceAttr",
             &_CreateFocusDistanceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetStereoRoleAttr",
             &This::GetStereoRoleAttr)
        .def("CreateStereoRoleAttr",
             &_CreateStereoRoleAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetShutterOpenAttr",
             &This::GetShutterOpenAttr)
        .def("CreateShutterOpenAttr",
             &_CreateShutterOpenAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetShutterCloseAttr",
             &This::GetShutterCloseAttr)
        .def("CreateShutterCloseAttr",
             &_CreateShutterCloseAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetExposureAttr",
             &This::GetExposureAttr)
        .def("CreateExposureAttr",
             &_CreateExposureAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

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
             (arg("time") = UsdTimeCode::Default()))
        .def("SetFromCamera", &UsdGeomCamera::SetFromCamera,
             (arg("camera"),
              arg("time") = UsdTimeCode::Default()))
    ;
}

} // anonymous namespace 
