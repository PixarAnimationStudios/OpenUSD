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
#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(HdsiSceneGlobalsSceneIndex);

// undefined -> 2 : Make SGSI a filtering scene index.
#define HDSI_SGSI_API_VERSION 2

/// \class HdsiSceneGlobalsSceneIndex
///
/// Scene index that populates the "sceneGlobals" data source as modeled
/// by HdSceneGlobalsSchema and provides public API to mutate it.
/// This provides a way for applications to control high-level scene behavior.
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
    
    /// Set the path to use as the primaryCameraPrim in the scene
    /// globals schema.
    HDSI_API
    void SetPrimaryCameraPrimPath(const SdfPath &);

    /// Set the frame number to use the as the currentFrame in the
    /// scene globals schema.
    HDSI_API
    void SetCurrentFrame(double);

    /// Set the timeCodesPerSecond to use the as the currentFrame in the
    /// scene globals schema.
    HDSI_API
    void SetTimeCodesPerSecond(double);

    /// Injects an arbitrary value that identifies the state of the input scene
    /// at that point in time. This value ends up in the render index scene
    /// globals once that state is processed by Hydra. This is useful for the
    /// client to identify when certain scene edits have been processed by
    /// Hydra.
    HDSI_API
    void SetSceneStateId(int);

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
    std::optional<SdfPath> _activeRenderSettingsPrimPath;
    std::optional<SdfPath> _primaryCameraPrimPath;
    double _time = std::numeric_limits<double>::quiet_NaN();
    double _timeCodesPerSecond = 24.0;
    int _sceneStateId = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDSI_SCENE_GLOBALS_SCENE_INDEX_H
