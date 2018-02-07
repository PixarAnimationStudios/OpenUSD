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
#include "pxr/usd/usdRi/lightAPI.h"
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
_CreateRiSamplingFixedSampleCountAttr(UsdRiLightAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRiSamplingFixedSampleCountAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateRiSamplingImportanceMultiplierAttr(UsdRiLightAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRiSamplingImportanceMultiplierAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateRiIntensityNearDistAttr(UsdRiLightAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRiIntensityNearDistAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateRiLightGroupAttr(UsdRiLightAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRiLightGroupAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateRiShadowThinShadowAttr(UsdRiLightAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRiShadowThinShadowAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateRiTraceLightPathsAttr(UsdRiLightAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRiTraceLightPathsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}

} // anonymous namespace

void wrapUsdRiLightAPI()
{
    typedef UsdRiLightAPI This;

    class_<This, bases<UsdSchemaBase> >
        cls("LightAPI");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("Apply", &This::Apply, (arg("stage"), arg("path")))
        .staticmethod("Apply")

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

        
        .def("GetRiSamplingFixedSampleCountAttr",
             &This::GetRiSamplingFixedSampleCountAttr)
        .def("CreateRiSamplingFixedSampleCountAttr",
             &_CreateRiSamplingFixedSampleCountAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRiSamplingImportanceMultiplierAttr",
             &This::GetRiSamplingImportanceMultiplierAttr)
        .def("CreateRiSamplingImportanceMultiplierAttr",
             &_CreateRiSamplingImportanceMultiplierAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRiIntensityNearDistAttr",
             &This::GetRiIntensityNearDistAttr)
        .def("CreateRiIntensityNearDistAttr",
             &_CreateRiIntensityNearDistAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRiLightGroupAttr",
             &This::GetRiLightGroupAttr)
        .def("CreateRiLightGroupAttr",
             &_CreateRiLightGroupAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRiShadowThinShadowAttr",
             &This::GetRiShadowThinShadowAttr)
        .def("CreateRiShadowThinShadowAttr",
             &_CreateRiShadowThinShadowAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRiTraceLightPathsAttr",
             &This::GetRiTraceLightPathsAttr)
        .def("CreateRiTraceLightPathsAttr",
             &_CreateRiTraceLightPathsAttr,
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
