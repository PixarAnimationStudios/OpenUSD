//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_SELECTION_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_SELECTION_SCENE_INDEX_H

#include "pxr/usdImaging/usdImaging/api.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace UsdImagingSelectionSceneIndex_Impl
{
using _SelectionInfoSharedPtr = std::shared_ptr<struct _SelectionInfo>;
}

TF_DECLARE_REF_PTRS(UsdImagingSelectionSceneIndex);

/// \class UsdImagingSelectionSceneIndex
///
/// A simple scene index adding HdSelectionsSchema to all prims selected
/// with AddSelection.
///
class UsdImagingSelectionSceneIndex final
                          : public HdSingleInputFilteringSceneIndexBase
{
public:
    USDIMAGING_API
    static UsdImagingSelectionSceneIndexRefPtr New(
        HdSceneIndexBaseRefPtr const &inputSceneIndex);

    ~UsdImagingSelectionSceneIndex() override;

    USDIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    USDIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    /// Given a path (including usd proxy path inside a native instance) of
    /// a USD prim, determine the corresponding prim in the Usd scene index
    /// (filtered by the UsdImagingNiPrototypePropagatingSceneIndex) and
    /// populate its selections data source.
    USDIMAGING_API
    void AddSelection(const SdfPath &usdPath);

    /// Reset the scene index selection state.
    USDIMAGING_API
    void ClearSelection();

protected:
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
    UsdImagingSelectionSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex);

    UsdImagingSelectionSceneIndex_Impl::
    _SelectionInfoSharedPtr _selectionInfo;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
