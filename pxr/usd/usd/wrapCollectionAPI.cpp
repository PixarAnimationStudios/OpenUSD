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
_CreateExpansionRuleAttr(UsdCollectionAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateExpansionRuleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateIncludeRootAttr(UsdCollectionAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateIncludeRootAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
}
        
static UsdAttribute
_CreateMembershipExpressionAttr(UsdCollectionAPI &self,
                                      object defaultVal, bool writeSparsely) {
    return self.CreateMembershipExpressionAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->PathExpression), writeSparsely);
}
        
static UsdAttribute
_CreateCollectionAttr(UsdCollectionAPI &self,
                                      object defaultVal, bool writeSparsely) {
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

void wrapUsdCollectionAPI()
{
    typedef UsdCollectionAPI This;

    UsdCollectionAPI_CanApplyResult::Wrap<UsdCollectionAPI_CanApplyResult>(
        "_CanApplyResult", "whyNot");

    class_<This, bases<UsdAPISchemaBase> >
        cls("CollectionAPI");

    cls
        .def(init<UsdPrim, TfToken>((arg("prim"), arg("name"))))
        .def(init<UsdSchemaBase const&, TfToken>((arg("schemaObj"), arg("name"))))
        .def(TfTypePythonClass())

        .def("Get",
            (UsdCollectionAPI(*)(const UsdStagePtr &stage, 
                                       const SdfPath &path))
               &This::Get,
            (arg("stage"), arg("path")))
        .def("Get",
            (UsdCollectionAPI(*)(const UsdPrim &prim,
                                       const TfToken &name))
               &This::Get,
            (arg("prim"), arg("name")))
        .staticmethod("Get")

        .def("GetAll",
            (std::vector<UsdCollectionAPI>(*)(const UsdPrim &prim))
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

        
        .def("GetExpansionRuleAttr",
             &This::GetExpansionRuleAttr)
        .def("CreateExpansionRuleAttr",
             &_CreateExpansionRuleAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetIncludeRootAttr",
             &This::GetIncludeRootAttr)
        .def("CreateIncludeRootAttr",
             &_CreateIncludeRootAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetMembershipExpressionAttr",
             &This::GetMembershipExpressionAttr)
        .def("CreateMembershipExpressionAttr",
             &_CreateMembershipExpressionAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))
        
        .def("GetCollectionAttr",
             &This::GetCollectionAttr)
        .def("CreateCollectionAttr",
             &_CreateCollectionAttr,
             (arg("defaultValue")=object(),
              arg("writeSparsely")=false))

        
        .def("GetIncludesRel",
             &This::GetIncludesRel)
        .def("CreateIncludesRel",
             &This::CreateIncludesRel)
        
        .def("GetExcludesRel",
             &This::GetExcludesRel)
        .def("CreateExcludesRel",
             &This::CreateExcludesRel)
        .def("IsCollectionAPIPath", _WrapIsCollectionAPIPath)
            .staticmethod("IsCollectionAPIPath")
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

#include "pxr/external/boost/python/tuple.hpp"

#include "pxr/usd/usd/collectionMembershipQuery.h"

namespace {

static object _WrapValidate(const UsdCollectionAPI &coll) {
    std::string reason; 
    bool valid = coll.Validate(&reason);
    return pxr_boost::python::make_tuple(valid, reason);
}

WRAP_CUSTOM {

    scope s_query = _class;

    using MQuery = UsdCollectionMembershipQuery;
    using This = UsdCollectionAPI;

    MQuery (This::*_ComputeMembershipQuery)() const = 
        &This::ComputeMembershipQuery;

    scope collectionAPI = _class 
        .def(init<UsdPrim, TfToken>())

        .def("GetCollection", 
             (UsdCollectionAPI(*)(const UsdPrim &prim, 
                                  const TfToken &name))
                &This::GetCollection,
             (arg("prim"), arg("name")))
        .def("GetCollection", 
             (UsdCollectionAPI(*)(const UsdStagePtr &stage, 
                                  const SdfPath &collectionPath))
                &This::GetCollection,
             (arg("stage"), arg("collectionPath")))
            .staticmethod("GetCollection")

        .def("GetAllCollections", &This::GetAllCollections, 
             arg("prim"), return_value_policy<TfPySequenceToList>())
            .staticmethod("GetAllCollections")

        .def("GetName", &This::GetName)
        .def("GetCollectionPath", &This::GetCollectionPath)

        .def("GetNamedCollectionPath", 
             &This::GetNamedCollectionPath,
             (arg("prim"), arg("collectionName")))
            .staticmethod("GetNamedCollectionPath")

        .def("ResolveCompleteMembershipExpression",
             (SdfPathExpression (This::*)() const)
             &This::ResolveCompleteMembershipExpression)

        .def("IsSchemaPropertyBaseName", &This::IsSchemaPropertyBaseName,
            arg("baseName"))
            .staticmethod("IsSchemaPropertyBaseName")

        .def("ComputeMembershipQuery", _ComputeMembershipQuery)

        .def("HasNoIncludedPaths", &This::HasNoIncludedPaths)

        .def("IsInRelationshipsMode", &This::IsInRelationshipsMode)
        .def("IsInExpressionMode", &This::IsInExpressionMode)

        .def("IncludePath", &This::IncludePath, arg("pathToInclude"))
        .def("ExcludePath", &This::ExcludePath, arg("pathToExclude"))

        .def("Validate", &_WrapValidate)

        .def("ComputeIncludedObjects", &This::ComputeIncludedObjects, 
             (arg("query"), arg("stage"), 
              arg("predicate")=UsdPrimDefaultPredicate),
             return_value_policy<TfPySequenceToList>())
             .staticmethod("ComputeIncludedObjects")

        .def("ComputeIncludedPaths", &This::ComputeIncludedPaths, 
             (arg("query"), arg("stage"), 
              arg("predicate")=UsdPrimDefaultPredicate),
             return_value_policy<TfPySequenceToList>())
             .staticmethod("ComputeIncludedPaths")

        .def("CanContainPropertyName", 
                This::CanContainPropertyName, arg("name"))
        .staticmethod("CanContainPropertyName")

        .def("ResetCollection", &This::ResetCollection)
        .def("BlockCollection", &This::BlockCollection)
     ;
}

}
