//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdExecImaging/stageSceneIndex.h"

#include "pxr/base/tf/refPtr.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdExecImaging_StageSceneIndexRefPtr
UsdExecImaging_StageSceneIndex::New()
{
    return TfCreateRefPtr(new UsdExecImaging_StageSceneIndex);
}

UsdExecImaging_StageSceneIndex::UsdExecImaging_StageSceneIndex() = default;

UsdExecImaging_StageSceneIndex::~UsdExecImaging_StageSceneIndex() = default;

HdSceneIndexPrim
UsdExecImaging_StageSceneIndex::GetPrim(const SdfPath &primPath) const
{
    return {
        // UsdExecImaging_StageSceneIndex provides no opinion for the prim type.
        // The prim type is overridden by UsdImagingStageSceneIndex.
        TfToken(),

        // Data source.
        _request ? _request->GetPrimData(primPath) : nullptr
    };
}

SdfPathVector
UsdExecImaging_StageSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    return {};
}

void
UsdExecImaging_StageSceneIndex::SetStage(UsdStageRefPtr stage)
{
    if (stage) {
        _request = UsdExecImaging_Request::New(std::move(stage));
    }
    else {
        _request.reset();
    }
}

void
UsdExecImaging_StageSceneIndex::SetTime(UsdTimeCode time)
{
    if (_request) {
        _request->SetTime(time);
        _request->Refresh();
        _SendPrimsDirtied(_request->TakeDirtiedPrimEntries());
    }
}

void
UsdExecImaging_StageSceneIndex::ApplyPendingUpdates()
{
    if (_request) {
        _request->Refresh();
        _SendPrimsDirtied(_request->TakeDirtiedPrimEntries());
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
