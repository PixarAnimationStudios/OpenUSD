//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdText/paragraphStyle.h"
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
_CreateFirstLineIndentAttr(UsdTextParagraphStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateFirstLineIndentAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateLeftIndentAttr(UsdTextParagraphStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateLeftIndentAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateRightIndentAttr(UsdTextParagraphStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateRightIndentAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateParagraphSpaceAttr(UsdTextParagraphStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateParagraphSpaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateParagraphAlignmentAttr(UsdTextParagraphStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateParagraphAlignmentAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateTabStopPositionsAttr(UsdTextParagraphStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTabStopPositionsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->FloatArray), writeSparsely);
}
        
static UsdAttribute
_CreateTabStopTypesAttr(UsdTextParagraphStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateTabStopTypesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->TokenArray), writeSparsely);
}
        
static UsdAttribute
_CreateLineSpaceAttr(UsdTextParagraphStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateLineSpaceAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateLineSpaceTypeAttr(UsdTextParagraphStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateLineSpaceTypeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdTextParagraphStyle &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdText.ParagraphStyle(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdTextParagraphStyle()
{
    typedef UsdTextParagraphStyle This;

    class_<This, bases<UsdTyped> >
        cls("ParagraphStyle");

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

        
        .def("GetFirstLineIndentAttr",
             &This::GetFirstLineIndentAttr)
        .def("CreateFirstLineIndentAttr",
             &_CreateFirstLineIndentAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetLeftIndentAttr",
             &This::GetLeftIndentAttr)
        .def("CreateLeftIndentAttr",
             &_CreateLeftIndentAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetRightIndentAttr",
             &This::GetRightIndentAttr)
        .def("CreateRightIndentAttr",
             &_CreateRightIndentAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetParagraphSpaceAttr",
             &This::GetParagraphSpaceAttr)
        .def("CreateParagraphSpaceAttr",
             &_CreateParagraphSpaceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetParagraphAlignmentAttr",
             &This::GetParagraphAlignmentAttr)
        .def("CreateParagraphAlignmentAttr",
             &_CreateParagraphAlignmentAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTabStopPositionsAttr",
             &This::GetTabStopPositionsAttr)
        .def("CreateTabStopPositionsAttr",
             &_CreateTabStopPositionsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetTabStopTypesAttr",
             &This::GetTabStopTypesAttr)
        .def("CreateTabStopTypesAttr",
             &_CreateTabStopTypesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetLineSpaceAttr",
             &This::GetLineSpaceAttr)
        .def("CreateLineSpaceAttr",
             &_CreateLineSpaceAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetLineSpaceTypeAttr",
             &This::GetLineSpaceTypeAttr)
        .def("CreateLineSpaceTypeAttr",
             &_CreateLineSpaceTypeAttr,
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
