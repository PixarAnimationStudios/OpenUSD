//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef PXR_USD_IMAGING_USD_IMAGING_MATERIAL_BINDINGS_RESOLVING_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_MATERIAL_BINDINGS_RESOLVING_SCENE_INDEX_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdImagingMaterialBindingsResolvingSceneIndex);

/// Scene index that computes the resolved material binding for a given
/// purpose from the flattened direct material bindings and collection
/// material bindings.
///
/// \note Current support does not factor in collection bindings.
///
class UsdImagingMaterialBindingsResolvingSceneIndex
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    USDIMAGING_API
    static UsdImagingMaterialBindingsResolvingSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdContainerDataSourceHandle &inputArgs);

    USDIMAGING_API
    ~UsdImagingMaterialBindingsResolvingSceneIndex() override;
    
    USDIMAGING_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    USDIMAGING_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    UsdImagingMaterialBindingsResolvingSceneIndex(
        const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdContainerDataSourceHandle &inputArgs);

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

#endif // PXR_USD_IMAGING_USD_IMAGING_GL_MATERIAL_BINDINGS_RESOLVING_SCENE_INDEX_H
