//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdVol/volumeFieldAsset.h"
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
_CreateFilePathAttr(UsdVolVolumeFieldAsset &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateFilePathAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateFieldNameAttr(UsdVolVolumeFieldAsset &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateFieldNameAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateFieldIndexAttr(UsdVolVolumeFieldAsset &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateFieldIndexAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateFieldDataTypeAttr(UsdVolVolumeFieldAsset &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateFieldDataTypeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateVectorDataRoleHintAttr(UsdVolVolumeFieldAsset &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateVectorDataRoleHintAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdVolVolumeFieldAsset &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdVol.VolumeFieldAsset(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdVolVolumeFieldAsset(nb::module_ &m)
{
    typedef UsdVolVolumeFieldAsset This;

    nb::class_<This, UsdVolFieldBase> cls(m, "VolumeFieldAsset");

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

        
        .def("GetFilePathAttr",
             &This::GetFilePathAttr)
        .def("CreateFilePathAttr",
             &_CreateFilePathAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetFieldNameAttr",
             &This::GetFieldNameAttr)
        .def("CreateFieldNameAttr",
             &_CreateFieldNameAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetFieldIndexAttr",
             &This::GetFieldIndexAttr)
        .def("CreateFieldIndexAttr",
             &_CreateFieldIndexAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetFieldDataTypeAttr",
             &This::GetFieldDataTypeAttr)
        .def("CreateFieldDataTypeAttr",
             &_CreateFieldDataTypeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetVectorDataRoleHintAttr",
             &This::GetVectorDataRoleHintAttr)
        .def("CreateVectorDataRoleHintAttr",
             &_CreateVectorDataRoleHintAttr,
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
