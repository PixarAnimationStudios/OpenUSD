//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/return_value_policy.hpp"
#include "pxr/external/boost/python/tuple.hpp"

#include "pxr/usd/usdUtils/authoring.h"

#include "pxr/usd/sdf/layer.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"

PXR_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

static 
std::vector<UsdCollectionAPI> 
_WrapUsdUtilsCreateCollections(
    const pxr_boost::python::list &assignments, 
    const UsdPrim &usdPrim, 
    const double minInclusionRatio, 
    const unsigned int maxNumExcludesBelowInclude,
    const unsigned int minIncludeExcludeCollectionSize)
{
    // Create an stl vector with the required data from the python list.
    std::vector<std::pair<TfToken, SdfPathSet>> assignmentsVec;
    size_t nPairs = len(assignments);
    assignmentsVec.resize(nPairs);

    for(size_t i = 0; i < nPairs; ++i) {
        tuple pair = extract<tuple>(assignments[i]);
        TfToken collName = extract<TfToken>(pair[0]);
        list pathList = extract<list>(pair[1]);

        SdfPathSet includedPaths; 
        size_t numPaths = len(pathList);
        for (size_t pathIdx = 0; pathIdx < numPaths ; ++pathIdx) {
            SdfPath includedPath = extract<SdfPath>(pathList[pathIdx]);
            includedPaths.insert(includedPath);
        }
        assignmentsVec[i] = std::make_pair(collName, includedPaths);
    }

    return UsdUtilsCreateCollections(assignmentsVec, usdPrim, 
            minInclusionRatio, maxNumExcludesBelowInclude, 
            minIncludeExcludeCollectionSize);
}

static 
object
_WrapUsdUtilsComputeCollectionIncludesAndExcludes(
    const SdfPathSet &includedRootPaths, 
    const UsdStageWeakPtr &usdStage,
    double minInclusionRatio,
    const unsigned int maxNumExcludesBelowInclude,
    const unsigned int minIncludeExcludeCollectionSize,
    const SdfPathVector &pathsToIgnore)
{
    // The pathsToIgnore parameter is an SdfPathVector instead of an SdfPathSet
    // because we have to convert it into a hash set anyways. This lets us
    // accept both Python sets and lists, but without creating a temporary
    // std::set.
    UsdUtilsPathHashSet pathsToIgnoreSet;
    pathsToIgnoreSet.insert(pathsToIgnore.begin(), pathsToIgnore.end());

    SdfPathVector pathsToInclude;
    SdfPathVector pathsToExclude;

    UsdUtilsComputeCollectionIncludesAndExcludes(includedRootPaths, 
        usdStage, &pathsToInclude, &pathsToExclude, minInclusionRatio, 
        maxNumExcludesBelowInclude, minIncludeExcludeCollectionSize,
        pathsToIgnoreSet);

    return pxr_boost::python::make_tuple(pathsToInclude, pathsToExclude);
}

void wrapAuthoring()
{
    def("CopyLayerMetadata", UsdUtilsCopyLayerMetadata, 
        (arg("source"), arg("destination"), arg("skipSublayers") = false,
         arg("bakeUnauthoredFallbacks") = false ));

    def("ComputeCollectionIncludesAndExcludes", 
        &_WrapUsdUtilsComputeCollectionIncludesAndExcludes,
        (arg("includedRootPaths"), arg("usdStage"), 
         arg("minInclusionRatio")=0.75, 
         arg("maxNumExcludesBelowInclude")=5u,
         arg("minIncludeExcludeCollectionSize")=3u,
         arg("pathsToIgnore")=SdfPathVector()));

    def("AuthorCollection", UsdUtilsAuthorCollection, 
        (arg("collectionName"), arg("usdPrim"), arg("pathsToInclude"), 
         arg("pathsToExclude")=SdfPathSet()));

    def ("CreateCollections", _WrapUsdUtilsCreateCollections,
        pxr_boost::python::return_value_policy<TfPySequenceToList>(),
        (arg("assignments"), arg("usdPrim"), 
         arg("minInclusionRatio")=0.75, 
         arg("maxNumExcludesBelowInclude")=5u,
         arg("minIncludeExcludeCollectionSize")=3u));

    def("GetDirtyLayers", UsdUtilsGetDirtyLayers,
        (arg("stage"), arg("includeClipLayers")=true),
        return_value_policy<TfPySequenceToList>());
}
