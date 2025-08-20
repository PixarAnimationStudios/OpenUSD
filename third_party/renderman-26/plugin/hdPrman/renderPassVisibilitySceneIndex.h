//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_PASS_VISIBILITY_SCENE_INDEX_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RENDER_PASS_VISIBILITY_SCENE_INDEX_H

#include "pxr/pxr.h"
#if PXR_VERSION >= 2408
#include "pxr/imaging/hd/collectionExpressionEvaluator.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(HdPrman_RenderPassVisibilitySceneIndex);

/// Applies visibility and matte rules of the scene globals' active render pass.
///
class HdPrman_RenderPassVisibilitySceneIndex : 
    public HdSingleInputFilteringSceneIndexBase
{
public:
    static HdPrman_RenderPassVisibilitySceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex);

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    HdPrman_RenderPassVisibilitySceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex);
    ~HdPrman_RenderPassVisibilitySceneIndex();

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:
    friend class HdPrman_RenderPassVis_PrimDataSource;

    // Visibility state specified by a render pass.
    struct _RenderPassVisState {
        SdfPath renderPassPath;

        // Retain the expressions so we can compare old vs. new state.
        SdfPathExpression matteExpr;
        SdfPathExpression renderVisExpr;
        SdfPathExpression cameraVisExpr;

        // Evalulators for each pattern expression.
        std::optional<HdCollectionExpressionEvaluator> matteEval;
        std::optional<HdCollectionExpressionEvaluator> renderVisEval;
        std::optional<HdCollectionExpressionEvaluator> cameraVisEval;

        bool DoesOverrideMatte(
            const SdfPath &primPath,
            HdSceneIndexPrim const& prim) const;
        bool DoesOverrideVis(
            const SdfPath &primPath,
            HdSceneIndexPrim const& prim) const;
        bool DoesOverrideCameraVis(
            const SdfPath &primPath,
            HdSceneIndexPrim const& prim) const;
    };

    // Pull on the scene globals schema for the active render pass,
    // computing and caching its visibility state in _activeRenderPass.
    void _UpdateActiveRenderPassState(
        HdSceneIndexObserver::DirtiedPrimEntries *dirtyEntries);

    // Visibility state for the active render pass.
    _RenderPassVisState _activeRenderPass;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
#endif // PXR_VERSION >= 2408
