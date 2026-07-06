//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_EXEC_IMAGING_STAGE_SCENE_INDEX_INTERFACE_H
#define PXR_USD_IMAGING_USD_EXEC_IMAGING_STAGE_SCENE_INDEX_INTERFACE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/usd/usd/timeCode.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdStage);
TF_DECLARE_REF_PTRS(UsdExecImagingStageSceneIndexInterface);

/// A scene index that provides values computed by exec.
///
/// UsdExecImagingStageSceneIndex is an initial scene index that has no
/// upstream scene indices. The scene index builds and maintains an exec request
/// for a UsdStage and provides data sources for prims when the values of those
/// data sources must be computed by exec. The UsdExecImagingStageSceneIndex
/// also sends PrimsDirtied notifications for prims when their computed values
/// are invalidated.
///
/// To obtain a concrete instance of the interface, clients must call
/// UsdExecImagingCreateStageSceneIndex. That method will return an object that
/// implements this interface, or a null pointer if usdExecImaging was built
/// with PXR_BUILD_EXEC=OFF.
///
class UsdExecImagingStageSceneIndexInterface : public HdSceneIndexBase
{
public:
    ~UsdExecImagingStageSceneIndexInterface();

    /// Sets the USD stage.
    ///
    /// The scene index constructs an exec system for the provided \p stage.
    /// This will reset all state held in the scene index.
    ///
    /// The scene index registers a listener for changes to the scene by
    /// virtue of creating an ExecUsdSystem for the stage. When scene changes
    /// occur, the invalidation notices are retained by the scene index and
    /// flushed to downstream scene indices on the next call to
    /// ApplyPendingUpdates.
    ///
    virtual void SetStage(UsdStageRefPtr stage) = 0;

    /// Sets the \p time at which computed values are evaluated.
    ///
    /// This calls PrimsDirtied for all prims with time-varying computed values.
    /// The updated values are recomputed on the next call to GetPrim or
    /// ApplyPendingUpdates.
    ///
    virtual void SetTime(UsdTimeCode time) = 0;

    /// Calls PrimsDirtied for computed values that have changed due to scene
    /// changes.
    ///
    /// When a scene change occurs that invalidates one or more computed values,
    /// the invalidation events are retained by this scene index until calling
    /// this method. The exec request is recomputed prior to calling
    /// PrimsDirtied.
    ///
    virtual void ApplyPendingUpdates() = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif