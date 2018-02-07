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
#ifndef USDUTILS_AUTHORING_H_
#define USDUTILS_AUTHORING_H_

/// \file usdUtils/authoring.h
///
/// A collection of utilities for higher-level authoring and copying scene
/// description than provided by the core Usd and Sdf API's

#include "pxr/pxr.h"

#include "pxr/usd/usd/collectionAPI.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdUtils/api.h"
#include "pxr/usd/sdf/declareHandles.h"

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);

/// Given two layers \p source and \p destination, copy the authored metadata
/// from one to the other.  By default, copy **all** authored metadata;
/// however, you can skip certain classes of metadata with the parameter
/// \p skipSublayers, which will prevent copying subLayers or subLayerOffsets
///
/// Makes no attempt to clear metadata that may already be authored in
/// \p destination, but any fields that are already in \p destination but also
/// in \p source will be replaced.
///
/// Certain bits of layer metadata (eg. colorConfiguration and
/// colorManagementSystem) can have their fallback values specified in the
/// plugInfo.json files of plugins. When such metadata is unauthored in the
/// source layer, if \p bakeUnauthoredFallbacks is set to true, then the
/// fallback values are baked into the destination layer.
///
/// \return \c true on success, \c false on error.
USDUTILS_API
bool UsdUtilsCopyLayerMetadata(const SdfLayerHandle &source,
                               const SdfLayerHandle &destination,
                               bool skipSublayers = false,
                               bool bakeUnauthoredFallbacks = false);

/// \anchor UsdUtilsAuthoring_Collections
/// \name API for computing and authoring collections
/// @{

/// Computes the optimal set of paths to include and the set of paths to
/// exclude below includes paths, in order to encode an "expandPrims"
/// collection that contains the subtrees of prims rooted at 
/// \p includedRootPaths. 
/// 
/// The algorithm used to determine a compact representation is driven
/// by the following three parameters: \p minInclusionRatio, 
/// \p maxNumExcludesBelowInclude and \p minIncludeExcludeCollectionSize.
/// See below for their descriptions.
/// 
/// \p usdStage is the USD stage to which the paths in \p includedRootPaths
/// belong. 
/// \p pathsToInclude is populated with the set of paths to include. Any 
/// existing paths in the set are cleared before adding paths to it.
/// \p pathsToExclude is populated with the set of paths to exclude. Any 
/// existing paths in the set are cleared before adding paths to it.
/// \p minInclusionRatio is the minimum value of the ratio between the number 
/// of included paths and the sum of the number of included and excluded paths 
/// below an ancestor path, at or above which the ancestor path is included in 
/// the collection. For example, if an ancestor prim has four children and 
/// three out of the four are included in the collection, the inclusion ratio 
/// at the ancestor is 0.75. This value should be in the range (0,1), if not, 
/// it's clamped to the range.
/// \p maxNumExcludesBelowInclude is the maximum number of paths that we exclude
/// below any ancestor path that we include in a collection. This parameter only 
/// affects paths that have already passed the min-inclusion-ratio test. 
/// Setting this to 0 will cause all collections to have includes only (and no 
/// excludes). Setting it to a higher number will cause ancestor paths that are 
/// higher up in the namespace hierarchy to be included in collections.
/// \p minIncludeExcludeCollectionSize is the minimum size of a collection 
/// (i.e. the number of subtree-root paths included in it), at or
/// above which the algorithm chooses to make a collection with both included 
/// and excluded paths, instead of creating a collection with only includes
/// (containing the specified set of paths). \ref UsdCollectionAPI
/// 
/// Returns false if paths in \p includedRootPaths (or their common ancestor)
/// can't be found on the given \p usdStage.
/// parameters has an invalid value.
/// 
/// The python version of this function returns a tuple containing the 
/// two lists (pathsToInclude, pathsToExclude).
USDUTILS_API
bool UsdUtilsComputeCollectionIncludesAndExcludes(
    const SdfPathSet &includedRootPaths, 
    const UsdStageWeakPtr &usdStage,
    SdfPathVector *pathsToInclude, 
    SdfPathVector *pathsToExclude,
    double minInclusionRatio=0.75,
    const unsigned int maxNumExcludesBelowInclude=5u,
    const unsigned int minIncludeExcludeCollectionSize=3u);

/// Authors a collection named \p collectionName on the given prim, 
/// \p usdPrim with the given set of included paths (\p athsToInclude) 
/// and excluded paths (\p pathsToExclude).
/// 
/// If a collection with the specified name already exists on \p usdPrim, 
/// its data is appended to. The resulting collection will contain 
/// both the old paths and the newly included paths.
USDUTILS_API
UsdCollectionAPI UsdUtilsAuthorCollection(
    const TfToken &collectionName, 
    const UsdPrim &usdPrim, 
    const SdfPathVector &pathsToInclude,
    const SdfPathVector &pathsToExclude=SdfPathVector());

/// Given a vector of (collection-name, path-set) pairs, \p assignements, 
/// creates and returns a vector of collections that include subtrees of prims 
/// rooted at the included paths. The collections are created on the given prim, 
/// \p usdPrim. 
/// 
/// Based on the paths included in the various collections, this function 
/// computes a compact representation for each collection in parallel 
/// using UsdUtilsGetCollectionIncludesExcludes(). So, it takes the 
/// same set of parameters as that function: \p minInclusionRatio, 
/// \p maxNumExcludesBelowInclude and \p minIncludeExcludeCollectionSize. 
/// 
/// \note It is valid for the paths or subtrees specified in \p assignements 
/// to have overlapping subtrees. In this case the overlapping bits will belong 
/// to multiple collections. 
/// 
/// \p assignments is a vector of pairs representing collection names and paths 
/// to be included in the collection in each collection.
/// \p usdPrim is the prim on which the collections are created.
/// \p minInclusionRatio is the minimum value of the ratio between the number 
/// of included paths and the sum of the number of included and excluded paths 
/// below an ancestor path, at or above which the ancestor path is included in 
/// the collection. For example, if an ancestor prim has four children and 
/// three out of the four are included in the collection, the inclusion ratio 
/// at the ancestor is 0.75. This value should be in the range (0,1), if not, 
/// it's clamped to the range.
/// \p maxNumExcludesBelowInclude is the maximum number of paths that we exclude
/// below any ancestor path that we include in a collection. This parameter only 
/// affects paths that have already passed the min-inclusion-ratio test. 
/// Setting this to 0 will cause all collections to have includes only (and no 
/// excludes). Setting it to a higher number will cause ancestor paths that are 
/// higher up in the namespace hierarchy to be included in collections.
/// \p minIncludeExcludeCollectionSize is the minimum size of a collection 
/// (i.e. the number of subtree-root paths included in it), at or
/// above which the algorithm chooses to make a collection with both included 
/// and excluded paths, instead of creating a collection with only includes
/// (containing the specified set of paths). \ref UsdCollectionAPI
/// 
/// Returns the vector of UsdCollectionAPI objects that were created. 
/// If a collection is empty (i.e. includes no paths), then an empty collection
/// is created for it with the default expansionRule. Hence, the size of the 
/// returned vector should match the size of \p assignments.
USDUTILS_API
std::vector<UsdCollectionAPI> UsdUtilsCreateCollections(
    const std::vector<std::pair<TfToken, SdfPathSet>> &assignments,
    const UsdPrim &usdPrim,
    const double minInclusionRatio=0.75,
    const unsigned int maxNumExcludesBelowInclude=5u,
    const unsigned int minIncludeExcludeCollectionSize=3u);

/// @}

/// Retrieve a list of all dirty layers from the stage's UsedLayers.
USDUTILS_API
SdfLayerHandleVector UsdUtilsGetDirtyLayers(UsdStagePtr stage,
                                            bool includeClipLayers = true);

PXR_NAMESPACE_CLOSE_SCOPE

#endif /* USDUTILS_AUTHORING_H_ */
