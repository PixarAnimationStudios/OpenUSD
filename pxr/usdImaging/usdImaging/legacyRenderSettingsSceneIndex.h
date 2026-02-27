//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_LEGACY_RENDER_SETTINGS_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_LEGACY_RENDER_SETTINGS_SCENE_INDEX_H

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

#include "pxr/pxr.h"

#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdImagingLegacyRenderSettingsSceneIndex);

/// UsdImagingLegacyRenderSettingsSceneIndex
///
/// A scene index to expose options set with SetRendererSetting()
/// into Hydra where downstream scene indices may query them or
/// respond to changes. Only sends dirty notices when an option's
/// value actually changes (checked using VtValue::operator ==).
class UsdImagingLegacyRenderSettingsSceneIndex final
  : public HdSingleInputFilteringSceneIndexBase
{
public:
    USDIMAGING_API
    static
    UsdImagingLegacyRenderSettingsSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex);

    USDIMAGING_API
    HdSceneIndexPrim
    GetPrim(const SdfPath& primPath) const override;

    USDIMAGING_API
    SdfPathVector
    GetChildPrimPaths(const SdfPath& primPath) const override;

    USDIMAGING_API
    void
    SetRenderSetting(
        const TfToken& id,
        const VtValue& value);

protected:
    void
    _PrimsAdded(
        const HdSceneIndexBase&,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;
    void
    _PrimsDirtied(
        const HdSceneIndexBase&,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;
    void
    _PrimsRemoved(
        const HdSceneIndexBase&,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    UsdImagingLegacyRenderSettingsSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex);

private:
    using _MapType = std::unordered_map<TfToken, VtValue, TfToken::HashFunctor>;
    using _EntryType = _MapType::value_type;

    void
    _Compose();

    _MapType _map;
    HdContainerDataSourceHandle _dataSource;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_LEGACY_RENDER_SETTINGS_SCENE_INDEX_H
