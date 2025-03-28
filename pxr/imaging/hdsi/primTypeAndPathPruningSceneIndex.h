//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_IMAGING_HDSI_PRIM_TYPE_AND_PATH_PRUNING_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_PRIM_TYPE_AND_PATH_PRUNING_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hdsi/api.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HDSI_PRIM_TYPE_AND_PATH_PRUNING_SCENE_INDEX_TOKENS \
    (primTypes)

TF_DECLARE_PUBLIC_TOKENS(HdsiPrimTypeAndPathPruningSceneIndexTokens, HDSI_API,
                         HDSI_PRIM_TYPE_AND_PATH_PRUNING_SCENE_INDEX_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(HdsiPrimTypeAndPathPruningSceneIndex);

/// Scene Index that prunes prims if its type is in a given list and
/// its path matches a given predicate.
///
/// Pruned prims are not removed from the scene index; instead, they
/// are given an empty primType and null dataSource. This is to
/// preserve hierarchy and allow children of the pruned types to still
/// exist.
///
/// By default, the predicate is empty (empty std::function) and no prims
/// will be pruned.
///
class HdsiPrimTypeAndPathPruningSceneIndex final
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiPrimTypeAndPathPruningSceneIndexRefPtr
    New(HdSceneIndexBaseRefPtr const &inputSceneIndex,
        HdContainerDataSourceHandle const &inputArgs);

    using PathPredicate = std::function<bool(const SdfPath &)>;

    /// Set predicate returning true if a prim at a particular path
    /// should be pruned. Setting to an empty predicate means that
    /// no prim will be pruned.
    HDSI_API
    void SetPathPredicate(PathPredicate pathPredicate);

public: // HdSceneIndex overrides
    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected: // HdSingleInputFilteringSceneIndexBase overrides
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

protected:
    HDSI_API
    HdsiPrimTypeAndPathPruningSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        HdContainerDataSourceHandle const &inputArgs);
    HDSI_API
    ~HdsiPrimTypeAndPathPruningSceneIndex() override;

private:
    // Should prim be pruned based on its type?
    bool _PruneType(const TfToken &primType) const;

    const TfTokenVector _primTypes;

    PathPredicate _pathPredicate;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
