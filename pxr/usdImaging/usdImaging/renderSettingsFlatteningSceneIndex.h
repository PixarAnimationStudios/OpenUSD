//
// Copyright 2023 Pixar
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
