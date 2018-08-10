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
#include "pxr/usd/usdSkel/animation.h"
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
_CreateJointsAttr(UsdSkelAnimation &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateJointsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TokenArray), writeSparsely);
}
        
static UsdAttribute
_CreateTranslationsAttr(UsdSkelAnimation &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTranslationsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float3Array), writeSparsely);
}
        
static UsdAttribute
_CreateRotationsAttr(UsdSkelAnimation &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRotationsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->QuatfArray), writeSparsely);
}
        
static UsdAttribute
_CreateScalesAttr(UsdSkelAnimation &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateScalesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Half3Array), writeSparsely);
}
        
static UsdAttribute
_CreateBlendShapesAttr(UsdSkelAnimation &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateBlendShapesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TokenArray), writeSparsely);
}
        
static UsdAttribute
_CreateBlendShapeWeightsAttr(UsdSkelAnimation &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateBlendShapeWeightsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}

} // anonymous namespace

void wrapUsdSkelAnimation()
{
    typedef UsdSkelAnimation This;

    class_<This, bases<UsdTyped> >
        cls("Animation");

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

        
        .def("GetJointsAttr",
             &This::GetJointsAttr)
        .def("CreateJointsAttr",
             &_CreateJointsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTranslationsAttr",
             &This::GetTranslationsAttr)
        .def("CreateTranslationsAttr",
             &_CreateTranslationsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRotationsAttr",
             &This::GetRotationsAttr)
        .def("CreateRotationsAttr",
             &_CreateRotationsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetScalesAttr",
             &This::GetScalesAttr)
        .def("CreateScalesAttr",
             &_CreateScalesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetBlendShapesAttr",
             &This::GetBlendShapesAttr)
        .def("CreateBlendShapesAttr",
             &_CreateBlendShapesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetBlendShapeWeightsAttr",
             &This::GetBlendShapeWeightsAttr)
        .def("CreateBlendShapeWeightsAttr",
             &_CreateBlendShapeWeightsAttr,
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
