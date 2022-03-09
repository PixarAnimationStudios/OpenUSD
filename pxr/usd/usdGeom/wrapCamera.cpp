//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usd/usdGeom/camera.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>


PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateProjectionAttr(UsdGeomCamera &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateProjectionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateHorizontalApertureAttr(UsdGeomCamera &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateHorizontalApertureAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateVerticalApertureAttr(UsdGeomCamera &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateVerticalApertureAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateHorizontalApertureOffsetAttr(UsdGeomCamera &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateHorizontalApertureOffsetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateVerticalApertureOffsetAttr(UsdGeomCamera &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateVerticalApertureOffsetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateFocalLengthAttr(UsdGeomCamera &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateFocalLengthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateClippingRangeAttr(UsdGeomCamera &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateClippingRangeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}
        
static UsdAttribute
_CreateClippingPlanesAttr(UsdGeomCamera &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateClippingPlanesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float4Array), writeSparsely);
}
        
static UsdAttribute
_CreateFStopAttr(UsdGeomCamera &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateFStopAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateFocusDistanceAttr(UsdGeomCamera &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateFocusDistanceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateStereoRoleAttr(UsdGeomCamera &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateStereoRoleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateShutterOpenAttr(UsdGeomCamera &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateShutterOpenAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateShutterCloseAttr(UsdGeomCamera &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateShutterCloseAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateExposureAttr(UsdGeomCamera &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
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

    boost::python::class_<This, boost::python::bases<UsdGeomXformable> >
        cls("Camera");

    cls
        .def(boost::python::init<UsdPrim>(boost::python::arg("prim")))
        .def(boost::python::init<UsdSchemaBase const&>(boost::python::arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Get")

        .def("Define", &This::Define, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Define")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             boost::python::arg("includeInherited")=true,
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!boost::python::self)

        
        .def("GetProjectionAttr",
             &This::GetProjectionAttr)
        .def("CreateProjectionAttr",
             &_CreateProjectionAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetHorizontalApertureAttr",
             &This::GetHorizontalApertureAttr)
        .def("CreateHorizontalApertureAttr",
             &_CreateHorizontalApertureAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetVerticalApertureAttr",
             &This::GetVerticalApertureAttr)
        .def("CreateVerticalApertureAttr",
             &_CreateVerticalApertureAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetHorizontalApertureOffsetAttr",
             &This::GetHorizontalApertureOffsetAttr)
        .def("CreateHorizontalApertureOffsetAttr",
             &_CreateHorizontalApertureOffsetAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetVerticalApertureOffsetAttr",
             &This::GetVerticalApertureOffsetAttr)
        .def("CreateVerticalApertureOffsetAttr",
             &_CreateVerticalApertureOffsetAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetFocalLengthAttr",
             &This::GetFocalLengthAttr)
        .def("CreateFocalLengthAttr",
             &_CreateFocalLengthAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetClippingRangeAttr",
             &This::GetClippingRangeAttr)
        .def("CreateClippingRangeAttr",
             &_CreateClippingRangeAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetClippingPlanesAttr",
             &This::GetClippingPlanesAttr)
        .def("CreateClippingPlanesAttr",
             &_CreateClippingPlanesAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetFStopAttr",
             &This::GetFStopAttr)
        .def("CreateFStopAttr",
             &_CreateFStopAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetFocusDistanceAttr",
             &This::GetFocusDistanceAttr)
        .def("CreateFocusDistanceAttr",
             &_CreateFocusDistanceAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetStereoRoleAttr",
             &This::GetStereoRoleAttr)
        .def("CreateStereoRoleAttr",
             &_CreateStereoRoleAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetShutterOpenAttr",
             &This::GetShutterOpenAttr)
        .def("CreateShutterOpenAttr",
             &_CreateShutterOpenAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetShutterCloseAttr",
             &This::GetShutterCloseAttr)
        .def("CreateShutterCloseAttr",
             &_CreateShutterCloseAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetExposureAttr",
             &This::GetExposureAttr)
        .def("CreateExposureAttr",
             &_CreateExposureAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))

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
             (boost::python::arg("time") = UsdTimeCode::Default()))
        .def("SetFromCamera", &UsdGeomCamera::SetFromCamera,
             (boost::python::arg("camera"),
              boost::python::arg("time") = UsdTimeCode::Default()))
    ;
}

} // anonymous namespace 
