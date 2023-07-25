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
//     http://www.apache.org/licenses/LICEN SE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef PXR_IMAGING_HDSI_RENDER_SETTINGS_FILTERING_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_RENDER_SETTINGS_FILTERING_SCENE_INDEX_H

#include "pxr/imaging/hdsi/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

#include "pxr/base/vt/array.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HDSI_RENDER_SETTINGS_FILTERING_SCENE_INDEX_TOKENS \
    (namespacePrefixes)                                   \
    (fallbackPrimDs)

TF_DECLARE_PUBLIC_TOKENS(HdsiRenderSettingsFilteringSceneIndexTokens, HDSI_API,
    HDSI_RENDER_SETTINGS_FILTERING_SCENE_INDEX_TOKENS);

TF_DECLARE_WEAK_AND_REF_PTRS(HdsiRenderSettingsFilteringSceneIndex);

/// Scene index that provides the following functionality in service of the
/// HdRenderSettingsSchema:
/// - Filters the namespacedSettings based on the array of input prefixes 
///   (provided via the \p inputArgs constructor argument) that
///   are relevant to the renderer. An empty array implies no filtering.
/// - Registers a dependency on the sceneGlobals.activeRenderSettings locator
///   to invalidate the renderSetings.active locator.
/// - Determines whether the render settings prim is active.
/// - Optionally adds a fallback render settings prim whose container data
///   source is provided via the \p inputArgs constructor argument.
///
class HdsiRenderSettingsFilteringSceneIndex final :
    public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiRenderSettingsFilteringSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdContainerDataSourceHandle &inputArgs);

    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    // Public API
    HDSI_API
    static const SdfPath& GetFallbackPrimPath();

    HDSI_API
    static const SdfPath& GetRenderScope();

protected:
    HDSI_API
    HdsiRenderSettingsFilteringSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdContainerDataSourceHandle &inputArgs);

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
    const VtArray<TfToken> _namespacePrefixes;
    HdContainerDataSourceHandle _fallbackPrimDs;

    bool _addedFallbackPrim;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDSI_RENDER_SETTINGS_FILTERING_SCENE_INDEX_H
