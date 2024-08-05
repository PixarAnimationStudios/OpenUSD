//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#ifndef EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_GLOBALS_SCENE_INDEX_H
#define EXT_RMANPKG_25_0_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_RILEY_GLOBALS_SCENE_INDEX_H

#include "pxr/pxr.h"
#include "hdPrman/sceneIndexObserverApi.h"

#ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#include "pxr/imaging/hd/filteringSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneGlobalsSchema;
TF_DECLARE_REF_PTRS(HdPrman_RileyGlobalsSceneIndex);

/// \class HdPrman_RileyGlobalsSceneIndex
///
/// A filtering scene index that adds a riley:globals prim populated with riley
/// options. The riley scene index observer will use these options when calling
/// Riley::SetOptions.
///
/// The options are generated from the HdSceneGlobalsSchema (particular using
/// the current frame) and from the namespaced settings on the current render
/// settings.
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
    HdContainerDataSourceHandle _GetRileyParams(
        HdSceneGlobalsSchema globalsSchema,
        const SdfPath &renderSettingsPath) const;
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

#endif // #ifdef HDPRMAN_USE_SCENE_INDEX_OBSERVER

#endif
