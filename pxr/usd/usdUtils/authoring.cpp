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
#include "pxr/usd/usdUtils/authoring.h"

#include "pxr/base/gf/math.h"
#include "pxr/base/work/loops.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/schema.h"

#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usd/stage.h"

#include <vector>
#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

bool 
UsdUtilsCopyLayerMetadata(const SdfLayerHandle &source,
                          const SdfLayerHandle &destination,
                          bool skipSublayers, 
                          bool bakeUnauthoredFallbacks)
{
    if (!TF_VERIFY(source && destination))
        return false;

    SdfPrimSpecHandle sourcePseudo = source->GetPseudoRoot();
    SdfPrimSpecHandle destPseudo = destination->GetPseudoRoot();
    
    std::vector<TfToken> infoKeys = sourcePseudo->ListInfoKeys();
    std::vector<TfToken>::iterator last = infoKeys.end();
    
    if (skipSublayers){
        last = std::remove_if(infoKeys.begin(), last,
                              [](TfToken key) { return (key == SdfFieldKeys->SubLayers || key == SdfFieldKeys->SubLayerOffsets); });
    }

    for (auto key = infoKeys.begin(); key != last; ++key){
        destPseudo->SetInfo(*key, sourcePseudo->GetInfo(*key));
    }

    if (bakeUnauthoredFallbacks) {
        bool bakeColorConfiguration = 
            std::find(infoKeys.begin(), infoKeys.end(), 
                    SdfFieldKeys->ColorConfiguration) == infoKeys.end();
        bool bakeColorManagementSystem = 
            std::find(infoKeys.begin(), infoKeys.end(), 
                    SdfFieldKeys->ColorManagementSystem) == infoKeys.end();
        
        if (bakeColorConfiguration || bakeColorManagementSystem) {
            SdfAssetPath fallbackColorConfig;
            TfToken fallbackCms;
            
            UsdStage::GetColorConfigFallbacks(&fallbackColorConfig, 
                                              &fallbackCms);

            if (bakeColorConfiguration &&
                !fallbackColorConfig.GetAssetPath().empty()) {
                destPseudo->SetInfo(SdfFieldKeys->ColorConfiguration,
                                    VtValue(fallbackColorConfig));
            }
            if (bakeColorManagementSystem && !fallbackCms.IsEmpty()) {
                destPseudo->SetInfo(SdfFieldKeys->ColorManagementSystem, 
                                    VtValue(fallbackCms));
            }
        }
    }

    return true; 
}

// Helper method for determining the set of paths to exclude below the common
// ancestor, in order to include just includedRootPaths (and their ancestors).
static
SdfPathVector
_GetPathsToExcludeBelowCommonAncestor(
    const UsdUtilsPathHashSet &includedRootPaths,
    const UsdPrim &commonAncestor,
    const UsdUtilsPathHashSet &pathsToIgnore)
{
    SdfPathSet pathsToExclude;

    // Travere beneath the common prefix to find all the paths that don't 
    // belong to the collection. 
    UsdPrimRange commonAncestorRange(
        commonAncestor, UsdTraverseInstanceProxies(UsdPrimAllPrimsPredicate));
    for (auto primIt = commonAncestorRange.begin(); 
         primIt != commonAncestorRange.end() ; 
         ++primIt) 
    {
        SdfPath primPath = primIt->GetPath();

        if (pathsToIgnore.count(primPath)) {
            continue;
        }

        if (includedRootPaths.count(primPath) > 0) {
            // If we find a path that's included in the collection, we must 
            // remove all the ancestor paths of the path from pathsToExclude.
            SdfPath parentPath = primPath;
            while (parentPath != commonAncestor.GetPath()) {
                parentPath = parentPath.GetParentPath();
                pathsToExclude.erase(parentPath);
            }
            primIt.PruneChildren();
        } else {
            pathsToExclude.insert(primPath);
        }
    }

    // Remove all descendant paths of already excluded ancestor paths to come 
    // up with the mimimal set of paths to exclude below commonAncestor.
    SdfPathVector pathsToExcludeVec(pathsToExclude.begin(), 
                                    pathsToExclude.end());
    SdfPath::RemoveDescendentPaths(&pathsToExcludeVec);
    return pathsToExcludeVec;
}

static 
bool
_ComputePathsToIncludeAndExclude(
    const UsdUtilsPathHashSet &includedRootPaths, 
    const UsdPrim &commonAncestor,
    const double minInclusionRatio,
    const unsigned int maxNumExcludesBelowInclude,
    SdfPathVector *pathsToInclude,
    SdfPathVector *pathsToExclude,
    const UsdUtilsPathHashSet &pathsToIgnore)
{
    // XXX: performance
    // Note: the following code could be implemented as a single PreAndPostOrder 
    // traversal underneath the commonAncestor prim, which may be more performant
    // and likely use less memory. Until we have a use-case that reqired this to 
    // be super-efficient, we've decided to leave the implementation as-is, which 
    // might makes this easier to understand or debug.
    // 

    // Find the minimal set of paths that must be excluded, if we were 
    // to include all of the subtree rooted at commonPrefix.
    SdfPathVector pathsToExcludeBelowCommonAncestor = 
            _GetPathsToExcludeBelowCommonAncestor(
                includedRootPaths, commonAncestor, pathsToIgnore);

    SdfPath commonAncestorParentPath = commonAncestor.GetPath().GetParentPath();

    // At each path below the commonAncestor and at or above includedRootPaths,
    // compute the set of paths to be excluded if the path were to be 
    // included.
    using AncestorPathToExcludedPathsMap = 
        std::map<SdfPath, SdfPathVector>;
    AncestorPathToExcludedPathsMap excludedPathsMap;
    for (const SdfPath &pathToExclude : pathsToExcludeBelowCommonAncestor) {
        SdfPath parentPath = pathToExclude;
        while (parentPath != commonAncestorParentPath) {
            excludedPathsMap[parentPath].push_back(pathToExclude);
            parentPath = parentPath.GetParentPath();
        }
    }

    // At each path below the commonAncestor and at or above includedRootPaths,
    // compute the number of included paths.
    using AncestorPathToNumIncludesMap = std::map<SdfPath, size_t>;
    AncestorPathToNumIncludesMap numIncludedPathsMap;
    for (const SdfPath &includedRootPath : includedRootPaths) {
        SdfPath parentPath = includedRootPath;
        while (parentPath != commonAncestorParentPath) {
            ++numIncludedPathsMap[parentPath];
            parentPath = parentPath.GetParentPath();
        }
    }

    // We have all the information needed to compute the optimal set of 
    // included paths and excluded paths.
    UsdPrimRange commonAncestorRange(
        commonAncestor, UsdTraverseInstanceProxies(UsdPrimAllPrimsPredicate));
    for (auto primIt = commonAncestorRange.begin(); 
         primIt != commonAncestorRange.end() ; 
         ++primIt) 
    {
        const UsdPrim &p = *primIt;

        if (pathsToIgnore.count(p.GetPath())) {
            continue;
        }

        const auto inclMapIt = numIncludedPathsMap.find(p.GetPath());
        size_t inclPathCount = (inclMapIt != numIncludedPathsMap.end()) ? 
            inclMapIt->second : 0;

        if (inclPathCount > 0) {
            const auto exclMapIt = excludedPathsMap.find(p.GetPath());
            size_t exclPathCount = (exclMapIt != excludedPathsMap.end()) ?
                exclMapIt->second.size() : 0;

            double inclusionRatio = static_cast<double>(inclPathCount) / 
                                    (inclPathCount + exclPathCount);
            if (inclusionRatio >= minInclusionRatio && 
                exclPathCount <= maxNumExcludesBelowInclude) 
            {                    
                pathsToInclude->push_back(p.GetPath());
                if (exclPathCount > 0) {
                    const SdfPathVector &excludedPaths = excludedPathsMap[p.GetPath()];
                    pathsToExclude->insert(pathsToExclude->end(), 
                                           excludedPaths.begin(), 
                                           excludedPaths.end());
                }
                // Prune the subtree once an ancestor path has been included.
                primIt.PruneChildren();
            }
        } else {
            // Prune subtrees that don't have any included paths.
            primIt.PruneChildren();
        }
    }

    return true;
}

bool 
UsdUtilsComputeCollectionIncludesAndExcludes(
    const SdfPathSet &includedRootPaths, 
    const UsdStageWeakPtr &usdStage,
    SdfPathVector *pathsToInclude, 
    SdfPathVector *pathsToExclude,
    double minInclusionRatio /* 0.75 */,
    const unsigned int maxNumExcludesBelowInclude /* 5u */,
    const unsigned int minIncludeExcludeCollectionSize /* 3u */,
    const UsdUtilsPathHashSet &pathsToIgnore /*=UsdUtilsPathHashSet()*/)
{
    // Clear out the lists of paths that we're about to populate.
    pathsToInclude->clear();
    pathsToExclude->clear();

    if (minInclusionRatio <= 0 || minInclusionRatio > 1) {
        TF_CODING_ERROR("Invalid minInclusionRatio value: %f. Clamping value "
                        "to range (0, 1).", minInclusionRatio);
        minInclusionRatio = GfClamp(minInclusionRatio, 0, 1.0);
    }

    if (includedRootPaths.empty()) {
        return true;
    }

    // If the number of included paths is small (less than 
    // minIncludeExcludeCollectionSize), then create an includes-only
    // collection. If not, attempt to come up with a compact representation
    // for the collection with both included and excluded paths.
    if (includedRootPaths.size() < minIncludeExcludeCollectionSize) {
        pathsToInclude->insert(pathsToInclude->end(),
                               includedRootPaths.begin(), 
                               includedRootPaths.end());
        return true;
    }

    // Here's a quick summary of the algorithm used here 
    // [1] Find the common prefix of the paths included in the collection
    //     (commonPrefix) and get the corresponding prim (commonAncestor).
    // [2] Find the paths to exclude from the collection, if we were to 
    //     include all of commonPrefix (pathsToExcludeBelowCommonAncestor).
    //     [2a] Traverse all the paths below commonPrefix to find the 
    //          paths that don't belong to the collection. 
    // [3] For each of the included paths, walk up the namespace and compute
    //     a mapping of ancestor path to the number of included paths 
    //     (numIncludedPathsMap)
    // [4] For each of the excluded paths, walk up the namespace and compute
    //     a mapping of ancestor path to the set of paths to exclude if we were 
    //     to include the ancestor path in the collection (excludedPathsMap).
    // [5] Traverse the subtree rooted at "commonPrefix". At each path, 
    //     [5a] Determine the inclusion ratio (i.e the number of included 
    //          paths / (sum of included and excluded paths). 
    //     [5b] If the computed inclusionRatio > minInclusionRatio and if 
    //          the number of paths to exclude is <= maxNumExcludesBelowInclude, 
    //          add the current path path to the list of pathsToInclude and 
    //          add the corresponding excluded-path-set (from excludedPathsMap) 
    //          to the list of pathsToExclude.
    // [6] Return accumulated set of pathsToInclude and pathsToExclude.

    // Find the common prefix of all included paths in this collection.
    SdfPath commonPrefix = *includedRootPaths.begin();
    for (const auto &p : includedRootPaths) {
        commonPrefix = commonPrefix.GetCommonPrefix(p);
    }

    UsdPrim commonAncestor = usdStage->GetPrimAtPath(commonPrefix);
    if (!commonAncestor) {
        TF_CODING_ERROR("Could not get the prim at common-prefix path <%s>.", 
                        commonPrefix.GetText());
        return false;
    }

    // Construct and use a hash_set containing includedRootPaths
    // as we could (and in many cases will) be doing a lot of 
    // lookups in this set.
    UsdUtilsPathHashSet includedRootPathsHashSet;
    for (const auto &p: includedRootPaths) {
        includedRootPathsHashSet.insert(p);
    }

    return _ComputePathsToIncludeAndExclude(
            includedRootPathsHashSet, commonAncestor,
            minInclusionRatio, maxNumExcludesBelowInclude,
            pathsToInclude, pathsToExclude, pathsToIgnore);
}

UsdCollectionAPI
UsdUtilsAuthorCollection(
    const TfToken &collectionName, 
    const UsdPrim &usdPrim, 
    const SdfPathVector &pathsToInclude,
    const SdfPathVector &pathsToExclude)
{
    UsdCollectionAPI collection = UsdCollectionAPI::ApplyCollection(
        usdPrim, collectionName, UsdTokens->expandPrims);

    UsdRelationship includesRel = collection.CreateIncludesRel();
    includesRel.SetTargets(pathsToInclude);
    
    if (!pathsToExclude.empty()) {
        UsdRelationship excludesRel = collection.CreateExcludesRel();
        excludesRel.SetTargets(pathsToExclude);
    }

    return collection;
}


std::vector<UsdCollectionAPI> 
UsdUtilsCreateCollections(
    const std::vector<std::pair<TfToken, SdfPathSet>> &assignments,
    const UsdPrim &usdPrim,
    double minInclusionRatio /* 0.75 */,
    const unsigned int maxNumExcludesBelowInclude /* 5 */,
    const unsigned int minIncludeExcludeCollectionSize /* 2 */)
{
    std::vector<UsdCollectionAPI> result;

    if (assignments.empty()) {
        return result;
    }

    if (minInclusionRatio <= 0 || minInclusionRatio > 1) {
        TF_CODING_ERROR("Invalid minInclusionRatio value: %f. Clamping value "
                        "to range (0, 1).", minInclusionRatio);
        minInclusionRatio = GfClamp(minInclusionRatio, 0, 1.0);
    }

    const UsdStageWeakPtr usdStage = usdPrim.GetStage();  

    // Compute the set of collections to author in parallel.
    std::vector<std::pair<SdfPathVector, SdfPathVector>> 
        collectionIncludesAndExcludes(assignments.size(), 
            std::make_pair(SdfPathVector(), SdfPathVector()));

    WorkParallelForN(assignments.size(),
        [&](size_t start, size_t end) {
        for (size_t i = start ; i < end ; ++i) {
            const SdfPathSet &includedRootPaths = assignments[i].second;
            SdfPathVector &pathsToInclude = collectionIncludesAndExcludes[i].first;
            SdfPathVector &pathsToExclude = collectionIncludesAndExcludes[i].second;

            UsdUtilsComputeCollectionIncludesAndExcludes(includedRootPaths, 
                usdStage, &pathsToInclude, &pathsToExclude, minInclusionRatio,
                maxNumExcludesBelowInclude, minIncludeExcludeCollectionSize);
        }
    });

    // Do the authoring of the collections serially since we can't write from 
    // multiple threads in parallel.
    for (size_t i = 0; i < assignments.size() ; ++i) {
        const TfToken &collectionName = assignments[i].first;
        const auto &includesAndExcludes = collectionIncludesAndExcludes[i];

        UsdCollectionAPI coll = UsdUtilsAuthorCollection(collectionName, 
            usdPrim, includesAndExcludes.first, includesAndExcludes.second);
            
        result.push_back(coll);
    }

    return result;
}

USDUTILS_API
SdfLayerHandleVector 
UsdUtilsGetDirtyLayers(UsdStagePtr stage, bool includeClipLayers) {
    SdfLayerHandleVector usedLayers = stage->GetUsedLayers(includeClipLayers);
    auto newEnd = std::remove_if(
        usedLayers.begin(), usedLayers.end(),
        [](const SdfLayerHandle &layer) { return !layer->IsDirty(); });
    usedLayers.erase(newEnd, usedLayers.end());
    return usedLayers;
}

PXR_NAMESPACE_CLOSE_SCOPE

