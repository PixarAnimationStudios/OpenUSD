//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_IMAGING_HDSI_SCENE_MATERIAL_PRUNING_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_SCENE_MATERIAL_PRUNING_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hdsi/api.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HDSI_SCENE_MATERIAL_PRUNING_SCENE_INDEX_TOKENS \
    (builtinMaterialLocator)

TF_DECLARE_PUBLIC_TOKENS(HdsiSceneMaterialPruningSceneIndexTokens, HDSI_API,
                         HDSI_SCENE_MATERIAL_PRUNING_SCENE_INDEX_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(HdsiSceneMaterialPruningSceneIndex);

namespace HdsiSceneMaterialPruningSceneIndex_Impl
{

struct _Info;
using _InfoSharedPtr = std::shared_ptr<_Info>;

}

///
/// A scene index that prunes materials and material bindings.
///
/// The scene index can be disabled.
///
/// If enabled, prims of type material are prunned unless the bool data
/// source at a specified locator return true. The locator can be specified
/// by giving a builtinMaterialLocator to the inputArgs when constructing
/// the scene index.
///
/// Furthermore, for a prim that is not a material, the materialBindings are
/// prunned unless the bool data source for materialIsFinal of the
/// HdLegacyDisplayStyleSchema returns true.
///
/// Note that an alternative implementation could prune a material if the
/// all materialBindings targeting the material have been pruned.
/// However, we chose here to go for a simpler implementation and require
/// clients to tag materials as builtin.
///
class HdsiSceneMaterialPruningSceneIndex final
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiSceneMaterialPruningSceneIndexRefPtr
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

private:
    HDSI_API
    HdsiSceneMaterialPruningSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex,
        HdContainerDataSourceHandle const &inputArgs);
    HDSI_API
    ~HdsiSceneMaterialPruningSceneIndex() override;

    void _ProcessDirtiedEntryHelper(
        const HdSceneIndexObserver::DirtiedPrimEntry &entry,
        HdSceneIndexObserver::AddedPrimEntries *addedEntries);
    
    HdsiSceneMaterialPruningSceneIndex_Impl::_InfoSharedPtr const _info;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
