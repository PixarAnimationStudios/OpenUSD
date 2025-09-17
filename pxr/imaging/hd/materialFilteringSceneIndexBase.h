//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef HD_MATERIAL_FILTERING_SCENE_INDEX_H
#define HD_MATERIAL_FILTERING_SCENE_INDEX_H


#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/materialNetworkInterface.h"

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(HdMaterialFilteringSceneIndexBase);

/// \class HdMaterialFilteringSceneIndexBase
///
/// Base class for implementing scene indices which read from and write to
/// only material network data sources. Subclasses implement only 
/// _GetFilteringFunction to provide a callback to run when a material network
/// is first queried.
class HdMaterialFilteringSceneIndexBase :
    public HdSingleInputFilteringSceneIndexBase
{
public:
    HD_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override final;

    HD_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override final;

    using FilteringFnc =
        std::function<void(HdMaterialNetworkInterface *)>;

    HD_API
    FilteringFnc GetFilteringFunction() const;

protected:
    virtual FilteringFnc _GetFilteringFunction() const = 0;

    HD_API
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override final;

    HD_API
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override final;

    HD_API
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override final;

    HD_API
    HdMaterialFilteringSceneIndexBase(
        const HdSceneIndexBaseRefPtr &inputSceneIndex);

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //HD_MATERIAL_FILTERING_SCENE_INDEX_H
