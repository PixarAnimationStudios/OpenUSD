//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
/// - Provides the computed opinion for the 'active' and 'shutterInterval'
///   fields.
/// - Registers dependencies to invalidate the 'active' and 'shutterInterval'
///   locators.
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

    /// Public API
    HDSI_API
    static const SdfPath& GetFallbackPrimPath();

    HDSI_API
    static const SdfPath& GetRenderScope();

protected:
    HDSI_API
    HdsiRenderSettingsFilteringSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdContainerDataSourceHandle &inputArgs);

    /// Satisfying HdSingleInputFilteringSceneIndexBase
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
