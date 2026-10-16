//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/base.h"
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

namespace nb = nanobind;

#include <string>

using namespace foo;

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateMyVaryingTokenAttr(UsdContrivedBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateMyVaryingTokenAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateMyVaryingTokenArrayAttr(UsdContrivedBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateMyVaryingTokenArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TokenArray), writeSparsely);
}
        
static UsdAttribute
_CreateMyDoubleAttr(UsdContrivedBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateMyDoubleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateMyFloatAttr(UsdContrivedBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateMyFloatAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateMyColorFloatAttr(UsdContrivedBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateMyColorFloatAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3f), writeSparsely);
}
        
static UsdAttribute
_CreateMyNormalsAttr(UsdContrivedBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateMyNormalsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Normal3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateMyPointsAttr(UsdContrivedBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateMyPointsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateMyVelocitiesAttr(UsdContrivedBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateMyVelocitiesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateUnsignedIntAttr(UsdContrivedBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateUnsignedIntAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->UInt), writeSparsely);
}
        
static UsdAttribute
_CreateUnsignedCharAttr(UsdContrivedBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateUnsignedCharAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->UChar), writeSparsely);
}
        
static UsdAttribute
_CreateUnsignedInt64ArrayAttr(UsdContrivedBase &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateUnsignedInt64ArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->UInt64Array), writeSparsely);
}

static std::string
_Repr(const UsdContrivedBase &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdContrived.Base(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdContrivedBase(nb::module_ &m)
{
    typedef UsdContrivedBase This;

    nb::class_<This, UsdTyped>
        cls(m, "Base");

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

        
        .def("GetMyVaryingTokenAttr",
             &This::GetMyVaryingTokenAttr)
        .def("CreateMyVaryingTokenAttr",
             &_CreateMyVaryingTokenAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetMyVaryingTokenArrayAttr",
             &This::GetMyVaryingTokenArrayAttr)
        .def("CreateMyVaryingTokenArrayAttr",
             &_CreateMyVaryingTokenArrayAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetMyDoubleAttr",
             &This::GetMyDoubleAttr)
        .def("CreateMyDoubleAttr",
             &_CreateMyDoubleAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetMyFloatAttr",
             &This::GetMyFloatAttr)
        .def("CreateMyFloatAttr",
             &_CreateMyFloatAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetMyColorFloatAttr",
             &This::GetMyColorFloatAttr)
        .def("CreateMyColorFloatAttr",
             &_CreateMyColorFloatAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetMyNormalsAttr",
             &This::GetMyNormalsAttr)
        .def("CreateMyNormalsAttr",
             &_CreateMyNormalsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetMyPointsAttr",
             &This::GetMyPointsAttr)
        .def("CreateMyPointsAttr",
             &_CreateMyPointsAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetMyVelocitiesAttr",
             &This::GetMyVelocitiesAttr)
        .def("CreateMyVelocitiesAttr",
             &_CreateMyVelocitiesAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetUnsignedIntAttr",
             &This::GetUnsignedIntAttr)
        .def("CreateUnsignedIntAttr",
             &_CreateUnsignedIntAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetUnsignedCharAttr",
             &This::GetUnsignedCharAttr)
        .def("CreateUnsignedCharAttr",
             &_CreateUnsignedCharAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetUnsignedInt64ArrayAttr",
             &This::GetUnsignedInt64ArrayAttr)
        .def("CreateUnsignedInt64ArrayAttr",
             &_CreateUnsignedInt64ArrayAttr,
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

}
