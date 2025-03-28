//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usd/collectionMembershipQuery.h"

#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/usd/usd/object.h"

#include "pxr/external/boost/python.hpp"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

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

struct _PathExpansionRuleMapFromPython
{
    using RuleMap = Usd_CollectionMembershipQueryBase::PathExpansionRuleMap;
    
    _PathExpansionRuleMapFromPython() {
        converter::registry::insert(
            &convertible, &construct, type_id<RuleMap>());
    }
    
    static PyObject *convert(PyObject *p, RuleMap *result) {
        if (!PyDict_Check(p)) {
            return nullptr;
        }
        Py_ssize_t pos = 0;
        PyObject *pyKey = nullptr, *pyVal = nullptr;
        while (PyDict_Next(p, &pos, &pyKey, &pyVal)) {
            extract<SdfPath> keyProxy(pyKey);
            extract<TfToken> valProxy(pyVal);
            if (!keyProxy.check() || !valProxy.check()) {
                return nullptr;
            }
            object pVal(handle<>(borrowed(pyVal)));
            if (result) {
                (*result)[keyProxy()] = valProxy();
            }
        }
        return p;
    }

    static void *convertible(PyObject *p) {
        return convert(p, nullptr);
    }

    static void construct(PyObject *source,
                          converter::rvalue_from_python_stage1_data *data) {
        void *storage = (
            (converter::rvalue_from_python_storage<RuleMap>*)
            data)->storage.bytes;
        new (storage) RuleMap;
        data->convertible = storage;
        convert(source, (RuleMap *)storage);
    }
};

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

    def("ComputePathExpressionFromCollectionMembershipQueryRuleMap",
        UsdComputePathExpressionFromCollectionMembershipQueryRuleMap,
        arg("ruleMap"));

    class_<UsdCollectionMembershipQuery>("UsdCollectionMembershipQuery")
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
        .def("UsesPathExpansionRuleMap",
             &UsdCollectionMembershipQuery::UsesPathExpansionRuleMap)
        .def("__hash__", &UsdCollectionMembershipQuery::GetHash)
        .def(self == self)
        .def(self != self)
        ;

    // from-python conversion from PathExpansionRuleMap.
    TfPyContainerConversions::from_python_tuple_pair<
        Usd_CollectionMembershipQueryBase::PathExpansionRuleMap::value_type>();

    _PathExpansionRuleMapFromPython();
}


