//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdui/sceneObserver.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/trace/trace.h"
#include "pxr/imaging/hd/sceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

HduiSceneObserver::HduiSceneObserver()
    : _batching(false)
{
}

HduiSceneObserver::~HduiSceneObserver() = default;

void
HduiSceneObserver::Subscribe(
    const HdSceneIndexBasePtr& sceneIndex)
{
    if (!sceneIndex) {
        TF_CODING_ERROR("Invalid Hydra scene index");
        return;
    }

    if (_index != sceneIndex) {
        Unsubscribe();

        if (sceneIndex) {
            sceneIndex->AddObserver(TfCreateWeakPtr(this));
        }

        _index = sceneIndex;
    }
}

void
HduiSceneObserver::Unsubscribe()
{
    if (_index) {
        _index->RemoveObserver(TfCreateWeakPtr(this));
    }

    _ClearBatchedChanges();
    _index = nullptr;
}

void
HduiSceneObserver::SetBatchingEnabled(bool enabled)
{
    if (_batching != enabled) {
        if (_batching) {
            FlushBatchedUpdates();
        }

        _batching = enabled;
    }
}

void
HduiSceneObserver::FlushBatchedUpdates()
{
    // Adds and removes (and moves/renames)
    Q_EMIT PrimsAddedOrRemoved(_batchedAddedPrims, _batchedRemovedPrims);

    _batchedAddedPrims.clear();
    _batchedRemovedPrims.clear();

    // Prim dirties
    DirtiedPrimEntries dirtyEntries;
    dirtyEntries.reserve(_batchedDirtiedPrims.size());

    for (const auto& it : _batchedDirtiedPrims) {
        const SdfPath& primPath = it.first;
        const HdDataSourceLocatorSet& dirtyLocators = it.second;
        dirtyEntries.push_back({ primPath, dirtyLocators });
    }

    Q_EMIT PrimsMarkedDirty(dirtyEntries);

    _batchedDirtiedPrims.clear();
}

void
HduiSceneObserver::PrimsAdded(
    const HdSceneIndexBase&,
    const AddedPrimEntries& entries)
{
    TRACE_FUNCTION();

    if (_batching) {
        for (const AddedPrimEntry& entry : entries) {
            _BatchAddedPrim(entry.primPath);
        }

        Q_EMIT ChangeBatched();
    }
    else {
        SdfPathSet addedPaths;

        for (const AddedPrimEntry& entry : entries) {
            addedPaths.insert(entry.primPath);
        }

        Q_EMIT PrimsAddedOrRemoved(addedPaths, SdfPathSet());
    }
}

void
HduiSceneObserver::PrimsRemoved(
    const HdSceneIndexBase&,
    const RemovedPrimEntries& entries)
{
    TRACE_FUNCTION();

    if (_batching) {
        for (const RemovedPrimEntry& entry : entries) {
            _BatchRemovedPrim(entry.primPath);
        }

        Q_EMIT ChangeBatched();
    }
    else {
        SdfPathSet removedPaths;

        for (const RemovedPrimEntry& entry : entries) {
            removedPaths.insert(entry.primPath);
        }

        Q_EMIT PrimsAddedOrRemoved(SdfPathSet(), removedPaths);
    }
}

void
HduiSceneObserver::PrimsDirtied(
    const HdSceneIndexBase&,
    const DirtiedPrimEntries& entries)
{
    if (_batching) {
        for (const DirtiedPrimEntry& entry : entries) {
            _BatchDirtiedPrim(entry.primPath, entry.dirtyLocators);
        }

        Q_EMIT ChangeBatched();
    }
    else {
        Q_EMIT PrimsMarkedDirty(entries);
    }
}

void
HduiSceneObserver::PrimsRenamed(
    const HdSceneIndexBase&,
    const RenamedPrimEntries& entries)
{
    TRACE_FUNCTION();

    if (_batching) {
        for (const RenamedPrimEntry& entry : entries) {
            _BatchRemovedPrim(entry.oldPrimPath);
            _BatchAddedPrim(entry.newPrimPath);
        }

        Q_EMIT ChangeBatched();
    }
    else {
        SdfPathSet addedPaths;
        SdfPathSet removedPaths;

        for (const RenamedPrimEntry& entry : entries) {
            addedPaths.insert(entry.newPrimPath);
            removedPaths.insert(entry.oldPrimPath);
        }

        Q_EMIT PrimsAddedOrRemoved(addedPaths, removedPaths);
    }
}

void
HduiSceneObserver::_BatchAddedPrim(
    const SdfPath& primPath)
{
    _batchedAddedPrims.insert(primPath);
}

void
HduiSceneObserver::_BatchRemovedPrim(
    const SdfPath& primPath)
{
    _batchedRemovedPrims.insert(primPath);
}

void
HduiSceneObserver::_BatchDirtiedPrim(
    const SdfPath& primPath,
    const HdDataSourceLocatorSet& dirtyLocators)
{
    _batchedDirtiedPrims[primPath].insert(dirtyLocators);
}

void
HduiSceneObserver::_ClearBatchedChanges()
{
    _batchedAddedPrims.clear();
    _batchedRemovedPrims.clear();
    _batchedDirtiedPrims.clear();
}

PXR_NAMESPACE_CLOSE_SCOPE
