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
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/wrapTypeHelpers.h"
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/tuple.h>

namespace nb = nanobind;

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateSoftwarePackageAttr(UsdMediaAuthorshipAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateSoftwarePackageAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateSoftwareVersionAttr(UsdMediaAuthorshipAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateSoftwareVersionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateDigitalSourceTypeAttr(UsdMediaAuthorshipAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateDigitalSourceTypeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateCreatorAttr(UsdMediaAuthorshipAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateCreatorAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->StringArray), writeSparsely);
}
        
static UsdAttribute
_CreateDescriptionAttr(UsdMediaAuthorshipAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateDescriptionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateInputsAttr(UsdMediaAuthorshipAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateInputsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->StringArray), writeSparsely);
}
        
static UsdAttribute
_CreateCreatedAttr(UsdMediaAuthorshipAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateCreatedAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateInstanceIDAttr(UsdMediaAuthorshipAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateInstanceIDAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateUsageTermsAttr(UsdMediaAuthorshipAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateUsageTermsAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->String), writeSparsely);
}
        
static UsdAttribute
_CreateCopyrightOwnerAttr(UsdMediaAuthorshipAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateCopyrightOwnerAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->StringArray), writeSparsely);
}
        
static UsdAttribute
_CreateContactAttr(UsdMediaAuthorshipAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
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

void wrapUsdMediaAuthorshipAPI(nb::module_ &m)
{
    typedef UsdMediaAuthorshipAPI This;

    UsdMediaAuthorshipAPI_CanApplyResult::WrapNb<UsdMediaAuthorshipAPI_CanApplyResult>(
        m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "AuthorshipAPI");

    cls
        .def(nb::init<UsdPrim, TfToken>(), nb::arg("prim")=UsdPrim(), nb::arg("name")=TfToken())
        .def(nb::init<UsdSchemaBase const&, TfToken>(), nb::arg("schemaObj"), nb::arg("name"))
        .def(TfTypePythonClass())

        .def_static("Get",
            (UsdMediaAuthorshipAPI(*)(const UsdStagePtr &stage, 
                                       const SdfPath &path))
               &This::Get,
            nb::arg("stage"), nb::arg("path"))
        .def_static("Get",
            (UsdMediaAuthorshipAPI(*)(const UsdPrim &prim,
                                       const TfToken &name))
               &This::Get,
            nb::arg("prim"), nb::arg("name"))

        .def_static("GetAll",
            (std::vector<UsdMediaAuthorshipAPI>(*)(const UsdPrim &prim))
                &This::GetAll,
            nb::arg("prim"),
            nb::rv_policy::copy)

        .def_static("CanApply", &_WrapCanApply, nb::arg("prim"), nb::arg("name"))

        .def_static("Apply", &This::Apply, nb::arg("prim"), nb::arg("name"))

        .def_static("GetSchemaAttributeNames",
             (const TfTokenVector &(*)(bool))&This::GetSchemaAttributeNames,
             nb::arg("includeInherited")=true,
             nb::rv_policy::copy)
        .def_static("GetSchemaAttributeNames",
             (TfTokenVector(*)(bool, const TfToken &))
                &This::GetSchemaAttributeNames,
             nb::arg("includeInherited"),
             nb::arg("instanceName"),
             nb::rv_policy::copy)

        .def_static("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             nb::rv_policy::copy)

        .def("__bool__", [](const This &self) { return bool(self); })

        
        .def("GetSoftwarePackageAttr",
             &This::GetSoftwarePackageAttr)
        .def("CreateSoftwarePackageAttr",
             &_CreateSoftwarePackageAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetSoftwareVersionAttr",
             &This::GetSoftwareVersionAttr)
        .def("CreateSoftwareVersionAttr",
             &_CreateSoftwareVersionAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetDigitalSourceTypeAttr",
             &This::GetDigitalSourceTypeAttr)
        .def("CreateDigitalSourceTypeAttr",
             &_CreateDigitalSourceTypeAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetCreatorAttr",
             &This::GetCreatorAttr)
        .def("CreateCreatorAttr",
             &_CreateCreatorAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetDescriptionAttr",
             &This::GetDescriptionAttr)
        .def("CreateDescriptionAttr",
             &_CreateDescriptionAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetInputsAttr",
             &This::GetInputsAttr)
        .def("CreateInputsAttr",
             &_CreateInputsAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetCreatedAttr",
             &This::GetCreatedAttr)
        .def("CreateCreatedAttr",
             &_CreateCreatedAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetInstanceIDAttr",
             &This::GetInstanceIDAttr)
        .def("CreateInstanceIDAttr",
             &_CreateInstanceIDAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetUsageTermsAttr",
             &This::GetUsageTermsAttr)
        .def("CreateUsageTermsAttr",
             &_CreateUsageTermsAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetCopyrightOwnerAttr",
             &This::GetCopyrightOwnerAttr)
        .def("CreateCopyrightOwnerAttr",
             &_CreateCopyrightOwnerAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)
        
        .def("GetContactAttr",
             &This::GetContactAttr)
        .def("CreateContactAttr",
             &_CreateContactAttr,
             nb::arg("defaultValue").none()=nb::none(),
             nb::arg("writeSparsely")=false)

        .def_static("IsAuthorshipAPIPath", _WrapIsAuthorshipAPIPath)
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

        .def_static("GetAllOnStage", &This::GetAllOnStage, nb::arg("stage"))

        .def_static("ComputeAccumulatedRecords", &This::ComputeAccumulatedRecords,
             nb::arg("prim"))

        .def_static("GetAllUnder", &This::GetAllUnder, nb::arg("prim"))

        .def_static("GetShadowed", &This::GetShadowed, nb::arg("prim"))

        .def_static("GetDuplicates", &This::GetDuplicates, nb::arg("prim"))

        .def_static("GetAllShadowed", &This::GetAllShadowed, nb::arg("stage"))

        .def_static("GetAllDuplicates", &This::GetAllDuplicates, nb::arg("stage"))

        .def_static("GetAllInLayer", &This::GetAllInLayer, nb::arg("layer"))
        ;
}

}
