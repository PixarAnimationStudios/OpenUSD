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
#include "pxr/pxr.h"
#include "pxr/usd/usdHydra/uvTexture.h"
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

PXR_NAMESPACE_OPEN_SCOPE

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateUvAttr(UsdHydraUvTexture &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateUvAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}
        
static UsdAttribute
_CreateWrapSAttr(UsdHydraUvTexture &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateWrapSAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateWrapTAttr(UsdHydraUvTexture &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateWrapTAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateMinFilterAttr(UsdHydraUvTexture &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMinFilterAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateMagFilterAttr(UsdHydraUvTexture &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMagFilterAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

void wrapUsdHydraUvTexture()
{
    typedef UsdHydraUvTexture This;

    class_<This, bases<UsdHydraTexture> >
        cls("UvTexture");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")


        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetUvAttr",
             &This::GetUvAttr)
        .def("CreateUvAttr",
             &_CreateUvAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetWrapSAttr",
             &This::GetWrapSAttr)
        .def("CreateWrapSAttr",
             &_CreateWrapSAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetWrapTAttr",
             &This::GetWrapTAttr)
        .def("CreateWrapTAttr",
             &_CreateWrapTAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMinFilterAttr",
             &This::GetMinFilterAttr)
        .def("CreateMinFilterAttr",
             &_CreateMinFilterAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMagFilterAttr",
             &This::GetMagFilterAttr)
        .def("CreateMagFilterAttr",
             &_CreateMagFilterAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

    ;

    _CustomWrapCode(cls);
}

PXR_NAMESPACE_CLOSE_SCOPE

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
// Just remember to wrap code in the pxr namespace macros:
// PXR_NAMESPACE_OPEN_SCOPE, PXR_NAMESPACE_CLOSE_SCOPE.
//
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

PXR_NAMESPACE_OPEN_SCOPE

WRAP_CUSTOM {
}

PXR_NAMESPACE_CLOSE_SCOPE
