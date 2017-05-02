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
#include "pxr/usd/usdContrived/base.h"
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

using namespace foo::bar::baz;

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateMyVaryingTokenAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMyVaryingTokenAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateMyUniformBoolAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMyUniformBoolAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateMyDoubleAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMyDoubleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreateMyFloatAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMyFloatAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateMyColorFloatAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMyColorFloatAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3f), writeSparsely);
}
        
static UsdAttribute
_CreateMyNormalsAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMyNormalsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Normal3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateMyPointsAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMyPointsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Point3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateMyVelocitiesAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMyVelocitiesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Vector3fArray), writeSparsely);
}
        
static UsdAttribute
_CreateUnsignedIntAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateUnsignedIntAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->UInt), writeSparsely);
}
        
static UsdAttribute
_CreateUnsignedCharAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateUnsignedCharAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->UChar), writeSparsely);
}
        
static UsdAttribute
_CreateUnsignedInt64ArrayAttr(UsdContrivedBase &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateUnsignedInt64ArrayAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->UInt64Array), writeSparsely);
}

} // anonymous namespace

void wrapUsdContrivedBase()
{
    typedef UsdContrivedBase This;

    class_<This, bases<UsdTyped> >
        cls("Base");

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

        
        .def("GetMyVaryingTokenAttr",
             &This::GetMyVaryingTokenAttr)
        .def("CreateMyVaryingTokenAttr",
             &_CreateMyVaryingTokenAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMyUniformBoolAttr",
             &This::GetMyUniformBoolAttr)
        .def("CreateMyUniformBoolAttr",
             &_CreateMyUniformBoolAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMyDoubleAttr",
             &This::GetMyDoubleAttr)
        .def("CreateMyDoubleAttr",
             &_CreateMyDoubleAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMyFloatAttr",
             &This::GetMyFloatAttr)
        .def("CreateMyFloatAttr",
             &_CreateMyFloatAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMyColorFloatAttr",
             &This::GetMyColorFloatAttr)
        .def("CreateMyColorFloatAttr",
             &_CreateMyColorFloatAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMyNormalsAttr",
             &This::GetMyNormalsAttr)
        .def("CreateMyNormalsAttr",
             &_CreateMyNormalsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMyPointsAttr",
             &This::GetMyPointsAttr)
        .def("CreateMyPointsAttr",
             &_CreateMyPointsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMyVelocitiesAttr",
             &This::GetMyVelocitiesAttr)
        .def("CreateMyVelocitiesAttr",
             &_CreateMyVelocitiesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetUnsignedIntAttr",
             &This::GetUnsignedIntAttr)
        .def("CreateUnsignedIntAttr",
             &_CreateUnsignedIntAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetUnsignedCharAttr",
             &This::GetUnsignedCharAttr)
        .def("CreateUnsignedCharAttr",
             &_CreateUnsignedCharAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetUnsignedInt64ArrayAttr",
             &This::GetUnsignedInt64ArrayAttr)
        .def("CreateUnsignedInt64ArrayAttr",
             &_CreateUnsignedInt64ArrayAttr,
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
