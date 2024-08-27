//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdText/markupText.h"
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
_CreateMarkupAttr(UsdTextMarkupText &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMarkupAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateMarkupPlainAttr(UsdTextMarkupText &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMarkupPlainAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateMarkupLanguageAttr(UsdTextMarkupText &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMarkupLanguageAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateBackgroundColorAttr(UsdTextMarkupText &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateBackgroundColorAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Color3f), writeSparsely);
}
        
static UsdAttribute
_CreateBackgroundOpacityAttr(UsdTextMarkupText &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateBackgroundOpacityAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateTextMetricsUnitAttr(UsdTextMarkupText &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTextMetricsUnitAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdTextMarkupText &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdText.MarkupText(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdTextMarkupText()
{
    typedef UsdTextMarkupText This;

    class_<This, bases<UsdGeomGprim> >
        cls("MarkupText");

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

        
        .def("GetMarkupAttr",
             &This::GetMarkupAttr)
        .def("CreateMarkupAttr",
             &_CreateMarkupAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMarkupPlainAttr",
             &This::GetMarkupPlainAttr)
        .def("CreateMarkupPlainAttr",
             &_CreateMarkupPlainAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMarkupLanguageAttr",
             &This::GetMarkupLanguageAttr)
        .def("CreateMarkupLanguageAttr",
             &_CreateMarkupLanguageAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetBackgroundColorAttr",
             &This::GetBackgroundColorAttr)
        .def("CreateBackgroundColorAttr",
             &_CreateBackgroundColorAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetBackgroundOpacityAttr",
             &This::GetBackgroundOpacityAttr)
        .def("CreateBackgroundOpacityAttr",
             &_CreateBackgroundOpacityAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTextMetricsUnitAttr",
             &This::GetTextMetricsUnitAttr)
        .def("CreateTextMetricsUnitAttr",
             &_CreateTextMetricsUnitAttr,
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
