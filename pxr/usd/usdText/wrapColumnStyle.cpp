//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdText/columnStyle.h"
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
_CreateColumnWidthAttr(UsdTextColumnStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColumnWidthAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateColumnHeightAttr(UsdTextColumnStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColumnHeightAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float), writeSparsely);
}
        
static UsdAttribute
_CreateColumnOffsetAttr(UsdTextColumnStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColumnOffsetAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float2), writeSparsely);
}
        
static UsdAttribute
_CreateMarginsAttr(UsdTextColumnStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMarginsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Float4), writeSparsely);
}
        
static UsdAttribute
_CreateColumnAlignmentAttr(UsdTextColumnStyle &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateColumnAlignmentAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdTextColumnStyle &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdText.ColumnStyle(%s)",
        primRepr.c_str());
}

} // anonymous namespace

void wrapUsdTextColumnStyle()
{
    typedef UsdTextColumnStyle This;

    class_<This, bases<UsdTyped> >
        cls("ColumnStyle");

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

        
        .def("GetColumnWidthAttr",
             &This::GetColumnWidthAttr)
        .def("CreateColumnWidthAttr",
             &_CreateColumnWidthAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColumnHeightAttr",
             &This::GetColumnHeightAttr)
        .def("CreateColumnHeightAttr",
             &_CreateColumnHeightAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColumnOffsetAttr",
             &This::GetColumnOffsetAttr)
        .def("CreateColumnOffsetAttr",
             &_CreateColumnOffsetAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMarginsAttr",
             &This::GetMarginsAttr)
        .def("CreateMarginsAttr",
             &_CreateMarginsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetColumnAlignmentAttr",
             &This::GetColumnAlignmentAttr)
        .def("CreateColumnAlignmentAttr",
             &_CreateColumnAlignmentAttr,
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
