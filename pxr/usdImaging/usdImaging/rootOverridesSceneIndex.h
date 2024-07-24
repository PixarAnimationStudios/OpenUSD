//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_ROOT_OVERRIDES_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_ROOT_OVERRIDES_SCENE_INDEX_H

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdImagingRootOverridesSceneIndex);

namespace UsdImagingRootOverridesSceneIndex_Impl
{
using _RootOverlayInfoSharedPtr = std::shared_ptr<struct _RootOverlayInfo>;
}

/// \class UsdImagingRootOverridesSceneIndex
///
/// Overrides some data sources on the root prim.
///
class UsdImagingRootOverridesSceneIndex
            : public HdSingleInputFilteringSceneIndexBase
{
public:
    USDIMAGING_API
    static
    UsdImagingRootOverridesSceneIndexRefPtr
    New(HdSceneIndexBaseRefPtr const &inputSceneIndex);

    USDIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    USDIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    USDIMAGING_API
    void SetRootTransform(const GfMatrix4d &);

    USDIMAGING_API
    const GfMatrix4d& GetRootTransform() const;

    USDIMAGING_API
    void SetRootVisibility(bool);

    USDIMAGING_API
    const bool GetRootVisibility() const;

protected:
    void _PrimsAdded(
        const HdSceneIndexBase&,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;
    void _PrimsDirtied(
        const HdSceneIndexBase&,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;
    void _PrimsRemoved(
        const HdSceneIndexBase&,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

private:
    UsdImagingRootOverridesSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex);

    UsdImagingRootOverridesSceneIndex_Impl::
    _RootOverlayInfoSharedPtr const _rootOverlayInfo;

    HdContainerDataSourceHandle const _rootOverlayDs;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
