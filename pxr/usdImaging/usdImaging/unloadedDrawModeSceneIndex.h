//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_USD_IMAGING_USD_IMAGING_UNLOADED_DRAW_MODE_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_UNLOADED_DRAW_MODE_SCENE_INDEX_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdImagingUnloadedDrawModeSceneIndex);

/// \class UsdImagingUnloadedDrawModeSceneIndex
///
/// A scene index that sets the draw mode for unloaded prims to
/// show bounding boxes.
///
class UsdImagingUnloadedDrawModeSceneIndex
        : public HdSingleInputFilteringSceneIndexBase
{
public:
    
    USDIMAGING_API
    static UsdImagingUnloadedDrawModeSceneIndexRefPtr
    New(HdSceneIndexBaseRefPtr const &inputSceneIndex);

    USDIMAGING_API
    ~UsdImagingUnloadedDrawModeSceneIndex() override;
    
    USDIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    USDIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    UsdImagingUnloadedDrawModeSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex);

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

#endif // PXR_USD_IMAGING_USD_IMAGING_UNLOADED_DRAW_MODE_SCENE_INDEX_H
