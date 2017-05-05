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
#include "pxr/usd/usdRi/pxrRodLightFilter.h"
#include "pxr/usd/usd/schemaBase.h"
#include "pxr/usd/usd/conversions.h"

#include "pxr/usd/sdf/primSpec.h"

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
_CreateWidthAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateWidthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateHeightAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateHeightAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateDepthAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDepthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateRadiusAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRadiusAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateEdgeThicknessAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateEdgeThicknessAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateScaleWidthAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateScaleWidthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateScaleHeightAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateScaleHeightAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateScaleDepthAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateScaleDepthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateRefineTopAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRefineTopAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateRefineBottomAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRefineBottomAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateRefineLeftAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRefineLeftAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateRefineRightAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRefineRightAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateRefineFrontAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRefineFrontAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateRefineBackAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRefineBackAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateEdgeTopAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateEdgeTopAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateEdgeBottomAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateEdgeBottomAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateEdgeLeftAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateEdgeLeftAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateEdgeRightAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateEdgeRightAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateEdgeFrontAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateEdgeFrontAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateEdgeBackAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateEdgeBackAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateColorSaturationAttr(UsdRiPxrRodLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColorSaturationAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}

} // anonymous namespace

void wrapUsdRiPxrRodLightFilter()
{
    typedef UsdRiPxrRodLightFilter This;

    class_<This, bases<UsdLuxLightFilter> >
        cls("PxrRodLightFilter");

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

        
        .def("GetWidthAttr",
             &This::GetWidthAttr)
        .def("CreateWidthAttr",
             &_CreateWidthAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetHeightAttr",
             &This::GetHeightAttr)
        .def("CreateHeightAttr",
             &_CreateHeightAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDepthAttr",
             &This::GetDepthAttr)
        .def("CreateDepthAttr",
             &_CreateDepthAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRadiusAttr",
             &This::GetRadiusAttr)
        .def("CreateRadiusAttr",
             &_CreateRadiusAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetEdgeThicknessAttr",
             &This::GetEdgeThicknessAttr)
        .def("CreateEdgeThicknessAttr",
             &_CreateEdgeThicknessAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetScaleWidthAttr",
             &This::GetScaleWidthAttr)
        .def("CreateScaleWidthAttr",
             &_CreateScaleWidthAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetScaleHeightAttr",
             &This::GetScaleHeightAttr)
        .def("CreateScaleHeightAttr",
             &_CreateScaleHeightAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetScaleDepthAttr",
             &This::GetScaleDepthAttr)
        .def("CreateScaleDepthAttr",
             &_CreateScaleDepthAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRefineTopAttr",
             &This::GetRefineTopAttr)
        .def("CreateRefineTopAttr",
             &_CreateRefineTopAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRefineBottomAttr",
             &This::GetRefineBottomAttr)
        .def("CreateRefineBottomAttr",
             &_CreateRefineBottomAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRefineLeftAttr",
             &This::GetRefineLeftAttr)
        .def("CreateRefineLeftAttr",
             &_CreateRefineLeftAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRefineRightAttr",
             &This::GetRefineRightAttr)
        .def("CreateRefineRightAttr",
             &_CreateRefineRightAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRefineFrontAttr",
             &This::GetRefineFrontAttr)
        .def("CreateRefineFrontAttr",
             &_CreateRefineFrontAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRefineBackAttr",
             &This::GetRefineBackAttr)
        .def("CreateRefineBackAttr",
             &_CreateRefineBackAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetEdgeTopAttr",
             &This::GetEdgeTopAttr)
        .def("CreateEdgeTopAttr",
             &_CreateEdgeTopAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetEdgeBottomAttr",
             &This::GetEdgeBottomAttr)
        .def("CreateEdgeBottomAttr",
             &_CreateEdgeBottomAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetEdgeLeftAttr",
             &This::GetEdgeLeftAttr)
        .def("CreateEdgeLeftAttr",
             &_CreateEdgeLeftAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetEdgeRightAttr",
             &This::GetEdgeRightAttr)
        .def("CreateEdgeRightAttr",
             &_CreateEdgeRightAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetEdgeFrontAttr",
             &This::GetEdgeFrontAttr)
        .def("CreateEdgeFrontAttr",
             &_CreateEdgeFrontAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetEdgeBackAttr",
             &This::GetEdgeBackAttr)
        .def("CreateEdgeBackAttr",
             &_CreateEdgeBackAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColorSaturationAttr",
             &This::GetColorSaturationAttr)
        .def("CreateColorSaturationAttr",
             &_CreateColorSaturationAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

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
