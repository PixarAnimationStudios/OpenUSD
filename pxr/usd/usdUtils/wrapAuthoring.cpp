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
    size_t nPairs = boost::python::len(assignments);
    assignmentsVec.resize(nPairs);

    for(size_t i = 0; i < nPairs; ++i) {
        boost::python::tuple pair = boost::python::extract<boost::python::tuple>(assignments[i]);
        TfToken collName = boost::python::extract<TfToken>(pair[0]);
        boost::python::list pathList = boost::python::extract<boost::python::list>(pair[1]);

        SdfPathSet includedPaths; 
        size_t numPaths = boost::python::len(pathList);
        for (size_t pathIdx = 0; pathIdx < numPaths ; ++pathIdx) {
            SdfPath includedPath = boost::python::extract<SdfPath>(pathList[pathIdx]);
            includedPaths.insert(includedPath);
        }
        assignmentsVec[i] = std::make_pair(collName, includedPaths);
    }

    return UsdUtilsCreateCollections(assignmentsVec, usdPrim, 
            minInclusionRatio, maxNumExcludesBelowInclude, 
            minIncludeExcludeCollectionSize);
}

static 
boost::python::object
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
    boost::python::def("CopyLayerMetadata", UsdUtilsCopyLayerMetadata, 
        (boost::python::arg("source"), boost::python::arg("destination"), boost::python::arg("skipSublayers") = false,
         boost::python::arg("bakeUnauthoredFallbacks") = false ));

    boost::python::def("ComputeCollectionIncludesAndExcludes", 
        &_WrapUsdUtilsComputeCollectionIncludesAndExcludes,
        (boost::python::arg("includedRootPaths"), boost::python::arg("usdStage"), 
         boost::python::arg("minInclusionRatio")=0.75, 
         boost::python::arg("maxNumExcludesBelowInclude")=5u,
         boost::python::arg("minIncludeExcludeCollectionSize")=3u,
         boost::python::arg("pathsToIgnore")=SdfPathVector()));

    boost::python::def("AuthorCollection", UsdUtilsAuthorCollection, 
        (boost::python::arg("collectionName"), boost::python::arg("usdPrim"), boost::python::arg("pathsToInclude"), 
         boost::python::arg("pathsToExclude")=SdfPathSet()));

    boost::python::def ("CreateCollections", _WrapUsdUtilsCreateCollections,
        boost::python::return_value_policy<TfPySequenceToList>(),
        (boost::python::arg("assignments"), boost::python::arg("usdPrim"), 
         boost::python::arg("minInclusionRatio")=0.75, 
         boost::python::arg("maxNumExcludesBelowInclude")=5u,
         boost::python::arg("minIncludeExcludeCollectionSize")=3u));

    boost::python::def("GetDirtyLayers", UsdUtilsGetDirtyLayers,
        (boost::python::arg("stage"), boost::python::arg("includeClipLayers")=true),
        boost::python::return_value_policy<TfPySequenceToList>());
}
