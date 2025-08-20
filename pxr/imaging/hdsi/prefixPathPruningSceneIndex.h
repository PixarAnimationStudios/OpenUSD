//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_IMAGING_HDSI_PREFIX_PATH_PRUNING_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_PREFIX_PATH_PRUNING_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hdsi/api.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HDSI_PREFIX_PATH_PRUNING_SCENE_INDEX_TOKENS \
    (excludePathPrefixes)

TF_DECLARE_PUBLIC_TOKENS(HdsiPrefixPathPruningSceneIndexTokens, HDSI_API,
                         HDSI_PREFIX_PATH_PRUNING_SCENE_INDEX_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(HdsiPrefixPathPruningSceneIndex);

///
/// A scene index that prunes prims at or below the list of provided
/// prefix paths.
/// The list of prefix paths may be provided at c'tor time using a path vector
/// data source for the "excludePathPrefixes" locator, and updated
/// using the SetExcludePathPrefixes method.
///
/// \note
/// "Pruning" is an overloaded term in the context of scene indices and
/// deserves some clarification.
/// The pruning behavior of this scene index removes the subtree of prims 
/// rooted at the provided path prefixes. Thus, the topology of the input scene
/// is modified as a result of pruning.
/// Notices are also filtered to exclude entries for paths that are pruned.
///
/// While this scene index seems similar in nature to 
/// HdsiPrimTypeAndPathPruningSceneIndex and HdsiPrimTypePruningSceneIndex, it
/// differs from them in the pruning behavior. The former two scene indices
/// do not modify the topology of the input scene, but instead return an empty
/// prim type and prim container for a prim that is "pruned".
///
class HdsiPrefixPathPruningSceneIndex final
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiPrefixPathPruningSceneIndexRefPtr
    New(HdSceneIndexBaseRefPtr const &inputSceneIndex,
        HdContainerDataSourceHandle const &inputArgs);

public:
    // HdSceneIndex overrides
    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    // Public API
    HDSI_API
    void SetExcludePathPrefixes(SdfPathVector paths);

protected:
    // HdSingleInputFilteringSceneIndexBase overrides
    HDSI_API
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;
    HDSI_API
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;
    HDSI_API
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:
    HDSI_API
    HdsiPrefixPathPruningSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        HdContainerDataSourceHandle const &inputArgs);
    HDSI_API
    ~HdsiPrefixPathPruningSceneIndex() override;

    bool _IsPruned(const SdfPath &primPath) const;
    
    void _RemovePrunedChildren(SdfPathVector *childPaths) const;

    SdfPathVector _sortedExcludePaths;
    SdfPathSet _excludePathPrefixes;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
