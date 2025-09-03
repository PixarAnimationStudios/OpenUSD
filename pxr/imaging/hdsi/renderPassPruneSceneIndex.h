//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_IMAGING_HDSI_RENDER_PASS_PRUNE_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_RENDER_PASS_PRUNE_SCENE_INDEX_H

#include "pxr/pxr.h"
#if PXR_VERSION >= 2408
#include "pxr/imaging/hd/collectionExpressionEvaluator.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hdsi/api.h"
#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(HdsiRenderPassPruneSceneIndex);

/// Applies prune rules of the scene globals' active render pass.
///
class HdsiRenderPassPruneSceneIndex : 
    public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiRenderPassPruneSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex);

    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    HDSI_API
    HdsiRenderPassPruneSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex);
    HDSI_API
    ~HdsiRenderPassPruneSceneIndex() override;

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
    // Prune state specified by a render pass.
    struct _RenderPassPruneState {
        SdfPath renderPassPath;

        // Retain the expression so we can compare old vs. new state.
        SdfPathExpression pruneExpr;

        // Evalulator for the pattern expression.
        std::optional<HdCollectionExpressionEvaluator> pruneEval;

        bool DoesPrune(const SdfPath &primPath) const;
    };

    // Pull on the scene globals schema for the active render pass,
    // computing and caching its prune state in _activeRenderPass.
    void _UpdateActiveRenderPassState(
        HdSceneIndexObserver::AddedPrimEntries *addedEntries,
        HdSceneIndexObserver::RemovedPrimEntries *removedEntries);

    // Prune state for the active render pass.
    _RenderPassPruneState _activeRenderPass;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
#endif // PXR_VERSION >= 2408
