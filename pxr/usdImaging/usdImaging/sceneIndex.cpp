//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/sceneIndex.h"

#include "pxr/usdImaging/usdImaging/selectionSceneIndex.h"
#include "pxr/usdImaging/usdImaging/sceneIndices.h"
#include "pxr/usdImaging/usdImaging/stageSceneIndex.h"
#include "pxr/usdImaging/usdImaging/sceneIndexCreateArgsSchema.h"

PXR_NAMESPACE_OPEN_SCOPE

// Bind sceneIndexCreateArgs into the given callback to produce the (deprecated) single-
// argument callback that UsdImagingCreateSceneIndices expects.
static
UsdImagingSceneIndexAppendCallback
_BindSceneIndexCreateArgs(
    const UsdImagingSceneIndex::SceneIndexAppendCallback &callback,
    HdContainerDataSourceHandle const &sceneIndexCreateArgs)
{
    if (!callback) {
        return {};
    }
    return [callback, sceneIndexCreateArgs](
        HdSceneIndexBaseRefPtr const &inputScene) {
            return callback(inputScene, sceneIndexCreateArgs);
    };
}

UsdImagingSceneIndex::UsdImagingSceneIndex(
    HdContainerDataSourceHandle const &sceneIndexCreateArgs,
    const SceneIndexAppendCallbacks &sceneIndexAppendCallbacks)
 : UsdImagingSceneIndex(
     UsdImagingCreateSceneIndices(
         sceneIndexCreateArgs,
         _BindSceneIndexCreateArgs(
             sceneIndexAppendCallbacks.overridesSceneIndexCallback,
             sceneIndexCreateArgs)))
{
}

UsdImagingSceneIndex::UsdImagingSceneIndex(
    const UsdImagingSceneIndices &sceneIndices)
 : _stageSceneIndex(
     sceneIndices.stageSceneIndex)
 , _postInstancingNoticeBatchingSceneIndex(
     sceneIndices.postInstancingNoticeBatchingSceneIndex)
 , _selectionSceneIndex(
     sceneIndices.selectionSceneIndex)
 , _finalSceneIndex(
     sceneIndices.finalSceneIndex)
 , _observer(this)
{
    _finalSceneIndex->AddObserver(HdSceneIndexObserverPtr(&_observer));
}

UsdImagingSceneIndex::~UsdImagingSceneIndex() = default;

HdSceneIndexPrim
UsdImagingSceneIndex::GetPrim(const SdfPath &primPath) const
{
    return _finalSceneIndex->GetPrim(primPath);
}

SdfPathVector
UsdImagingSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    return _finalSceneIndex->GetChildPrimPaths(primPath);
}

HdSceneIndexBaseRefPtrVector
UsdImagingSceneIndex::GetEncapsulatedScenes() const
{
    return { _finalSceneIndex };
}

void
UsdImagingSceneIndex::SetStage(UsdStageRefPtr stage)
{
    _stageSceneIndex->SetStage(std::move(stage));
}

void
UsdImagingSceneIndex::SetTime(UsdTimeCode time,
                              bool forceDirtyingTimeDeps)
{
    _stageSceneIndex->SetTime(time, forceDirtyingTimeDeps);
}

UsdTimeCode
UsdImagingSceneIndex::GetTime() const
{
    return _stageSceneIndex->GetTime();
}

void
UsdImagingSceneIndex::ApplyPendingUpdates()
{
    return _stageSceneIndex->ApplyPendingUpdates();
}

void
UsdImagingSceneIndex::AddSelection(const SdfPath &path)
{
    _selectionSceneIndex->AddSelection(path);
}

void
UsdImagingSceneIndex::ClearSelection()
{
    _selectionSceneIndex->ClearSelection();
}

PXR_NAMESPACE_CLOSE_SCOPE
