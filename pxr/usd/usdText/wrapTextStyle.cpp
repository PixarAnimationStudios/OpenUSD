//
// Copyright 2023 Pixar
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
#include "pxr/usd/usdText/textStyle.h"
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
_CreateTypefaceAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTypefaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateBoldAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateBoldAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateItalicAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateItalicAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateWeightAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateWeightAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateTextHeightAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTextHeightAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateTextWidthFactorAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTextWidthFactorAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateObliqueAngleAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateObliqueAngleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateCharSpacingAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCharSpacingAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateUnderlineTypeAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateUnderlineTypeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateOverlineTypeAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateOverlineTypeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateStrikethroughTypeAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateStrikethroughTypeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}

static std::string
_Repr(const UsdTextTextStyle &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdText.TextStyle(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdTextTextStyle()
{
    typedef UsdTextTextStyle This;

    class_<This, bases<UsdTyped> >
        cls("TextStyle");

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

        
        .def("GetTypefaceAttr",
             &This::GetTypefaceAttr)
        .def("CreateTypefaceAttr",
             &_CreateTypefaceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetBoldAttr",
             &This::GetBoldAttr)
        .def("CreateBoldAttr",
             &_CreateBoldAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetItalicAttr",
             &This::GetItalicAttr)
        .def("CreateItalicAttr",
             &_CreateItalicAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetWeightAttr",
             &This::GetWeightAttr)
        .def("CreateWeightAttr",
             &_CreateWeightAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTextHeightAttr",
             &This::GetTextHeightAttr)
        .def("CreateTextHeightAttr",
             &_CreateTextHeightAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTextWidthFactorAttr",
             &This::GetTextWidthFactorAttr)
        .def("CreateTextWidthFactorAttr",
             &_CreateTextWidthFactorAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetObliqueAngleAttr",
             &This::GetObliqueAngleAttr)
        .def("CreateObliqueAngleAttr",
             &_CreateObliqueAngleAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCharSpacingAttr",
             &This::GetCharSpacingAttr)
        .def("CreateCharSpacingAttr",
             &_CreateCharSpacingAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetUnderlineTypeAttr",
             &This::GetUnderlineTypeAttr)
        .def("CreateUnderlineTypeAttr",
             &_CreateUnderlineTypeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetOverlineTypeAttr",
             &This::GetOverlineTypeAttr)
        .def("CreateOverlineTypeAttr",
             &_CreateOverlineTypeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetStrikethroughTypeAttr",
             &This::GetStrikethroughTypeAttr)
        .def("CreateStrikethroughTypeAttr",
             &_CreateStrikethroughTypeAttr,
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
}

}
