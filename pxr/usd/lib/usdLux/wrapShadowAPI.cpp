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
#include "pxr/usd/usdLux/shadowAPI.h"
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
_CreateShadowEnableAttr(UsdLuxShadowAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateShadowEnableAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateShadowColorAttr(UsdLuxShadowAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateShadowColorAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3f), writeSparsely);
}
        
static UsdAttribute
_CreateShadowDistanceAttr(UsdLuxShadowAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateShadowDistanceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateShadowFalloffAttr(UsdLuxShadowAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateShadowFalloffAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateShadowFalloffGammaAttr(UsdLuxShadowAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateShadowFalloffGammaAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}

} // anonymous namespace

void wrapUsdLuxShadowAPI()
{
    typedef UsdLuxShadowAPI This;

    class_<This, bases<UsdAPISchemaBase> >
        cls("ShadowAPI");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("Apply", &This::Apply, (arg("prim")))
        .staticmethod("Apply")

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetShadowEnableAttr",
             &This::GetShadowEnableAttr)
        .def("CreateShadowEnableAttr",
             &_CreateShadowEnableAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetShadowColorAttr",
             &This::GetShadowColorAttr)
        .def("CreateShadowColorAttr",
             &_CreateShadowColorAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetShadowDistanceAttr",
             &This::GetShadowDistanceAttr)
        .def("CreateShadowDistanceAttr",
             &_CreateShadowDistanceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetShadowFalloffAttr",
             &This::GetShadowFalloffAttr)
        .def("CreateShadowFalloffAttr",
             &_CreateShadowFalloffAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetShadowFalloffGammaAttr",
             &This::GetShadowFalloffGammaAttr)
        .def("CreateShadowFalloffGammaAttr",
             &_CreateShadowFalloffGammaAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        
        .def("GetShadowIncludeRel",
             &This::GetShadowIncludeRel)
        .def("CreateShadowIncludeRel",
             &This::CreateShadowIncludeRel)
        
        .def("GetShadowExcludeRel",
             &This::GetShadowExcludeRel)
        .def("CreateShadowExcludeRel",
             &This::CreateShadowExcludeRel)
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
