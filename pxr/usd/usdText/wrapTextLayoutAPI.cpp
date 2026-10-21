//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdText/textLayoutAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
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
_CreateLayoutBaselineDirectionAttr(UsdTextTextLayoutAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateLayoutBaselineDirectionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateLayoutLinesStackDirectionAttr(UsdTextTextLayoutAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateLayoutLinesStackDirectionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static std::string
_Repr(const UsdTextTextLayoutAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    return TfStringPrintf(
        "UsdText.TextLayoutAPI(%s)",
        primRepr.c_str());
}

struct UsdTextTextLayoutAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdTextTextLayoutAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdTextTextLayoutAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim)
{
    std::string whyNot;
    bool result = UsdTextTextLayoutAPI::CanApply(prim, &whyNot);
    return UsdTextTextLayoutAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdTextTextLayoutAPI()
{
    typedef UsdTextTextLayoutAPI This;

    UsdTextTextLayoutAPI_CanApplyResult::Wrap<UsdTextTextLayoutAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    class_<This, bases<UsdAPISchemaBase> >
        cls("TextLayoutAPI");

    cls
        .def(init<UsdPrim>(arg("prim")))
        .def(init<UsdSchemaBase const&>(arg("schemaObj")))
        .def(TfTypePythonClass())

        .def("Get", &This::Get, (arg("stage"), arg("path")))
        .staticmethod("Get")

        .def("CanApply", &_WrapCanApply, (arg("prim")))
        .staticmethod("CanApply")

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

        
        .def("GetLayoutBaselineDirectionAttr",
             &This::GetLayoutBaselineDirectionAttr)
        .def("CreateLayoutBaselineDirectionAttr",
             &_CreateLayoutBaselineDirectionAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetLayoutLinesStackDirectionAttr",
             &This::GetLayoutLinesStackDirectionAttr)
        .def("CreateLayoutLinesStackDirectionAttr",
             &_CreateLayoutLinesStackDirectionAttr,
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
