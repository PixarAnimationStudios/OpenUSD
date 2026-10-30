//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdContrived/publicMultipleApplyAPI.h"
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

namespace nb = nanobind;

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateTestAttrOneAttr(UsdContrivedPublicMultipleApplyAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateTestAttrOneAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Int), writeSparsely);
}
        
static UsdAttribute
_CreateTestAttrTwoAttr(UsdContrivedPublicMultipleApplyAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateTestAttrTwoAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Double), writeSparsely);
}
        
static UsdAttribute
_CreatePublicAPIAttr(UsdContrivedPublicMultipleApplyAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreatePublicAPIAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Opaque), writeSparsely);
}

static bool _WrapIsPublicMultipleApplyAPIPath(const SdfPath &path) {
    TfToken collectionName;
    return UsdContrivedPublicMultipleApplyAPI::IsPublicMultipleApplyAPIPath(
        path, &collectionName);
}

static std::string
_Repr(const UsdContrivedPublicMultipleApplyAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    std::string instanceName = TfPyRepr(self.GetName());
    return TfStringPrintf(
        "UsdContrived.PublicMultipleApplyAPI(%s, '%s')",
        primRepr.c_str(), instanceName.c_str());
}

struct UsdContrivedPublicMultipleApplyAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdContrivedPublicMultipleApplyAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdContrivedPublicMultipleApplyAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim, const TfToken& name)
{
    std::string whyNot;
    bool result = UsdContrivedPublicMultipleApplyAPI::CanApply(prim, name, &whyNot);
    return UsdContrivedPublicMultipleApplyAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdContrivedPublicMultipleApplyAPI(nb::module_ &m)
{
    typedef UsdContrivedPublicMultipleApplyAPI This;

    UsdContrivedPublicMultipleApplyAPI_CanApplyResult::Wrap<UsdContrivedPublicMultipleApplyAPI_CanApplyResult>(
        m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase>
        cls(m, "PublicMultipleApplyAPI");

    cls
        .def(nb::init<UsdPrim, TfToken>(), nb::arg("prim")=UsdPrim(), nb::arg("name")=TfToken())
        .def(nb::init<UsdSchemaBase const&, TfToken>(), nb::arg("schemaObj"), nb::arg("name"))
        .def(TfTypePythonClass())

        .def_static("Get",
            (UsdContrivedPublicMultipleApplyAPI(*)(const UsdStagePtr &stage, 
                                       const SdfPath &path))
               &This::Get,
            nb::arg("stage"), nb::arg("path"))
        .def_static("Get",
            (UsdContrivedPublicMultipleApplyAPI(*)(const UsdPrim &prim,
                                       const TfToken &name))
               &This::Get,
            nb::arg("prim"), nb::arg("name"))

        .def_static("GetAll",
            (std::vector<UsdContrivedPublicMultipleApplyAPI>(*)(const UsdPrim &prim))
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

        
        .def("GetTestAttrOneAttr",
             &This::GetTestAttrOneAttr)
        .def("CreateTestAttrOneAttr",
             &_CreateTestAttrOneAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetTestAttrTwoAttr",
             &This::GetTestAttrTwoAttr)
        .def("CreateTestAttrTwoAttr",
             &_CreateTestAttrTwoAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetPublicAPIAttr",
             &This::GetPublicAPIAttr)
        .def("CreatePublicAPIAttr",
             &_CreatePublicAPIAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        .def_static("IsPublicMultipleApplyAPIPath", _WrapIsPublicMultipleApplyAPIPath)
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
