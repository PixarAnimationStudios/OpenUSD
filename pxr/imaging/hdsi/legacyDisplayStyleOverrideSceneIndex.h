//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_LEGACY_DISPLAY_STYLE_OVERRIDE_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_LEGACY_DISPLAY_STYLE_OVERRIDE_SCENE_INDEX_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdsi/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace HdsiLegacyDisplayStyleSceneIndex_Impl
{
struct _StyleInfo;
using _StyleInfoSharedPtr = std::shared_ptr<_StyleInfo>;
}

TF_DECLARE_REF_PTRS(HdsiLegacyDisplayStyleOverrideSceneIndex);

///
/// \class HdsiLegacyDisplayStyleOverrideSceneIndex
///
/// A scene index overriding the legacy display style for each prim.
/// So far, it only supports the refine level and cull style.
///
class HdsiLegacyDisplayStyleOverrideSceneIndex :
    public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiLegacyDisplayStyleOverrideSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex);

    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    /// A replacement for std::optional<int> that is not available until C++17.
    struct OptionalInt
    {
        bool hasValue = false;
        int value = 0;

        operator bool() const { return hasValue; }
        int operator*() const { return value; }
    };

    /// Sets the refine level (at data source locator displayStyle:refineLevel)
    /// for every prim in the input scene inedx.
    ///
    /// If an empty optional value is provided, a null data source will be
    /// returned for the data source locator.
    ///
    HDSI_API
    void SetRefineLevel(const OptionalInt &refineLevel);

    /// Sets the cullStyleFallback (at data source locator displayStyle:cullStyleFallback)
    /// for every prim in the input scene index.
    ///
    HDSI_API
    void SetCullStyleFallback(const TfToken &cullStyleFallback);

protected:
    HdsiLegacyDisplayStyleOverrideSceneIndex(
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

private:
    void _DirtyAllPrims(const HdDataSourceLocatorSet &locators);

    HdsiLegacyDisplayStyleSceneIndex_Impl::
    _StyleInfoSharedPtr const _styleInfo;

    /// Prim overlaj & underlay data sources.
    HdContainerDataSourceHandle const _overlayDs;
    HdContainerDataSourceHandle const _underlayDs;
};

HDSI_API
bool operator==(
    const HdsiLegacyDisplayStyleOverrideSceneIndex::OptionalInt &a,
    const HdsiLegacyDisplayStyleOverrideSceneIndex::OptionalInt &b);

HDSI_API
bool operator!=(
    const HdsiLegacyDisplayStyleOverrideSceneIndex::OptionalInt &a,
    const HdsiLegacyDisplayStyleOverrideSceneIndex::OptionalInt &b);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDSI_LEGACY_DISPLAY_STYLE_OVERRIDE_SCENE_INDEX_H
