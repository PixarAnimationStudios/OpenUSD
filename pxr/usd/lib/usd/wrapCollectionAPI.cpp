//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usd/usd/collectionAPI.h"
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

static bool _WrapIsCollectionAPIPath(const SdfPath &path) {
    TfToken collectionName;
    return UsdCollectionAPI::IsCollectionAPIPath(
        path, &collectionName);
}

} // anonymous namespace

void wrapUsdCollectionAPI()
{
    typedef UsdCollectionAPI This;

    class_<This, bases<UsdAPISchemaBase> >
        cls("CollectionAPI");

    cls
        .def(init<UsdPrim, TfToken>())
        .def(init<UsdSchemaBase const&, TfToken>())
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

        .def("GetSchemaAttributeNames",
             &This::GetSchemaAttributeNames,
             arg("includeInherited")=true,
             arg("instanceName")=TfToken(),
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

#include <boost/python/tuple.hpp>

namespace {

static object _WrapValidate(const UsdCollectionAPI &coll) {
    std::string reason; 
    bool valid = coll.Validate(&reason);
    return boost::python::make_tuple(valid, reason);
}

static bool _WrapIsPathIncluded_1(
    const UsdCollectionAPI::MembershipQuery &query, 
    const SdfPath &path)
{
    return query.IsPathIncluded(path);
}

static bool _WrapIsPathIncluded_2(
    const UsdCollectionAPI::MembershipQuery &query, 
    const SdfPath &path,
    const TfToken &parentExpansionRule)
{
    return query.IsPathIncluded(path, parentExpansionRule);
}

WRAP_CUSTOM {

    scope s_query = _class;

    using MQuery = UsdCollectionAPI::MembershipQuery;

    scope scope_mQuery = class_<MQuery>("MembershipQuery")
        .def(init<>())
        .def("IsPathIncluded", _WrapIsPathIncluded_1, arg("path"))
        .def("IsPathIncluded", _WrapIsPathIncluded_2, 
             (arg("path"), arg("parentExpansionRule")))
        .def("HasExcludes", &MQuery::HasExcludes)
        .def("GetAsPathExpansionRuleMap",
             &MQuery::GetAsPathExpansionRuleMap,
             return_value_policy<TfPyMapToDictionary>())
        .def("__hash__", &MQuery::GetHash)
        .def(self == self)
        .def(self != self)
        ;

    using This = UsdCollectionAPI;

    MQuery (This::*_ComputeMembershipQuery)() const = 
        &This::ComputeMembershipQuery;

    scope collectionAPI = _class 
        .def(init<UsdPrim, TfToken>())

        .def("ApplyCollection", &This::ApplyCollection, 
             (arg("prim"), arg("name"), 
              arg("expansionRule")=UsdTokens->expandPrims))
            .staticmethod("ApplyCollection")

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

        .def("IsSchemaPropertyBaseName", &This::IsSchemaPropertyBaseName,
            arg("baseName"))
            .staticmethod("IsSchemaPropertyBaseName")

        .def("ComputeMembershipQuery", _ComputeMembershipQuery)

        .def("HasNoIncludedPaths", &This::HasNoIncludedPaths)

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

        .def("ResetCollection", &This::ResetCollection)
        .def("BlockCollection", &This::BlockCollection)
     ;
}

}
