//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdMedia/authorshipAPI.h"
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
_CreateSoftwarePackageAttr(UsdMediaAuthorshipAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateSoftwarePackageAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateSoftwareVersionAttr(UsdMediaAuthorshipAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateSoftwareVersionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateDigitalSourceTypeAttr(UsdMediaAuthorshipAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDigitalSourceTypeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateCreatorAttr(UsdMediaAuthorshipAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCreatorAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->StringArray), writeSparsely);
}
        
static UsdAttribute
_CreateDescriptionAttr(UsdMediaAuthorshipAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateDescriptionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreatePromptInputNamesAttr(UsdMediaAuthorshipAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePromptInputNamesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->StringArray), writeSparsely);
}
        
static UsdAttribute
_CreatePromptInputValuesAttr(UsdMediaAuthorshipAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreatePromptInputValuesAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->StringArray), writeSparsely);
}
        
static UsdAttribute
_CreateCreatedAttr(UsdMediaAuthorshipAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCreatedAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateInstanceIDAttr(UsdMediaAuthorshipAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateInstanceIDAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateUsageTermsAttr(UsdMediaAuthorshipAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateUsageTermsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateCopyrightOwnerAttr(UsdMediaAuthorshipAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateCopyrightOwnerAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->StringArray), writeSparsely);
}
        
static UsdAttribute
_CreateContactAttr(UsdMediaAuthorshipAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateContactAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->StringArray), writeSparsely);
}

static bool _WrapIsAuthorshipAPIPath(const SdfPath &path) {
    TfToken collectionName;
    return UsdMediaAuthorshipAPI::IsAuthorshipAPIPath(
        path, &collectionName);
}

static std::string
_Repr(const UsdMediaAuthorshipAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    std::string instanceName = TfPyRepr(self.GetName());
    return TfStringPrintf(
        "UsdMedia.AuthorshipAPI(%s, '%s')",
        primRepr.c_str(), instanceName.c_str());
}

struct UsdMediaAuthorshipAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdMediaAuthorshipAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdMediaAuthorshipAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim, const TfToken& name)
{
    std::string whyNot;
    bool result = UsdMediaAuthorshipAPI::CanApply(prim, name, &whyNot);
    return UsdMediaAuthorshipAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdMediaAuthorshipAPI()
{
    typedef UsdMediaAuthorshipAPI This;

    UsdMediaAuthorshipAPI_CanApplyResult::Wrap<UsdMediaAuthorshipAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    class_<This, bases<UsdAPISchemaBase> >
        cls("AuthorshipAPI");

    cls
        .def(init<UsdPrim, TfToken>((arg("prim"), arg("name"))))
        .def(init<UsdSchemaBase const&, TfToken>((arg("schemaObj"), arg("name"))))
        .def(TfTypePythonClass())

        .def("Get",
            (UsdMediaAuthorshipAPI(*)(const UsdStagePtr &stage, 
                                       const SdfPath &path))
               &This::Get,
            (arg("stage"), arg("path")))
        .def("Get",
            (UsdMediaAuthorshipAPI(*)(const UsdPrim &prim,
                                       const TfToken &name))
               &This::Get,
            (arg("prim"), arg("name")))
        .staticmethod("Get")

        .def("GetAll",
            (std::vector<UsdMediaAuthorshipAPI>(*)(const UsdPrim &prim))
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

        
        .def("GetSoftwarePackageAttr",
             &This::GetSoftwarePackageAttr)
        .def("CreateSoftwarePackageAttr",
             &_CreateSoftwarePackageAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetSoftwareVersionAttr",
             &This::GetSoftwareVersionAttr)
        .def("CreateSoftwareVersionAttr",
             &_CreateSoftwareVersionAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDigitalSourceTypeAttr",
             &This::GetDigitalSourceTypeAttr)
        .def("CreateDigitalSourceTypeAttr",
             &_CreateDigitalSourceTypeAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCreatorAttr",
             &This::GetCreatorAttr)
        .def("CreateCreatorAttr",
             &_CreateCreatorAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetDescriptionAttr",
             &This::GetDescriptionAttr)
        .def("CreateDescriptionAttr",
             &_CreateDescriptionAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPromptInputNamesAttr",
             &This::GetPromptInputNamesAttr)
        .def("CreatePromptInputNamesAttr",
             &_CreatePromptInputNamesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetPromptInputValuesAttr",
             &This::GetPromptInputValuesAttr)
        .def("CreatePromptInputValuesAttr",
             &_CreatePromptInputValuesAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCreatedAttr",
             &This::GetCreatedAttr)
        .def("CreateCreatedAttr",
             &_CreateCreatedAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetInstanceIDAttr",
             &This::GetInstanceIDAttr)
        .def("CreateInstanceIDAttr",
             &_CreateInstanceIDAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetUsageTermsAttr",
             &This::GetUsageTermsAttr)
        .def("CreateUsageTermsAttr",
             &_CreateUsageTermsAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCopyrightOwnerAttr",
             &This::GetCopyrightOwnerAttr)
        .def("CreateCopyrightOwnerAttr",
             &_CreateCopyrightOwnerAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetContactAttr",
             &This::GetContactAttr)
        .def("CreateContactAttr",
             &_CreateContactAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        .def("IsAuthorshipAPIPath", _WrapIsAuthorshipAPIPath)
            .staticmethod("IsAuthorshipAPIPath")
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
    using This = UsdMediaAuthorshipAPI;

    _class
        .def("GetName", &This::GetName)

        .def("GetAllOnStage", &This::GetAllOnStage,
             (arg("stage"), arg("searchPrimStack") = false),
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetAllOnStage")

        .def("ComputeAccumulatedRecords", &This::ComputeAccumulatedRecords,
             (arg("prim"), arg("searchPrimStack") = false),
             return_value_policy<TfPySequenceToList>())
        .staticmethod("ComputeAccumulatedRecords")

        .def("GetAllUnder", &This::GetAllUnder,
             (arg("prim"), arg("searchPrimStack") = false),
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetAllUnder")

        .def("GetAllInLayer", &This::GetAllInLayer, arg("layer"),
             return_value_policy<TfPySequenceToList>())
        .staticmethod("GetAllInLayer")
        ;
}

}
