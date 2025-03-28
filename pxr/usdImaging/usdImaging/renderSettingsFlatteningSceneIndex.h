//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_USD_IMAGING_USD_IMAGING_RENDER_SETTINGS_FLATTENING_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_RENDER_SETTINGS_FLATTENING_SCENE_INDEX_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdImagingRenderSettingsFlatteningSceneIndex);

///
/// Stateless scene index that adds a flattened render settings representation
/// (as modeled by HdRenderSettingsSchema) for downstream consumption by a Hydra
/// render backend, and adds dependencies from the settings prim to
/// the targeted products and vars (using HdDependenciesSchema) so that change
/// notices are forwarded back to appropriate locators on the flattened data
/// source.
///
class UsdImagingRenderSettingsFlatteningSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    USDIMAGING_API
    static UsdImagingRenderSettingsFlatteningSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex);

    USDIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    USDIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    UsdImagingRenderSettingsFlatteningSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex);

    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_RENDER_SETTINGS_FLATTENING_SCENE_INDEX_H
