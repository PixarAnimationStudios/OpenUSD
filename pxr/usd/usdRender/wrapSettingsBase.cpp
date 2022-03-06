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
#include "pxr/usd/usdRender/settingsBase.h"
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

namespace pxrUsdUsdRenderWrapSettingsBase {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateResolutionAttr(UsdRenderSettingsBase &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateResolutionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int2), writeSparsely);
}
        
static UsdAttribute
_CreatePixelAspectRatioAttr(UsdRenderSettingsBase &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreatePixelAspectRatioAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAspectRatioConformPolicyAttr(UsdRenderSettingsBase &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateAspectRatioConformPolicyAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateDataWindowNDCAttr(UsdRenderSettingsBase &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateDataWindowNDCAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float4), writeSparsely);
}
        
static UsdAttribute
_CreateInstantaneousShutterAttr(UsdRenderSettingsBase &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateInstantaneousShutterAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}

static std::string
_Repr(const UsdRenderSettingsBase &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdRender.SettingsBase(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdRenderSettingsBase()
{
    typedef UsdRenderSettingsBase This;

    boost::python::class_<This, boost::python::bases<UsdTyped> >
        cls("SettingsBase");

    cls
        .def(boost::python::init<UsdPrim>(boost::python::arg("prim")))
        .def(boost::python::init<UsdSchemaBase const&>(boost::python::arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (boost::python::arg("stage"), boost::python::arg("path")))
        .staticmethod("Get")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             boost::python::arg("includeInherited")=true,
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!boost::python::self)

        
        .def("GetResolutionAttr",
             &This::GetResolutionAttr)
        .def("CreateResolutionAttr",
             &pxrUsdUsdRenderWrapSettingsBase::_CreateResolutionAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetPixelAspectRatioAttr",
             &This::GetPixelAspectRatioAttr)
        .def("CreatePixelAspectRatioAttr",
             &pxrUsdUsdRenderWrapSettingsBase::_CreatePixelAspectRatioAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetAspectRatioConformPolicyAttr",
             &This::GetAspectRatioConformPolicyAttr)
        .def("CreateAspectRatioConformPolicyAttr",
             &pxrUsdUsdRenderWrapSettingsBase::_CreateAspectRatioConformPolicyAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetDataWindowNDCAttr",
             &This::GetDataWindowNDCAttr)
        .def("CreateDataWindowNDCAttr",
             &pxrUsdUsdRenderWrapSettingsBase::_CreateDataWindowNDCAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetInstantaneousShutterAttr",
             &This::GetInstantaneousShutterAttr)
        .def("CreateInstantaneousShutterAttr",
             &pxrUsdUsdRenderWrapSettingsBase::_CreateInstantaneousShutterAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))

        
        .def("GetCameraRel",
             &This::GetCameraRel)
        .def("CreateCameraRel",
             &This::CreateCameraRel)
        .def("__repr__", pxrUsdUsdRenderWrapSettingsBase::_Repr)
    ;

    pxrUsdUsdRenderWrapSettingsBase::_CustomWrapCode(cls);
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

namespace pxrUsdUsdRenderWrapSettingsBase {

WRAP_CUSTOM {
}

}
