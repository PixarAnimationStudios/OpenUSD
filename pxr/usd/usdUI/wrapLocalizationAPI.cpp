//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdUI/localizationAPI.h"
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
_CreateLanguageAttr(UsdUILocalizationAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateLanguageAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}

static bool _WrapIsLocalizationAPIPath(const SdfPath &path) {
    TfToken collectionName;
    return UsdUILocalizationAPI::IsLocalizationAPIPath(
        path, &collectionName);
}

static std::string
_Repr(const UsdUILocalizationAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    std::string instanceName = TfPyRepr(self.GetName());
    return TfStringPrintf(
        "UsdUI.LocalizationAPI(%s, '%s')",
        primRepr.c_str(), instanceName.c_str());
}

struct UsdUILocalizationAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdUILocalizationAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdUILocalizationAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim, const TfToken& name)
{
    std::string whyNot;
    bool result = UsdUILocalizationAPI::CanApply(prim, name, &whyNot);
    return UsdUILocalizationAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdUILocalizationAPI()
{
    typedef UsdUILocalizationAPI This;

    UsdUILocalizationAPI_CanApplyResult::Wrap<UsdUILocalizationAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    class_<This, bases<UsdAPISchemaBase> >
        cls("LocalizationAPI");

    cls
        .def(init<UsdPrim, TfToken>((arg("prim"), arg("name")=UsdUITokens->default_)))
        .def(init<UsdSchemaBase const&, TfToken>((arg("schemaObj"), arg("name")=UsdUITokens->default_)))
        .def(TfTypePythonClass())

        .def("Get",
            (UsdUILocalizationAPI(*)(const UsdStagePtr &stage, 
                                       const SdfPath &path))
               &This::Get,
            (arg("stage"), arg("path")))
        .def("Get",
            (UsdUILocalizationAPI(*)(const UsdPrim &prim,
                                       const TfToken &name))
               &This::Get,
            (arg("prim"), arg("name")))
        .staticmethod("Get")

        .def("GetAll",
            (std::vector<UsdUILocalizationAPI>(*)(const UsdPrim &prim))
                &This::GetAll,
            arg("prim"),
            return_value_policy<TfPySequenceToList>())
        .staticmethod("GetAll")

        .def("CanApply", &_WrapCanApply, (arg("prim"), arg("name")))
        .staticmethod("CanApply")

        .def("Apply", &This::Apply, (arg("prim"), arg("name")=UsdUITokens->default_))
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

        
        .def("GetLanguageAttr",
             &This::GetLanguageAttr)
        .def("CreateLanguageAttr",
             &_CreateLanguageAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        .def("IsLocalizationAPIPath", _WrapIsLocalizationAPIPath)
            .staticmethod("IsLocalizationAPIPath")
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
    _class
        .def("GetDefaultProperty",  &UsdUILocalizationAPI::GetDefaultProperty, (arg("source")))
        .staticmethod("GetDefaultProperty")

        .def("GetPropertyLanguage", &UsdUILocalizationAPI::GetPropertyLanguage, (arg("prop")))
        .staticmethod("GetPropertyLanguage")

        .def("GetLocalizedPropertyName",
            &UsdUILocalizationAPI::GetLocalizedPropertyName,
            (arg("source"), arg("localization")))
        .staticmethod("GetLocalizedPropertyName")
    ;
}

}
