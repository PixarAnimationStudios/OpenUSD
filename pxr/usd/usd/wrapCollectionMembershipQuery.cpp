//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usd/collectionMembershipQuery.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/usd/usd/object.h"

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/stl/unordered_map.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

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
wrapUsdCollectionMembershipQuery(nanobind::module_ &m)
{
    m.def("ComputeIncludedObjectsFromCollection",
        [](const UsdCollectionMembershipQuery &query,
           const UsdStageWeakPtr &stage,
           const Usd_PrimFlagsPredicate &predicate) {
            return TfNbSequenceToList(
                UsdComputeIncludedObjectsFromCollection(query, stage, predicate));
        },
        nb::arg("query"), nb::arg("stage"),
        nb::arg("predicate")=UsdPrimDefaultPredicate);

    m.def("ComputeIncludedPathsFromCollection",
        [](const UsdCollectionMembershipQuery &query,
           const UsdStageWeakPtr &stage,
           const Usd_PrimFlagsPredicate &predicate) {
            return TfNbSequenceToList(
                UsdComputeIncludedPathsFromCollection(query, stage, predicate));
        },
        nb::arg("query"), nb::arg("stage"),
        nb::arg("predicate")=UsdPrimDefaultPredicate);

    m.def("ComputePathExpressionFromCollectionMembershipQueryRuleMap",
        &UsdComputePathExpressionFromCollectionMembershipQueryRuleMap,
        nb::arg("ruleMap"));

    nb::class_<UsdCollectionMembershipQuery>(m, "UsdCollectionMembershipQuery")
        .def(nb::init<>())
        .def("IsPathIncluded", _WrapIsPathIncluded_1, nb::arg("path"))
        .def("IsPathIncluded", _WrapIsPathIncluded_2, 
             nb::arg("path"), nb::arg("parentExpansionRule"))
        .def("HasExcludes", &UsdCollectionMembershipQuery::HasExcludes)
        .def("GetAsPathExpansionRuleMap",
             &UsdCollectionMembershipQuery::GetAsPathExpansionRuleMap)
        .def("GetIncludedCollections",
             [](const UsdCollectionMembershipQuery &self) {
                 return TfNbSequenceToList(self.GetIncludedCollections());
             })
        .def("UsesPathExpansionRuleMap",
             &UsdCollectionMembershipQuery::UsesPathExpansionRuleMap)
        .def("__hash__", &UsdCollectionMembershipQuery::GetHash)
        .def(nb::self == nb::self)
        .def(nb::self != nb::self)
        ;
}



