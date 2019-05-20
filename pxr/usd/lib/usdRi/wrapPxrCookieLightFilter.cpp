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
#include "pxr/usd/usdRi/pxrCookieLightFilter.h"
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
_CreateCookieModeAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCookieModeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateWidthAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateWidthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateHeightAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateHeightAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateTextureMapAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTextureMapAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Asset), writeSparsely);
}
        
static UsdAttribute
_CreateTextureWrapModeAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTextureWrapModeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateTextureFillColorAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTextureFillColorAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3f), writeSparsely);
}
        
static UsdAttribute
_CreateTextureInvertUAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTextureInvertUAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateTextureInvertVAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTextureInvertVAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateTextureScaleUAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTextureScaleUAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateTextureScaleVAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTextureScaleVAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateTextureOffsetUAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTextureOffsetUAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateTextureOffsetVAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTextureOffsetVAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticDirectionalAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticDirectionalAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticShearXAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticShearXAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticShearYAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticShearYAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticApexAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticApexAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticUseLightDirectionAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticUseLightDirectionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticBlurAmountAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticBlurAmountAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticBlurSMultAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticBlurSMultAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticBlurTMultAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticBlurTMultAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticBlurNearDistanceAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticBlurNearDistanceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticBlurMidpointAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticBlurMidpointAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticBlurFarDistanceAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticBlurFarDistanceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticBlurNearValueAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticBlurNearValueAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticBlurMidValueAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticBlurMidValueAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticBlurFarValueAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticBlurFarValueAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticBlurExponentAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticBlurExponentAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticDensityNearDistanceAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticDensityNearDistanceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticDensityMidpointAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticDensityMidpointAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticDensityFarDistanceAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticDensityFarDistanceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticDensityNearValueAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticDensityNearValueAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticDensityMidValueAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticDensityMidValueAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticDensityFarValueAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticDensityFarValueAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateAnalyticDensityExponentAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateAnalyticDensityExponentAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateColorSaturationAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColorSaturationAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateColorMidpointAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColorMidpointAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateColorContrastAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColorContrastAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateColorWhitepointAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColorWhitepointAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateColorTintAttr(UsdRiPxrCookieLightFilter &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColorTintAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3f), writeSparsely);
}

} // anonymous namespace

void wrapUsdRiPxrCookieLightFilter()
{
    typedef UsdRiPxrCookieLightFilter This;

    class_<This, bases<UsdLuxLightFilter> >
        cls("PxrCookieLightFilter");

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

        
        .def("GetCookieModeAttr",
             &This::GetCookieModeAttr)
        .def("CreateCookieModeAttr",
             &_CreateCookieModeAttr,
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
        
        .def("GetTextureMapAttr",
             &This::GetTextureMapAttr)
        .def("CreateTextureMapAttr",
             &_CreateTextureMapAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTextureWrapModeAttr",
             &This::GetTextureWrapModeAttr)
        .def("CreateTextureWrapModeAttr",
             &_CreateTextureWrapModeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTextureFillColorAttr",
             &This::GetTextureFillColorAttr)
        .def("CreateTextureFillColorAttr",
             &_CreateTextureFillColorAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTextureInvertUAttr",
             &This::GetTextureInvertUAttr)
        .def("CreateTextureInvertUAttr",
             &_CreateTextureInvertUAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTextureInvertVAttr",
             &This::GetTextureInvertVAttr)
        .def("CreateTextureInvertVAttr",
             &_CreateTextureInvertVAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTextureScaleUAttr",
             &This::GetTextureScaleUAttr)
        .def("CreateTextureScaleUAttr",
             &_CreateTextureScaleUAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTextureScaleVAttr",
             &This::GetTextureScaleVAttr)
        .def("CreateTextureScaleVAttr",
             &_CreateTextureScaleVAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTextureOffsetUAttr",
             &This::GetTextureOffsetUAttr)
        .def("CreateTextureOffsetUAttr",
             &_CreateTextureOffsetUAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTextureOffsetVAttr",
             &This::GetTextureOffsetVAttr)
        .def("CreateTextureOffsetVAttr",
             &_CreateTextureOffsetVAttr,
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
        
        .def("GetAnalyticBlurAmountAttr",
             &This::GetAnalyticBlurAmountAttr)
        .def("CreateAnalyticBlurAmountAttr",
             &_CreateAnalyticBlurAmountAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticBlurSMultAttr",
             &This::GetAnalyticBlurSMultAttr)
        .def("CreateAnalyticBlurSMultAttr",
             &_CreateAnalyticBlurSMultAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticBlurTMultAttr",
             &This::GetAnalyticBlurTMultAttr)
        .def("CreateAnalyticBlurTMultAttr",
             &_CreateAnalyticBlurTMultAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticBlurNearDistanceAttr",
             &This::GetAnalyticBlurNearDistanceAttr)
        .def("CreateAnalyticBlurNearDistanceAttr",
             &_CreateAnalyticBlurNearDistanceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticBlurMidpointAttr",
             &This::GetAnalyticBlurMidpointAttr)
        .def("CreateAnalyticBlurMidpointAttr",
             &_CreateAnalyticBlurMidpointAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticBlurFarDistanceAttr",
             &This::GetAnalyticBlurFarDistanceAttr)
        .def("CreateAnalyticBlurFarDistanceAttr",
             &_CreateAnalyticBlurFarDistanceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticBlurNearValueAttr",
             &This::GetAnalyticBlurNearValueAttr)
        .def("CreateAnalyticBlurNearValueAttr",
             &_CreateAnalyticBlurNearValueAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticBlurMidValueAttr",
             &This::GetAnalyticBlurMidValueAttr)
        .def("CreateAnalyticBlurMidValueAttr",
             &_CreateAnalyticBlurMidValueAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticBlurFarValueAttr",
             &This::GetAnalyticBlurFarValueAttr)
        .def("CreateAnalyticBlurFarValueAttr",
             &_CreateAnalyticBlurFarValueAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticBlurExponentAttr",
             &This::GetAnalyticBlurExponentAttr)
        .def("CreateAnalyticBlurExponentAttr",
             &_CreateAnalyticBlurExponentAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticDensityNearDistanceAttr",
             &This::GetAnalyticDensityNearDistanceAttr)
        .def("CreateAnalyticDensityNearDistanceAttr",
             &_CreateAnalyticDensityNearDistanceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetAnalyticDensityMidpointAttr",
             &This::GetAnalyticDensityMidpointAttr)
        .def("CreateAnalyticDensityMidpointAttr",
             &_CreateAnalyticDensityMidpointAttr,
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
        
        .def("GetAnalyticDensityMidValueAttr",
             &This::GetAnalyticDensityMidValueAttr)
        .def("CreateAnalyticDensityMidValueAttr",
             &_CreateAnalyticDensityMidValueAttr,
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
        
        .def("GetColorSaturationAttr",
             &This::GetColorSaturationAttr)
        .def("CreateColorSaturationAttr",
             &_CreateColorSaturationAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColorMidpointAttr",
             &This::GetColorMidpointAttr)
        .def("CreateColorMidpointAttr",
             &_CreateColorMidpointAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColorContrastAttr",
             &This::GetColorContrastAttr)
        .def("CreateColorContrastAttr",
             &_CreateColorContrastAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColorWhitepointAttr",
             &This::GetColorWhitepointAttr)
        .def("CreateColorWhitepointAttr",
             &_CreateColorWhitepointAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColorTintAttr",
             &This::GetColorTintAttr)
        .def("CreateColorTintAttr",
             &_CreateColorTintAttr,
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
