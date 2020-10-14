//
// Copyright 2019 Pixar
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
#include "pxr/usd/usd/collectionMembershipQuery.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/usd/usd/object.h"

#include <boost/python.hpp>

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static bool _WrapIsPathIncluded_1(
    const UsdCollectionMembershipQuery &query,
    const SdfPath &path)
{
    return query.IsPathIncluded(path);
}

static bool _WrapIsPathIncluded_2(
    const UsdCollectionMembershipQuery &query,
    const SdfPath &path,
    const TfToken &parentExpansionRule)
{
    return query.IsPathIncluded(path, parentExpansionRule);
}

} // anonymous namespace

void
wrapUsdCollectionMembershipQuery()
{
    def("ComputeIncludedObjectsFromCollection",
        &UsdComputeIncludedObjectsFromCollection,
        (arg("query"), arg("stage"),
         arg("predicate")=UsdPrimDefaultPredicate),
        return_value_policy<TfPySequenceToList>());

    def("ComputeIncludedPathsFromCollection",
        &UsdComputeIncludedPathsFromCollection,
        (arg("query"), arg("stage"),
         arg("predicate")=UsdPrimDefaultPredicate),
        return_value_policy<TfPySequenceToList>());

    class_<UsdCollectionMembershipQuery>("UsdCollectionMembershipQuery")
        .def(init<>())
        .def("IsPathIncluded", _WrapIsPathIncluded_1, arg("path"))
        .def("IsPathIncluded", _WrapIsPathIncluded_2, 
             (arg("path"), arg("parentExpansionRule")))
        .def("HasExcludes", &UsdCollectionMembershipQuery::HasExcludes)
        .def("GetAsPathExpansionRuleMap",
             &UsdCollectionMembershipQuery::GetAsPathExpansionRuleMap,
             return_value_policy<TfPyMapToDictionary>())
        .def("GetIncludedCollections",
             &UsdCollectionMembershipQuery::GetIncludedCollections,
             return_value_policy<TfPySequenceToList>())
        .def("__hash__", &UsdCollectionMembershipQuery::GetHash)
        .def(self == self)
        .def(self != self)
        ;
}


