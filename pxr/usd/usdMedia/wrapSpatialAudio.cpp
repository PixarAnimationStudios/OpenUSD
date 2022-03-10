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
#include "pxr/usd/usdMedia/spatialAudio.h"
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

namespace pxrUsdUsdMediaWrapSpatialAudio {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateFilePathAttr(UsdMediaSpatialAudio &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateFilePathAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateAuralModeAttr(UsdMediaSpatialAudio &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateAuralModeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreatePlaybackModeAttr(UsdMediaSpatialAudio &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreatePlaybackModeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateStartTimeAttr(UsdMediaSpatialAudio &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateStartTimeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TimeCode), writeSparsely);
}
        
static UsdAttribute
_CreateEndTimeAttr(UsdMediaSpatialAudio &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateEndTimeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TimeCode), writeSparsely);
}
        
static UsdAttribute
_CreateMediaOffsetAttr(UsdMediaSpatialAudio &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateMediaOffsetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateGainAttr(UsdMediaSpatialAudio &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
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

    boost::python::class_<This, boost::python::bases<UsdGeomXformable> >
        cls("SpatialAudio");

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

        
        .def("GetFilePathAttr",
             &This::GetFilePathAttr)
        .def("CreateFilePathAttr",
             &pxrUsdUsdMediaWrapSpatialAudio::_CreateFilePathAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetAuralModeAttr",
             &This::GetAuralModeAttr)
        .def("CreateAuralModeAttr",
             &pxrUsdUsdMediaWrapSpatialAudio::_CreateAuralModeAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetPlaybackModeAttr",
             &This::GetPlaybackModeAttr)
        .def("CreatePlaybackModeAttr",
             &pxrUsdUsdMediaWrapSpatialAudio::_CreatePlaybackModeAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetStartTimeAttr",
             &This::GetStartTimeAttr)
        .def("CreateStartTimeAttr",
             &pxrUsdUsdMediaWrapSpatialAudio::_CreateStartTimeAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetEndTimeAttr",
             &This::GetEndTimeAttr)
        .def("CreateEndTimeAttr",
             &pxrUsdUsdMediaWrapSpatialAudio::_CreateEndTimeAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetMediaOffsetAttr",
             &This::GetMediaOffsetAttr)
        .def("CreateMediaOffsetAttr",
             &pxrUsdUsdMediaWrapSpatialAudio::_CreateMediaOffsetAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetGainAttr",
             &This::GetGainAttr)
        .def("CreateGainAttr",
             &pxrUsdUsdMediaWrapSpatialAudio::_CreateGainAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))

        .def("__repr__", pxrUsdUsdMediaWrapSpatialAudio::_Repr)
    ;

    pxrUsdUsdMediaWrapSpatialAudio::_CustomWrapCode(cls);
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

namespace pxrUsdUsdMediaWrapSpatialAudio {

WRAP_CUSTOM {
}

}
