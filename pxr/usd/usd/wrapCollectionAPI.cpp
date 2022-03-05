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
#include "pxr/base/tf/pyAnnotatedBoolResult.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/python.hpp>

#include <string>


PXR_NAMESPACE_USING_DIRECTIVE

namespace {

#define WRAP_CUSTOM                                                     \
    template <class Cls> static void _CustomWrapCode(Cls &_class)

// fwd decl.
WRAP_CUSTOM;

        
static UsdAttribute
_CreateExpansionRuleAttr(UsdCollectionAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateExpansionRuleAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Token), writeSparsely);
}
        
static UsdAttribute
_CreateIncludeRootAttr(UsdCollectionAPI &self,
                                      boost::python::object defaultVal, bool writeSparsely) {
    return self.CreateIncludeRootAttr(
        UsdPythonToSdfType(defaultVal, SdfValueTypeNames->Bool), writeSparsely);
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
    std::string instanceName = self.GetName();
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

    boost::python::class_<This, boost::python::bases<UsdAPISchemaBase> >
        cls("CollectionAPI");

    cls
        .def(boost::python::init<UsdPrim, TfToken>())
        .def(boost::python::init<UsdSchemaBase const&, TfToken>())
        .def(TfTypePythonClass())

        .def("Get",
            (UsdCollectionAPI(*)(const UsdStagePtr &stage, 
                                       const SdfPath &path))
               &This::Get,
            (boost::python::arg("stage"), boost::python::arg("path")))
        .def("Get",
            (UsdCollectionAPI(*)(const UsdPrim &prim,
                                       const TfToken &name))
               &This::Get,
            (boost::python::arg("prim"), boost::python::arg("name")))
        .staticmethod("Get")

        .def("CanApply", &_WrapCanApply, (boost::python::arg("prim"), boost::python::arg("name")))
        .staticmethod("CanApply")

        .def("Apply", &This::Apply, (boost::python::arg("prim"), boost::python::arg("name")))
        .staticmethod("Apply")

        .def("GetSchemaAttributeNames",
             (const TfTokenVector &(*)(bool))&This::GetSchemaAttributeNames,
             boost::python::arg("includeInherited")=true,
             boost::python::return_value_policy<TfPySequenceToList>())
        .def("GetSchemaAttributeNames",
             (TfTokenVector(*)(bool, const TfToken &))
                &This::GetSchemaAttributeNames,
             boost::python::arg("includeInherited"),
             boost::python::arg("instanceName"),
             boost::python::return_value_policy<TfPySequenceToList>())
        .staticmethod("GetSchemaAttributeNames")

        .def("_GetStaticTfType", (TfType const &(*)()) TfType::Find<This>,
             boost::python::return_value_policy<boost::python::return_by_value>())
        .staticmethod("_GetStaticTfType")

        .def(!boost::python::self)

        
        .def("GetExpansionRuleAttr",
             &This::GetExpansionRuleAttr)
        .def("CreateExpansionRuleAttr",
             &_CreateExpansionRuleAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))
        
        .def("GetIncludeRootAttr",
             &This::GetIncludeRootAttr)
        .def("CreateIncludeRootAttr",
             &_CreateIncludeRootAttr,
             (boost::python::arg("defaultValue")=boost::python::object(),
              boost::python::arg("writeSparsely")=false))

        
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

#include <boost/python/tuple.hpp>

#include "pxr/usd/usd/collectionMembershipQuery.h"

namespace {

static boost::python::object _WrapValidate(const UsdCollectionAPI &coll) {
    std::string reason; 
    bool valid = coll.Validate(&reason);
    return boost::python::make_tuple(valid, reason);
}

WRAP_CUSTOM {

    boost::python::scope s_query = _class;

    using MQuery = UsdCollectionMembershipQuery;
    using This = UsdCollectionAPI;

    MQuery (This::*_ComputeMembershipQuery)() const = 
        &This::ComputeMembershipQuery;

    boost::python::scope collectionAPI = _class 
        .def(boost::python::init<UsdPrim, TfToken>())

        .def("GetCollection", 
             (UsdCollectionAPI(*)(const UsdPrim &prim, 
                                  const TfToken &name))
                &This::GetCollection,
             (boost::python::arg("prim"), boost::python::arg("name")))
        .def("GetCollection", 
             (UsdCollectionAPI(*)(const UsdStagePtr &stage, 
                                  const SdfPath &collectionPath))
                &This::GetCollection,
             (boost::python::arg("stage"), boost::python::arg("collectionPath")))
            .staticmethod("GetCollection")

        .def("GetAllCollections", &This::GetAllCollections, 
             boost::python::arg("prim"), boost::python::return_value_policy<TfPySequenceToList>())
            .staticmethod("GetAllCollections")

        .def("GetName", &This::GetName)
        .def("GetCollectionPath", &This::GetCollectionPath)

        .def("GetNamedCollectionPath", 
             &This::GetNamedCollectionPath,
             (boost::python::arg("prim"), boost::python::arg("collectionName")))
            .staticmethod("GetNamedCollectionPath")

        .def("IsSchemaPropertyBaseName", &This::IsSchemaPropertyBaseName,
            boost::python::arg("baseName"))
            .staticmethod("IsSchemaPropertyBaseName")

        .def("ComputeMembershipQuery", _ComputeMembershipQuery)

        .def("HasNoIncludedPaths", &This::HasNoIncludedPaths)

        .def("IncludePath", &This::IncludePath, boost::python::arg("pathToInclude"))
        .def("ExcludePath", &This::ExcludePath, boost::python::arg("pathToExclude"))

        .def("Validate", &_WrapValidate)

        .def("ComputeIncludedObjects", &This::ComputeIncludedObjects, 
             (boost::python::arg("query"), boost::python::arg("stage"), 
              boost::python::arg("predicate")=UsdPrimDefaultPredicate),
             boost::python::return_value_policy<TfPySequenceToList>())
             .staticmethod("ComputeIncludedObjects")

        .def("ComputeIncludedPaths", &This::ComputeIncludedPaths, 
             (boost::python::arg("query"), boost::python::arg("stage"), 
              boost::python::arg("predicate")=UsdPrimDefaultPredicate),
             boost::python::return_value_policy<TfPySequenceToList>())
             .staticmethod("ComputeIncludedPaths")

        .def("CanContainPropertyName", 
                This::CanContainPropertyName, boost::python::arg("name"))
        .staticmethod("CanContainPropertyName")

        .def("ResetCollection", &This::ResetCollection)
        .def("BlockCollection", &This::BlockCollection)
     ;
}

}
