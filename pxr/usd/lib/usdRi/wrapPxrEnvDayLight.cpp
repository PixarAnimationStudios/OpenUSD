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
#include "pxr/usd/usdRi/pxrEnvDayLight.h"
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
_CreateDayAttr(UsdRiPxrEnvDayLight &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateHazinessAttr(UsdRiPxrEnvDayLight &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateHazinessAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateHourAttr(UsdRiPxrEnvDayLight &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateHourAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateLatitudeAttr(UsdRiPxrEnvDayLight &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateLatitudeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateLongitudeAttr(UsdRiPxrEnvDayLight &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateLongitudeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateMonthAttr(UsdRiPxrEnvDayLight &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMonthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateSkyTintAttr(UsdRiPxrEnvDayLight &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateSkyTintAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3f), writeSparsely);
}
        
static UsdAttribute
_CreateSunDirectionAttr(UsdRiPxrEnvDayLight &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateSunDirectionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3f), writeSparsely);
}
        
static UsdAttribute
_CreateSunSizeAttr(UsdRiPxrEnvDayLight &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateSunSizeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateSunTintAttr(UsdRiPxrEnvDayLight &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateSunTintAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3f), writeSparsely);
}
        
static UsdAttribute
_CreateYearAttr(UsdRiPxrEnvDayLight &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateYearAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateZoneAttr(UsdRiPxrEnvDayLight &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateZoneAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}

} // anonymous namespace

void wrapUsdRiPxrEnvDayLight()
{
    typedef UsdRiPxrEnvDayLight This;

    class_<This, bases<UsdLuxLight> >
        cls("PxrEnvDayLight");

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

        
        .def("GetDayAttr",
             &This::GetDayAttr)
        .def("CreateDayAttr",
             &_CreateDayAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetHazinessAttr",
             &This::GetHazinessAttr)
        .def("CreateHazinessAttr",
             &_CreateHazinessAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetHourAttr",
             &This::GetHourAttr)
        .def("CreateHourAttr",
             &_CreateHourAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetLatitudeAttr",
             &This::GetLatitudeAttr)
        .def("CreateLatitudeAttr",
             &_CreateLatitudeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetLongitudeAttr",
             &This::GetLongitudeAttr)
        .def("CreateLongitudeAttr",
             &_CreateLongitudeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMonthAttr",
             &This::GetMonthAttr)
        .def("CreateMonthAttr",
             &_CreateMonthAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetSkyTintAttr",
             &This::GetSkyTintAttr)
        .def("CreateSkyTintAttr",
             &_CreateSkyTintAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetSunDirectionAttr",
             &This::GetSunDirectionAttr)
        .def("CreateSunDirectionAttr",
             &_CreateSunDirectionAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetSunSizeAttr",
             &This::GetSunSizeAttr)
        .def("CreateSunSizeAttr",
             &_CreateSunSizeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetSunTintAttr",
             &This::GetSunTintAttr)
        .def("CreateSunTintAttr",
             &_CreateSunTintAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetYearAttr",
             &This::GetYearAttr)
        .def("CreateYearAttr",
             &_CreateYearAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetZoneAttr",
             &This::GetZoneAttr)
        .def("CreateZoneAttr",
             &_CreateZoneAttr,
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
