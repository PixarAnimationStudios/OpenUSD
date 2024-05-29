//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_NI_PROTOTYPE_PRUNING_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_IMAGING_NI_PROTOTYPE_PRUNING_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdImaging_NiPrototypePruningSceneIndex);

/// \class UsdImaging_NiPrototypePruningSceneIndex
///
/// Removes all prototypes (immediate children of the pseudo root with
/// the data source isUsdPrototype at returning true) from the scene index.
///
/// Intended to be used by the UsdImagingNiPrototypePropagatingSceneIndex
/// to obtain the Usd Stage without the prototypes.
///
class UsdImaging_NiPrototypePruningSceneIndex final
            : public HdSingleInputFilteringSceneIndexBase
{
public:
    static UsdImaging_NiPrototypePruningSceneIndexRefPtr New(
        HdSceneIndexBaseRefPtr const &inputSceneIndex)
    {
        return TfCreateRefPtr(
            new UsdImaging_NiPrototypePruningSceneIndex(
                inputSceneIndex));
    }

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

private:
    UsdImaging_NiPrototypePruningSceneIndex(
        HdSceneIndexBaseRefPtr const &inputSceneIndex);

    SdfPathSet _prototypes;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
