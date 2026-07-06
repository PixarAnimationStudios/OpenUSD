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
#include "pxr/usdImaging/usdExecImaging/stageSceneIndexInterface.h"

#include "pxr/base/tf/declarePtrs.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdExecImaging_StageSceneIndex);

/// The one and only implementation of UsdExecImagingStageSceneIndexInterface.
///
/// The implementation is private to usdExecImaging, so that it can be omitted
/// when building with PXR_BUILD_EXEC=OFF.
///
class UsdExecImaging_StageSceneIndex
    : public UsdExecImagingStageSceneIndexInterface
{
public:
    static UsdExecImaging_StageSceneIndexRefPtr New();

    ~UsdExecImaging_StageSceneIndex() override;

    // ------------------------------------------------------------------------
    // Scene index API

    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    // ------------------------------------------------------------------------
    // UsdExecImagingStageSceneIndexInterface API

    void SetStage(UsdStageRefPtr stage) override;

    void SetTime(UsdTimeCode time) override;

    void ApplyPendingUpdates() override;

private:
    UsdExecImaging_StageSceneIndex();

private:
    UsdExecImaging_RequestSharedPtr _request;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif