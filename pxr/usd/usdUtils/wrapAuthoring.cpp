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
#include "pxr/pxr.h"
#include <boost/python/def.hpp>
#include <boost/python/return_value_policy.hpp>
#include <boost/python/tuple.hpp>

#include "pxr/usd/usdUtils/authoring.h"

#include "pxr/usd/sdf/layer.h"

#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyContainerConversions.h"

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

static 
std::vector<UsdCollectionAPI> 
_WrapUsdUtilsCreateCollections(
    const boost::python::list &assignments, 
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

    return boost::python::make_tuple(pathsToInclude, pathsToExclude);
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
        boost::python::return_value_policy<TfPySequenceToList>(),
        (arg("assignments"), arg("usdPrim"), 
         arg("minInclusionRatio")=0.75, 
         arg("maxNumExcludesBelowInclude")=5u,
         arg("minIncludeExcludeCollectionSize")=3u));

    def("GetDirtyLayers", UsdUtilsGetDirtyLayers,
        (arg("stage"), arg("includeClipLayers")=true),
        return_value_policy<TfPySequenceToList>());
}
