//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/set.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/vector.h>

#include "pxr/usd/usdUtils/authoring.h"

#include "pxr/usd/sdf/layer.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

static SdfPathSet
_ExtractPathSet(const nb::iterable &paths)
{
    SdfPathSet result;
    for (nb::handle item : paths) {
        result.insert(nb::cast<SdfPath>(item));
    }
    return result;
}

static SdfPathVector
_ExtractPathVector(const nb::iterable &paths)
{
    SdfPathVector result;
    for (nb::handle item : paths) {
        result.push_back(nb::cast<SdfPath>(item));
    }
    return result;
}

static 
std::vector<UsdCollectionAPI> 
_WrapUsdUtilsCreateCollections(
    const nb::list &assignments, 
    const UsdPrim &usdPrim, 
    const double minInclusionRatio, 
    const unsigned int maxNumExcludesBelowInclude,
    const unsigned int minIncludeExcludeCollectionSize)
{
    // Create an stl vector with the required data from the python list.
    std::vector<std::pair<TfToken, SdfPathSet>> assignmentsVec;
    size_t nPairs = nb::len(assignments);
    assignmentsVec.resize(nPairs);

    for(size_t i = 0; i < nPairs; ++i) {
        nb::tuple pair = nb::cast<nb::tuple>(assignments[i]);
        TfToken collName = nb::cast<TfToken>(pair[0]);
        nb::iterable pathList = nb::cast<nb::iterable>(pair[1]);

        SdfPathSet includedPaths = _ExtractPathSet(pathList);
        assignmentsVec[i] = std::make_pair(collName, includedPaths);
    }

    return UsdUtilsCreateCollections(assignmentsVec, usdPrim, 
            minInclusionRatio, maxNumExcludesBelowInclude, 
            minIncludeExcludeCollectionSize);
}

static 
nb::tuple
_WrapUsdUtilsComputeCollectionIncludesAndExcludes(
    const nb::iterable &includedRootPaths, 
    const UsdStageWeakPtr &usdStage,
    double minInclusionRatio,
    const unsigned int maxNumExcludesBelowInclude,
    const unsigned int minIncludeExcludeCollectionSize,
    const nb::iterable &pathsToIgnore)
{
    // The pathsToIgnore parameter is an SdfPathVector instead of an SdfPathSet
    // because we have to convert it into a hash set anyways. This lets us
    // accept both Python sets and lists, but without creating a temporary
    // std::set.
    UsdUtilsPathHashSet pathsToIgnoreSet;
    for (nb::handle item : pathsToIgnore) {
        pathsToIgnoreSet.insert(nb::cast<SdfPath>(item));
    }

    SdfPathVector pathsToInclude;
    SdfPathVector pathsToExclude;

    UsdUtilsComputeCollectionIncludesAndExcludes(
        _ExtractPathSet(includedRootPaths), 
        usdStage, &pathsToInclude, &pathsToExclude, minInclusionRatio, 
        maxNumExcludesBelowInclude, minIncludeExcludeCollectionSize,
        pathsToIgnoreSet);

    return nb::make_tuple(pathsToInclude, pathsToExclude);
}

static UsdCollectionAPI
_WrapUsdUtilsAuthorCollection(
    const TfToken &collectionName, 
    const UsdPrim &usdPrim, 
    const nb::iterable &pathsToInclude,
    const nb::iterable &pathsToExclude)
{
    return UsdUtilsAuthorCollection(
        collectionName, usdPrim,
        _ExtractPathVector(pathsToInclude),
        _ExtractPathVector(pathsToExclude));
}

void wrapAuthoring(nanobind::module_ &m)
{
    m.def("CopyLayerMetadata", UsdUtilsCopyLayerMetadata, 
        nb::arg("source"), nb::arg("destination"), nb::arg("skipSublayers") = false,
        nb::arg("bakeUnauthoredFallbacks") = false);

    m.def("ComputeCollectionIncludesAndExcludes", 
        &_WrapUsdUtilsComputeCollectionIncludesAndExcludes,
        nb::arg("includedRootPaths"), nb::arg("usdStage"), 
        nb::arg("minInclusionRatio")=0.75, 
        nb::arg("maxNumExcludesBelowInclude")=5u,
        nb::arg("minIncludeExcludeCollectionSize")=3u,
        nb::arg("pathsToIgnore")=SdfPathVector());

    m.def("AuthorCollection", &_WrapUsdUtilsAuthorCollection, 
        nb::arg("collectionName"), nb::arg("usdPrim"), nb::arg("pathsToInclude"), 
        nb::arg("pathsToExclude")=SdfPathSet());

    m.def("CreateCollections", _WrapUsdUtilsCreateCollections,
        nb::arg("assignments"), nb::arg("usdPrim"), 
        nb::arg("minInclusionRatio")=0.75, 
        nb::arg("maxNumExcludesBelowInclude")=5u,
        nb::arg("minIncludeExcludeCollectionSize")=3u);

    m.def("GetDirtyLayers", UsdUtilsGetDirtyLayers,
        nb::arg("stage"), nb::arg("includeClipLayers")=true);
}
