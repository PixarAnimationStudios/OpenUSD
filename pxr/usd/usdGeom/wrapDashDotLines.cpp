//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/dashDotLines.h"
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
_CreateShapeDetailAttr(UsdGeomDashDotLines &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateShapeDetailAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateScreenSpacePatternAttr(UsdGeomDashDotLines &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateScreenSpacePatternAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreatePatternScaleAttr(UsdGeomDashDotLines &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePatternScaleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateStartCapTypeAttr(UsdGeomDashDotLines &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateStartCapTypeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateEndCapTypeAttr(UsdGeomDashDotLines &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateEndCapTypeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateJointTypeAttr(UsdGeomDashDotLines &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateJointTypeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdGeomDashDotLines &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdGeom.DashDotLines(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdGeomDashDotLines()
{
    typedef UsdGeomDashDotLines This;

    class_<This, bases<UsdGeomCurves> >
        cls("DashDotLines");

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

        
        .def("GetShapeDetailAttr",
             &This::GetShapeDetailAttr)
        .def("CreateShapeDetailAttr",
             &_CreateShapeDetailAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetScreenSpacePatternAttr",
             &This::GetScreenSpacePatternAttr)
        .def("CreateScreenSpacePatternAttr",
             &_CreateScreenSpacePatternAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPatternScaleAttr",
             &This::GetPatternScaleAttr)
        .def("CreatePatternScaleAttr",
             &_CreatePatternScaleAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetStartCapTypeAttr",
             &This::GetStartCapTypeAttr)
        .def("CreateStartCapTypeAttr",
             &_CreateStartCapTypeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetEndCapTypeAttr",
             &This::GetEndCapTypeAttr)
        .def("CreateEndCapTypeAttr",
             &_CreateEndCapTypeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetJointTypeAttr",
             &This::GetJointTypeAttr)
        .def("CreateJointTypeAttr",
             &_CreateJointTypeAttr,
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
    typedef UsdGeomDashDotLines This;

    _class
        .def("GetTokenAttr", &This::GetTokenAttr)
        .def("GetFloatAttr", &This::GetFloatAttr)
        .def("GetIntAttr", &This::GetIntAttr)
        .def("GetBoolAttr", &This::GetBoolAttr)
        ;
}

}
