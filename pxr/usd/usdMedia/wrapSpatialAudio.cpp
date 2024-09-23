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
_CreateFilePathAttr(UsdMediaSpatialAudio &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFilePathAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateAuralModeAttr(UsdMediaSpatialAudio &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAuralModeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreatePlaybackModeAttr(UsdMediaSpatialAudio &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePlaybackModeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateStartTimeAttr(UsdMediaSpatialAudio &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateStartTimeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TimeCode), writeSparsely);
}
        
static UsdAttribute
_CreateEndTimeAttr(UsdMediaSpatialAudio &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateEndTimeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TimeCode), writeSparsely);
}
        
static UsdAttribute
_CreateMediaOffsetAttr(UsdMediaSpatialAudio &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMediaOffsetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateGainAttr(UsdMediaSpatialAudio &self,
                                      object defaultVal, bool writeSparsely) {
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

void wrapUsdMediaSpatialAudio()
{
    typedef UsdMediaSpatialAudio This;

    class_<This, bases<UsdGeomXformable> >
        cls("SpatialAudio");

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

        
        .def("GetFilePathAttr",
             &This::GetFilePathAttr)
        .def("CreateFilePathAttr",
             &_CreateFilePathAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAuralModeAttr",
             &This::GetAuralModeAttr)
        .def("CreateAuralModeAttr",
             &_CreateAuralModeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPlaybackModeAttr",
             &This::GetPlaybackModeAttr)
        .def("CreatePlaybackModeAttr",
             &_CreatePlaybackModeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetStartTimeAttr",
             &This::GetStartTimeAttr)
        .def("CreateStartTimeAttr",
             &_CreateStartTimeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetEndTimeAttr",
             &This::GetEndTimeAttr)
        .def("CreateEndTimeAttr",
             &_CreateEndTimeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMediaOffsetAttr",
             &This::GetMediaOffsetAttr)
        .def("CreateMediaOffsetAttr",
             &_CreateMediaOffsetAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetGainAttr",
             &This::GetGainAttr)
        .def("CreateGainAttr",
             &_CreateGainAttr,
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
}

}
