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
#include "pxr/usd/usdRi/pxrBarnLightFilter.h"
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
_CreateBarnModeAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateBarnModeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateWidthAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateWidthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateHeightAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateHeightAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateRadiusAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRadiusAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticDirectionalAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticDirectionalAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticShearXAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticShearXAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticShearYAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticShearYAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticApexAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticApexAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticUseLightDirectionAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticUseLightDirectionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticDensityNearDistanceAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticDensityNearDistanceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticDensityFarDistanceAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticDensityFarDistanceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticDensityNearValueAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticDensityNearValueAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticDensityFarValueAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticDensityFarValueAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticDensityExponentAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticDensityExponentAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateEdgeThicknessAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateEdgeThicknessAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreatePreBarnEffectAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePreBarnEffectAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateScaleWidthAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateScaleWidthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateScaleHeightAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateScaleHeightAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateRefineTopAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRefineTopAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateRefineBottomAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRefineBottomAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateRefineLeftAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRefineLeftAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateRefineRightAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRefineRightAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateEdgeTopAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateEdgeTopAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateEdgeBottomAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateEdgeBottomAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateEdgeLeftAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateEdgeLeftAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateEdgeRightAttr(UsdRiPxrBarnLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateEdgeRightAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}

} // anonymous namespace

void wrapUsdRiPxrBarnLightFilter()
{
    typedef UsdRiPxrBarnLightFilter This;

    class_<This, bases<UsdLuxLightFilter> >
        cls("PxrBarnLightFilter");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("Define", &This::Define, (arg("stage"), arg("path")))
        .staticmethod("Define")

        .def("IsConcrete",
            static_cast<bool (*)(void)>( [](){ return This::IsConcrete; }))
        .staticmethod("IsConcrete")

        .def("IsTyped",
            static_cast<bool (*)(void)>( [](){ return This::IsTyped; } ))
        .staticmethod("IsTyped")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetBarnModeAttr",
             &This::GetBarnModeAttr)
        .def("CreateBarnModeAttr",
             &_CreateBarnModeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
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
        
        .def("GetRadiusAttr",
             &This::GetRadiusAttr)
        .def("CreateRadiusAttr",
             &_CreateRadiusAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticDirectionalAttr",
             &This::GetAnalyticDirectionalAttr)
        .def("CreateAnalyticDirectionalAttr",
             &_CreateAnalyticDirectionalAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticShearXAttr",
             &This::GetAnalyticShearXAttr)
        .def("CreateAnalyticShearXAttr",
             &_CreateAnalyticShearXAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticShearYAttr",
             &This::GetAnalyticShearYAttr)
        .def("CreateAnalyticShearYAttr",
             &_CreateAnalyticShearYAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticApexAttr",
             &This::GetAnalyticApexAttr)
        .def("CreateAnalyticApexAttr",
             &_CreateAnalyticApexAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticUseLightDirectionAttr",
             &This::GetAnalyticUseLightDirectionAttr)
        .def("CreateAnalyticUseLightDirectionAttr",
             &_CreateAnalyticUseLightDirectionAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticDensityNearDistanceAttr",
             &This::GetAnalyticDensityNearDistanceAttr)
        .def("CreateAnalyticDensityNearDistanceAttr",
             &_CreateAnalyticDensityNearDistanceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticDensityFarDistanceAttr",
             &This::GetAnalyticDensityFarDistanceAttr)
        .def("CreateAnalyticDensityFarDistanceAttr",
             &_CreateAnalyticDensityFarDistanceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticDensityNearValueAttr",
             &This::GetAnalyticDensityNearValueAttr)
        .def("CreateAnalyticDensityNearValueAttr",
             &_CreateAnalyticDensityNearValueAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticDensityFarValueAttr",
             &This::GetAnalyticDensityFarValueAttr)
        .def("CreateAnalyticDensityFarValueAttr",
             &_CreateAnalyticDensityFarValueAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticDensityExponentAttr",
             &This::GetAnalyticDensityExponentAttr)
        .def("CreateAnalyticDensityExponentAttr",
             &_CreateAnalyticDensityExponentAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetEdgeThicknessAttr",
             &This::GetEdgeThicknessAttr)
        .def("CreateEdgeThicknessAttr",
             &_CreateEdgeThicknessAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPreBarnEffectAttr",
             &This::GetPreBarnEffectAttr)
        .def("CreatePreBarnEffectAttr",
             &_CreatePreBarnEffectAttr,
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
