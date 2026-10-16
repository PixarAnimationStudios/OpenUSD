//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdMedia/spatialAudio.h"
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
_CreateFilePathAttr(UsdMediaSpatialAudio &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateFilePathAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateAuralModeAttr(UsdMediaSpatialAudio &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateAuralModeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreatePlaybackModeAttr(UsdMediaSpatialAudio &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreatePlaybackModeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateStartTimeAttr(UsdMediaSpatialAudio &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateStartTimeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TimeCode), writeSparsely);
}
        
static UsdAttribute
_CreateEndTimeAttr(UsdMediaSpatialAudio &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateEndTimeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TimeCode), writeSparsely);
}
        
static UsdAttribute
_CreateMediaOffsetAttr(UsdMediaSpatialAudio &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateMediaOffsetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateGainAttr(UsdMediaSpatialAudio &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateGainAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}

static std::string
_Repr(const UsdMediaSpatialAudio &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdMedia.SpatialAudio(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdMediaSpatialAudio(nb::module_ &m)
{
    typedef UsdMediaSpatialAudio This;

    nb::class_<This, UsdGeomXformable> cls(m, "SpatialAudio");

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

        
        .def("GetFilePathAttr",
             &This::GetFilePathAttr)
        .def("CreateFilePathAttr",
             &_CreateFilePathAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetAuralModeAttr",
             &This::GetAuralModeAttr)
        .def("CreateAuralModeAttr",
             &_CreateAuralModeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetPlaybackModeAttr",
             &This::GetPlaybackModeAttr)
        .def("CreatePlaybackModeAttr",
             &_CreatePlaybackModeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetStartTimeAttr",
             &This::GetStartTimeAttr)
        .def("CreateStartTimeAttr",
             &_CreateStartTimeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetEndTimeAttr",
             &This::GetEndTimeAttr)
        .def("CreateEndTimeAttr",
             &_CreateEndTimeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetMediaOffsetAttr",
             &This::GetMediaOffsetAttr)
        .def("CreateMediaOffsetAttr",
             &_CreateMediaOffsetAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetGainAttr",
             &This::GetGainAttr)
        .def("CreateGainAttr",
             &_CreateGainAttr,
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
