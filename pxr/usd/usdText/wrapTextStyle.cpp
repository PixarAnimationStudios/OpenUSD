//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
_CreateFontTypefaceAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFontTypefaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateFontFormatAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFontFormatAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateFontAltTypefaceAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFontAltTypefaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateFontAltFormatAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFontAltFormatAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateFontBoldAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFontBoldAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateFontItalicAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFontItalicAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateFontWeightAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFontWeightAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateCharHeightAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCharHeightAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateCharWidthFactorAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCharWidthFactorAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateObliqueAngleAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateObliqueAngleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateCharSpacingFactorAttr(UsdTextTextStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCharSpacingFactorAttr(
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

        
        .def("GetFontTypefaceAttr",
             &This::GetFontTypefaceAttr)
        .def("CreateFontTypefaceAttr",
             &_CreateFontTypefaceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFontFormatAttr",
             &This::GetFontFormatAttr)
        .def("CreateFontFormatAttr",
             &_CreateFontFormatAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFontAltTypefaceAttr",
             &This::GetFontAltTypefaceAttr)
        .def("CreateFontAltTypefaceAttr",
             &_CreateFontAltTypefaceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFontAltFormatAttr",
             &This::GetFontAltFormatAttr)
        .def("CreateFontAltFormatAttr",
             &_CreateFontAltFormatAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFontBoldAttr",
             &This::GetFontBoldAttr)
        .def("CreateFontBoldAttr",
             &_CreateFontBoldAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFontItalicAttr",
             &This::GetFontItalicAttr)
        .def("CreateFontItalicAttr",
             &_CreateFontItalicAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetFontWeightAttr",
             &This::GetFontWeightAttr)
        .def("CreateFontWeightAttr",
             &_CreateFontWeightAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCharHeightAttr",
             &This::GetCharHeightAttr)
        .def("CreateCharHeightAttr",
             &_CreateCharHeightAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCharWidthFactorAttr",
             &This::GetCharWidthFactorAttr)
        .def("CreateCharWidthFactorAttr",
             &_CreateCharWidthFactorAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetObliqueAngleAttr",
             &This::GetObliqueAngleAttr)
        .def("CreateObliqueAngleAttr",
             &_CreateObliqueAngleAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCharSpacingFactorAttr",
             &This::GetCharSpacingFactorAttr)
        .def("CreateCharSpacingFactorAttr",
             &_CreateCharSpacingFactorAttr,
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
