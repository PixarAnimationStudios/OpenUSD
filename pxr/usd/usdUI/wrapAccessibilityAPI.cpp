//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdUI/accessibilityAPI.h"
#include "pxr/usd/usd/schemaBase.h"

#include "pxr/usd/sdf/primSpec.h"

#include "pxr/usd/usd/pyConversions.h"
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/external/boost/python.hpp"

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateLabelAttr(UsdUIAccessibilityAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateLabelAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateDescriptionAttr(UsdUIAccessibilityAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDescriptionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreatePriorityAttr(UsdUIAccessibilityAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePriorityAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}

static bool _WrapIsAccessibilityAPIPath(const SdfPath &path) {
    TfToken collectionName;
    return UsdUIAccessibilityAPI::IsAccessibilityAPIPath(
        path, &collectionName);
}

static std::string
_Repr(const UsdUIAccessibilityAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    std::string instanceName = TfPyRepr(self.GetName());
    return TfStringPrintf(
        "UsdUI.AccessibilityAPI(%s, '%s')",
        primRepr.c_str(), instanceName.c_str());
}

struct UsdUIAccessibilityAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdUIAccessibilityAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdUIAccessibilityAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim, const TfToken& name)
{
    std::string whyNot;
    bool result = UsdUIAccessibilityAPI::CanApply(prim, name, &whyNot);
    return UsdUIAccessibilityAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdUIAccessibilityAPI()
{
    typedef UsdUIAccessibilityAPI This;

    UsdUIAccessibilityAPI_CanApplyResult::Wrap<UsdUIAccessibilityAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    class_<This, bases<UsdAPISchemaBase> >
        cls("AccessibilityAPI");

    cls
        .def(init<UsdPrim, TfToken>((arg("prim"), arg("name"))))
        .def(init<UsdSchemaBase const&, TfToken>((arg("schemaObj"), arg("name"))))
        .def(TfTypePythonClass())

        .def("Get",
            (UsdUIAccessibilityAPI(*)(const UsdStagePtr &stage, 
                                       const SdfPath &path))
               &This::Get,
            (arg("stage"), arg("path")))
        .def("Get",
            (UsdUIAccessibilityAPI(*)(const UsdPrim &prim,
                                       const TfToken &name))
               &This::Get,
            (arg("prim"), arg("name")))
        .staticmethod("Get")

        .def("GetAll",
            (std::vector<UsdUIAccessibilityAPI>(*)(const UsdPrim &prim))
                &This::GetAll,
            arg("prim"),
            return_value_policy<TfPySequenceToList>())
        .staticmethod("GetAll")

        .def("CanApply", &_WrapCanApply, (arg("prim"), arg("name")))
        .staticmethod("CanApply")

        .def("Apply", &This::Apply, (arg("prim"), arg("name")))
        .staticmethod("Apply")

        .def("GetSchemaAttributeNames",
             (const TfTokenVector &(*)(bool))&This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             return_value_policy<TfPySequenceToList>())
        .def("GetSchemaAttributeNames",
             (TfTokenVector(*)(bool, const TfToken &))
                &This::GetSchemaAttributeNames,
             arg("includeInherited"),
             arg("instanceName"),
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             return_value_policy<return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!self)

        
        .def("GetLabelAttr",
             &This::GetLabelAttr)
        .def("CreateLabelAttr",
             &_CreateLabelAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDescriptionAttr",
             &This::GetDescriptionAttr)
        .def("CreateDescriptionAttr",
             &_CreateDescriptionAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPriorityAttr",
             &This::GetPriorityAttr)
        .def("CreatePriorityAttr",
             &_CreatePriorityAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        .def("IsAccessibilityAPIPath", _WrapIsAccessibilityAPIPath)
            .staticmethod("IsAccessibilityAPIPath")
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
    using This = UsdUIAccessibilityAPI;

    _class
    .def("CreateDefaultAPI",
            (This(*)(const UsdPrim &prim))
               &This::CreateDefaultAPI,
            (arg("prim")))
        .staticmethod("CreateDefaultAPI")

    .def("ApplyDefaultAPI",
            (This(*)(const UsdPrim &prim))
               &This::ApplyDefaultAPI,
            (arg("prim")))
        .staticmethod("ApplyDefaultAPI")
    ;
}

}
