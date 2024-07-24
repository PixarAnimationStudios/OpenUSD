//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_SCENE_GLOBALS_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_SCENE_GLOBALS_SCENE_INDEX_H

#include "pxr/imaging/hdsi/api.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(HdsiSceneGlobalsSceneIndex);

// undefined -> 2 : Make SGSI a filtering scene index.
#define HDSI_SGSI_API_VERSION 2

/// \class HdsiSceneGlobalsSceneIndex
///
/// Scene index that populates a "sceneGlobals" data source as modeled
/// by HdSceneGlobalsSchema and provides public API to mutate it.
///
class HdsiSceneGlobalsSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    HDSI_API
    static HdsiSceneGlobalsSceneIndexRefPtr
    New(const HdSceneIndexBaseRefPtr &inputSceneIndex);

    // ------------------------------------------------------------------------
    // Public (non-virtual) API
    // ------------------------------------------------------------------------
    
    /// Caches the provided path and notifies any observers when the active 
    /// render pass prim path is modified.
    ///
    HDSI_API
    void SetActiveRenderPassPrimPath(const SdfPath &);
    
    /// Caches the provided path and notifies any observers when the active 
    /// render settings prim path is modified.
    ///
    HDSI_API
    void SetActiveRenderSettingsPrimPath(const SdfPath &);

    ///
    HDSI_API
    void SetCurrentFrame(const double &);

    // ------------------------------------------------------------------------
    // Satisfying HdSceneIndexBase
    // ------------------------------------------------------------------------
    HDSI_API
    HdSceneIndexPrim
    GetPrim(const SdfPath &primPath) const override final;

    HDSI_API
    SdfPathVector
    GetChildPrimPaths(const SdfPath &primPath) const override final;

protected:
    HDSI_API
    HdsiSceneGlobalsSceneIndex(const HdSceneIndexBaseRefPtr &inputSceneIndex);

    // ------------------------------------------------------------------------
    // Satisfying HdSingleInputFilteringSceneIndexBase
    // ------------------------------------------------------------------------
    HDSI_API
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    HDSI_API
    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    HDSI_API
    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;

private:
    friend class _SceneGlobalsDataSource;

    SdfPath _activeRenderPassPrimPath;
    SdfPath _activeRenderSettingsPrimPath;
    double _time = std::numeric_limits<double>::quiet_NaN();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDSI_SCENE_GLOBALS_SCENE_INDEX_H
