//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_COMPUTE_SCENE_INDEX_DIFF_H
#define PXR_IMAGING_HDSI_COMPUTE_SCENE_INDEX_DIFF_H

#include "pxr/pxr.h"

#include "pxr/imaging/hdsi/api.h"

#include "pxr/imaging/hd/sceneIndex.h"

#include "pxr/base/tf/declarePtrs.h"

PXR_NAMESPACE_OPEN_SCOPE

/// This is used to compute the difference between `siA` and `siB`
/// expressed as `removedEntries`, `addedEntries`, `renamedEntries`, and
/// `dirtiedEntries`, being sent in that order.
using HdsiComputeSceneIndexDiff = std::function<void(
    const HdSceneIndexBaseRefPtr& siA,
    const HdSceneIndexBaseRefPtr& siB,
    HdSceneIndexObserver::RemovedPrimEntries* removedEntries,
    HdSceneIndexObserver::AddedPrimEntries* addedEntries,
    HdSceneIndexObserver::RenamedPrimEntries* renamedEntries,
    HdSceneIndexObserver::DirtiedPrimEntries* dirtiedEntries)>;

/// This compute diff function resets the entire scene.
///
/// If \p siA is not null, this will remove `/`.
/// If \p siB is not null, it will add all prims (recursively)
/// starting with `/`.
///
/// All of the pointers should be non-null.
HDSI_API
void HdsiComputeSceneIndexDiffRoot(
    const HdSceneIndexBaseRefPtr& siA,
    const HdSceneIndexBaseRefPtr& siB,
    HdSceneIndexObserver::RemovedPrimEntries* removedEntries,
    HdSceneIndexObserver::AddedPrimEntries* addedEntries,
    HdSceneIndexObserver::RenamedPrimEntries* renamedEntries,
    HdSceneIndexObserver::DirtiedPrimEntries* dirtiedEntries);

/// This will walk both scene indices and try to compute a sparse
/// delta at the prim level.
///
/// All of the pointers should be non-null.
HDSI_API
void HdsiComputeSceneIndexDiffDelta(
    const HdSceneIndexBaseRefPtr& siA,
    const HdSceneIndexBaseRefPtr& siB,
    HdSceneIndexObserver::RemovedPrimEntries* removedEntries,
    HdSceneIndexObserver::AddedPrimEntries* addedEntries,
    HdSceneIndexObserver::RenamedPrimEntries* renamedEntries,
    HdSceneIndexObserver::DirtiedPrimEntries* dirtiedEntries);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
