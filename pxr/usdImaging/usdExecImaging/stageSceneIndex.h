//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_EXEC_IMAGING_STAGE_SCENE_INDEX_H
#define PXR_USD_IMAGING_USD_EXEC_IMAGING_STAGE_SCENE_INDEX_H

/// \file

#include "pxr/pxr.h"

#include "pxr/usdImaging/usdExecImaging/request.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndexInterface.h"


PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdExecImaging_StageSceneIndex);

/// A scene index that provides values computed by exec.
///
/// UsdExecImaging_StageSceneIndex is an initial scene index that has no
/// upstream scene indices. The scene index builds and maintains an exec request
/// for a UsdStage and provides data sources for prims when the values of those
/// data sources must be computed by exec. The UsdExecImaging_StageSceneIndex
/// also sends PrimsDirtied notifications for prims when their computed values
/// are invalidated.
///
/// UsdExecImaging_StageSceneIndex registers a factory with TfType, so that it
/// can be manufactured from usdImagingGL without linking directly to
/// usdExecImaging.
///
class UsdExecImaging_StageSceneIndex
    : public UsdImagingStageSceneIndexInterface
{
public:
    UsdExecImaging_StageSceneIndex();

    static UsdExecImaging_StageSceneIndexRefPtr New();

    ~UsdExecImaging_StageSceneIndex() override;

    // ------------------------------------------------------------------------
    // Scene index API

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    // ------------------------------------------------------------------------
    // UsdExecImagingStageSceneIndexInterface API

    /// A scene index that provides values computed by exec.
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
    void SetStage(UsdStageRefPtr stage) override;

    /// Sets the \p time at which computed values are evaluated.
    ///
    /// This calls PrimsDirtied for all prims with time-varying computed values.
    /// The updated values are recomputed on the next call to GetPrim or
    /// ApplyPendingUpdates.
    ///
    /// PrimsDirtied is only called if the time is different from the last call.
    ///
    void SetTime(UsdTimeCode time) override;

    /// Returns the current time.
    UsdTimeCode GetTime() const override;

    /// Calls PrimsDirtied for computed values that have changed due to scene
    /// changes.
    ///
    /// When a scene change occurs that invalidates one or more computed values,
    /// the invalidation events are retained by this scene index until calling
    /// this method. The exec request is recomputed prior to calling
    /// PrimsDirtied.
    ///
    void ApplyPendingUpdates() override;

private:
    UsdExecImaging_RequestSharedPtr _request;
    UsdTimeCode _time;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif