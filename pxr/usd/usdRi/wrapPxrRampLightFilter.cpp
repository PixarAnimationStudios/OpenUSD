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
#include "pxr/usd/usdRi/pxrRampLightFilter.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateRampModeAttr(UsdRiPxrRampLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRampModeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateBeginDistanceAttr(UsdRiPxrRampLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateBeginDistanceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateEndDistanceAttr(UsdRiPxrRampLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateEndDistanceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateFalloffAttr(UsdRiPxrRampLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFalloffAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateFalloffKnotsAttr(UsdRiPxrRampLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFalloffKnotsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}
        
static UsdAttribute
_CreateFalloffFloatsAttr(UsdRiPxrRampLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFalloffFloatsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}
        
static UsdAttribute
_CreateFalloffInterpolationAttr(UsdRiPxrRampLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFalloffInterpolationAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateColorRampAttr(UsdRiPxrRampLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColorRampAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateColorRampKnotsAttr(UsdRiPxrRampLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColorRampKnotsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}
        
static UsdAttribute
_CreateColorRampColorsAttr(UsdRiPxrRampLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColorRampColorsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateColorRampInterpolationAttr(UsdRiPxrRampLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColorRampInterpolationAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdRiPxrRampLightFilter &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdRi.PxrRampLightFilter(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdRiPxrRampLightFilter()
{
    typedef UsdRiPxrRampLightFilter This;

    class_<This, bases<UsdLuxLightFilter> >
        cls("PxrRampLightFilter");

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

        
        .def("GetRampModeAttr",
             &This::GetRampModeAttr)
        .def("CreateRampModeAttr",
             &_CreateRampModeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetBeginDistanceAttr",
             &This::GetBeginDistanceAttr)
        .def("CreateBeginDistanceAttr",
             &_CreateBeginDistanceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetEndDistanceAttr",
             &This::GetEndDistanceAttr)
        .def("CreateEndDistanceAttr",
             &_CreateEndDistanceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFalloffAttr",
             &This::GetFalloffAttr)
        .def("CreateFalloffAttr",
             &_CreateFalloffAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFalloffKnotsAttr",
             &This::GetFalloffKnotsAttr)
        .def("CreateFalloffKnotsAttr",
             &_CreateFalloffKnotsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFalloffFloatsAttr",
             &This::GetFalloffFloatsAttr)
        .def("CreateFalloffFloatsAttr",
             &_CreateFalloffFloatsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFalloffInterpolationAttr",
             &This::GetFalloffInterpolationAttr)
        .def("CreateFalloffInterpolationAttr",
             &_CreateFalloffInterpolationAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColorRampAttr",
             &This::GetColorRampAttr)
        .def("CreateColorRampAttr",
             &_CreateColorRampAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColorRampKnotsAttr",
             &This::GetColorRampKnotsAttr)
        .def("CreateColorRampKnotsAttr",
             &_CreateColorRampKnotsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColorRampColorsAttr",
             &This::GetColorRampColorsAttr)
        .def("CreateColorRampColorsAttr",
             &_CreateColorRampColorsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColorRampInterpolationAttr",
             &This::GetColorRampInterpolationAttr)
        .def("CreateColorRampInterpolationAttr",
             &_CreateColorRampInterpolationAttr,
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
        .def("GetFalloffRampAPI", &UsdRiPxrRampLightFilter::GetFalloffRampAPI)
        .def("GetColorRampAPI", &UsdRiPxrRampLightFilter::GetColorRampAPI)
        ;
}

}
