//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_IMAGING_HDSI_PRIM_TYPE_PRUNING_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_PRIM_TYPE_PRUNING_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hdsi/api.h"
#include "pxr/usd/sdf/pathTable.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HDSI_PRIM_TYPE_PRUNING_SCENE_INDEX_TOKENS \
    (primTypes)                                   \
    (bindingToken)                                \
    (doNotPruneNonPrimPaths)

TF_DECLARE_PUBLIC_TOKENS(HdsiPrimTypePruningSceneIndexTokens, HDSI_API,
                         HDSI_PRIM_TYPE_PRUNING_SCENE_INDEX_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(HdsiPrimTypePruningSceneIndex);

/// Scene Index that prunes prims of given type (e.g., material) and
/// (optionally) bindings to that prim type (e.g., materialBindings).
///
/// Pruned prims are not removed from the scene index; instead, they
/// are given an empty primType and null dataSource.  This is to
/// preserve hierarchy and allow children of the pruned types to still
/// exist.
///
/// An optional bool argument specifies whether to suppress pruning for
/// prims at non-prim paths, and, correspondingly, leave bindings to
/// prims at non-prim paths unchanged.
///
/// By default, when creating the scene index, it is disabled and does
/// not pruning anything.
///
/// If an empty binding token is used, the scene index will not
/// prune any binding.
///
/// \deprecated Use HdsiSceneMaterialPruningSceneIndex or 
/// HdPrimTypeAndPathPruningSceneIndex with tautological PathPredicate instead.
///
class HdsiPrimTypePruningSceneIndex final
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiPrimTypePruningSceneIndexRefPtr
    New(HdSceneIndexBaseRefPtr const &inputSceneIndex,
        HdContainerDataSourceHandle const &inputArgs);

    /// Is scene index actually prunning?
    HDSI_API
    bool GetEnabled() const;
    /// Enable scene index to prune.
    HDSI_API
    void SetEnabled(bool);

public: // HdSceneIndex overrides
    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    const TfToken &GetBindingToken() const { return _bindingToken; }

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
    HdsiPrimTypePruningSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        HdContainerDataSourceHandle const &inputArgs);
    HDSI_API
    ~HdsiPrimTypePruningSceneIndex() override;

private:
    // Should prim be pruned based on its type?
    bool _PruneType(const TfToken &primType) const;
    // Should prim be pruned based on its path?
    bool _PrunePath(const SdfPath &path) const;

    const TfTokenVector _primTypes;
    const TfToken _bindingToken;
    const bool _doNotPruneNonPrimPaths;

    // Track pruned prims in a SdfPathTable.  A value of true
    // indicates a prim was filtered at that path.
    using _PruneMap = SdfPathTable<bool>;
    _PruneMap _pruneMap;

    bool _enabled;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
