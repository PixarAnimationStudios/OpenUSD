//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_IMAGING_HDSI_MATERIAL_RENDER_CONTEXT_FILTERING_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_MATERIAL_RENDER_CONTEXT_FILTERING_SCENE_INDEX_H

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hdsi/api.h"

#include "pxr/pxr.h"

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdsiMaterialRenderContextFilteringSceneIndex);

/// Scene Index that filters the render contexts available for the data source
/// at the 'material' locator of a prim to the first render context encountered
/// in \p renderContextPriorityOrder, a vector of render context names in
/// descending order of preference, provided at construction time.
/// If no such render context is found, all render contexts are filtered out.
///
/// Since material networks may be authored for non-material prims (e.g. lights)
/// a predicate function may be provided at construction time via
/// \p typePredicateFn to restrict filtering to only those prim types for which
/// the predicate returns true.
/// If no predicate function is provided, filtering is applied to all prims.
///
/// Hydra renderers will typically instantiate this scene index via the scene
/// index plugin registration facility to filter in material networks for
/// render contexts they support.
///
/// \note
/// In the Hydra 1.0 API, the render delegate provides the list of supported
/// render contexts via HdRenderDelegate::GetMaterialRenderContexts().
/// This scene index provides a way to express this opinion for filtering
/// purposes, so that downstream scene indices that modify the material
/// network don't need to concern themselves about multiple render contexts.
///
/// This functionality is currently implemented in the
/// \ref HdSceneIndexAdapterSceneDelegate (backend emulation).
///
class HdsiMaterialRenderContextFilteringSceneIndex final
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    using TypePredicateFn = std::function<bool(const TfToken&)>;

    HDSI_API
    static HdsiMaterialRenderContextFilteringSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex,
        TfTokenVector renderContextPriorityOrder,
        TypePredicateFn typePredicateFn);

public: // HdSceneIndex overrides
    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override;
    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath& primPath) const override;

protected: // HdSingleInputFilteringSceneIndexBase overrides
    void _PrimsAdded(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::AddedPrimEntries& entries) override;
    void _PrimsRemoved(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::RemovedPrimEntries& entries) override;
    void _PrimsDirtied(
        const HdSceneIndexBase& sender,
        const HdSceneIndexObserver::DirtiedPrimEntries& entries) override;

protected:
    HdsiMaterialRenderContextFilteringSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex,
        TfTokenVector renderContextPriorityOrder,
        TypePredicateFn typePredicateFn);
    ~HdsiMaterialRenderContextFilteringSceneIndex() override;

private:
    const TfTokenVector _renderContextPriorityOrder;
    const TypePredicateFn _typePredicateFn;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
