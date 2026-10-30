//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usd/collectionAPI.h"
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
_CreateExpansionRuleAttr(UsdCollectionAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateExpansionRuleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateIncludeRootAttr(UsdCollectionAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateIncludeRootAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateMembershipExpressionAttr(UsdCollectionAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateMembershipExpressionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->PathExpression), writeSparsely);
}
        
static UsdAttribute
_CreateModeAttr(UsdCollectionAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateModeAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateCollectionAttr(UsdCollectionAPI &self,
                                      nb::handle defaultVal, bool writeSparsely) {
    return self.CreateCollectionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Opaque), writeSparsely);
}

static bool _WrapIsCollectionAPIPath(const SdfPath &path) {
    TfToken collectionName;
    return UsdCollectionAPI::IsCollectionAPIPath(
        path, &collectionName);
}

static std::string
_Repr(const UsdCollectionAPI &self)
{
    std::string primRepr = TfPyRepr(self.GetPrim());
    std::string instanceName = TfPyRepr(self.GetName());
    return TfStringPrintf(
        "Usd.CollectionAPI(%s, '%s')",
        primRepr.c_str(), instanceName.c_str());
}

struct UsdCollectionAPI_CanApplyResult : 
    public TfPyAnnotatedBoolResult<std::string>
{
    UsdCollectionAPI_CanApplyResult(bool val, std::string const &msg) :
        TfPyAnnotatedBoolResult<std::string>(val, msg) {}
};

static UsdCollectionAPI_CanApplyResult
_WrapCanApply(const UsdPrim& prim, const TfToken& name)
{
    std::string whyNot;
    bool result = UsdCollectionAPI::CanApply(prim, name, &whyNot);
    return UsdCollectionAPI_CanApplyResult(result, whyNot);
}

} // anonymous namespace

void wrapUsdCollectionAPI(nb::module_ &m)
{
    typedef UsdCollectionAPI This;

    UsdCollectionAPI_CanApplyResult::WrapNb<UsdCollectionAPI_CanApplyResult>(m, "_CanApplyResult", "whyNot");

    nb::class_<This, UsdAPISchemaBase> cls(m, "CollectionAPI");

    cls
        .def(nb::init<UsdPrim, TfToken>(), nb::arg("prim")=UsdPrim(), nb::arg("name")=TfToken())
        .def(nb::init<UsdSchemaBase const&, TfToken>(), nb::arg("schemaObj"), nb::arg("name"))
        .def(TfTypePythonClass())

        .def_static("Get",
            (UsdCollectionAPI(*)(const UsdStagePtr &stage, 
                                       const SdfPath &path))
               &This::Get,
            nb::arg("stage"), nb::arg("path"))
        .def_static("Get",
            (UsdCollectionAPI(*)(const UsdPrim &prim,
                                       const TfToken &name))
               &This::Get,
            nb::arg("prim"), nb::arg("name"))

        .def_static("GetAll",
            (std::vector<UsdCollectionAPI>(*)(const UsdPrim &prim))
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

        
        .def("GetExpansionRuleAttr",
             &This::GetExpansionRuleAttr)
        .def("CreateExpansionRuleAttr",
             &_CreateExpansionRuleAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetIncludeRootAttr",
             &This::GetIncludeRootAttr)
        .def("CreateIncludeRootAttr",
             &_CreateIncludeRootAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetMembershipExpressionAttr",
             &This::GetMembershipExpressionAttr)
        .def("CreateMembershipExpressionAttr",
             &_CreateMembershipExpressionAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetModeAttr",
             &This::GetModeAttr)
        .def("CreateModeAttr",
             &_CreateModeAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)
        
        .def("GetCollectionAttr",
             &This::GetCollectionAttr)
        .def("CreateCollectionAttr",
             &_CreateCollectionAttr,
             nb::arg("defaultValue").none()=nb::none(),
              nb::arg("writeSparsely")=false)

        
        .def("GetIncludesRel",
             &This::GetIncludesRel)
        .def("CreateIncludesRel",
             &This::CreateIncludesRel)
        
        .def("GetExcludesRel",
             &This::GetExcludesRel)
        .def("CreateExcludesRel",
             &This::CreateExcludesRel)
        .def_static("IsCollectionAPIPath", _WrapIsCollectionAPIPath)
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

#include "pxr/usd/usd/collectionMembershipQuery.h"

namespace {

static nb::tuple _WrapValidate(const UsdCollectionAPI &coll) {
    std::string reason; 
    bool valid = coll.Validate(&reason);
    return nb::make_tuple(valid, reason);
}

WRAP_CUSTOM {

    using MQuery = UsdCollectionMembershipQuery;
    using This = UsdCollectionAPI;

    MQuery (This::*_ComputeMembershipQuery)() const = 
        &This::ComputeMembershipQuery;

    _class 
        .def(nb::init<UsdPrim, TfToken>())

        .def_static("GetCollection", 
             (UsdCollectionAPI(*)(const UsdPrim &prim, 
                                  const TfToken &name))
                &This::GetCollection,
             nb::arg("prim"), nb::arg("name"))
        .def_static("GetCollection", 
             (UsdCollectionAPI(*)(const UsdStagePtr &stage, 
                                  const SdfPath &collectionPath))
                &This::GetCollection,
             nb::arg("stage"), nb::arg("collectionPath"))

        .def_static("GetAllCollections", &This::GetAllCollections, 
             nb::arg("prim"),
             nb::rv_policy::copy)

        .def("GetName", &This::GetName)
        .def("GetCollectionPath", &This::GetCollectionPath)

        .def_static("GetNamedCollectionPath", 
             &This::GetNamedCollectionPath,
             nb::arg("prim"), nb::arg("collectionName"))

        .def("ResolveCompleteMembershipExpression",
             (SdfPathExpression (This::*)() const)
             &This::ResolveCompleteMembershipExpression)

        .def_static("IsSchemaPropertyBaseName", &This::IsSchemaPropertyBaseName,
            nb::arg("baseName"))

        .def("ComputeMembershipQuery", _ComputeMembershipQuery)

        .def("HasNoIncludedPaths", &This::HasNoIncludedPaths)

        .def("IsInRelationshipsMode", &This::IsInRelationshipsMode)
        .def("IsInExpressionMode", &This::IsInExpressionMode)

        .def("IncludePath", &This::IncludePath, nb::arg("pathToInclude"))
        .def("ExcludePath", &This::ExcludePath, nb::arg("pathToExclude"))

        .def("Validate", &_WrapValidate)

        .def_static("ComputeIncludedObjects", &This::ComputeIncludedObjects, 
             nb::arg("query"), nb::arg("stage"), 
              nb::arg("predicate")=UsdPrimDefaultPredicate,
             nb::rv_policy::copy)

        .def_static("ComputeIncludedPaths", &This::ComputeIncludedPaths, 
             nb::arg("query"), nb::arg("stage"), 
              nb::arg("predicate")=UsdPrimDefaultPredicate,
             nb::rv_policy::copy)

        .def_static("CanContainPropertyName", 
                This::CanContainPropertyName, nb::arg("name"))

        .def("ResetCollection", &This::ResetCollection)
        .def("BlockCollection", &This::BlockCollection)
     ;
}

}
