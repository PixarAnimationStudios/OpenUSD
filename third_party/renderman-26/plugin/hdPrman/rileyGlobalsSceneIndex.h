//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_GLOBALS_SCENE_INDEX_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_GLOBALS_SCENE_INDEX_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdPrman_RileyGlobalsSceneIndex);

/// \class HdPrman_RileyGlobalsSceneIndex
///
/// A scene index that inspects, e.g., HdSceneGlobalsSchema to
/// add a riley:globals prim that calls Riley::SetOptions.
///
class HdPrman_RileyGlobalsSceneIndex : 
    public HdSingleInputFilteringSceneIndexBase
{
public:
    static HdPrman_RileyGlobalsSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr& inputSceneIndex,
        const HdContainerDataSourceHandle &inputArgs);

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

private:
    HdContainerDataSourceHandle _GetRileyOptions() const;
    HdContainerDataSourceHandle _GetGlobalsPrimSource() const;
    
protected:
    HdPrman_RileyGlobalsSceneIndex(
        const HdSceneIndexBaseRefPtr& inputSceneIndex,
        const HdContainerDataSourceHandle &inputArgs);
    ~HdPrman_RileyGlobalsSceneIndex();

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

#endif
