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
    // Q_EMIT PrimsAddedOrRemoved(_batchedAddedPrimEntries, _batchedRemovedPrimEntries);
    Q_EMIT PrimsMarkedAdded(_batchedAddedPrimEntries);
    Q_EMIT PrimsMarkedRemoved(_batchedRemovedPrimEntries);
    Q_EMIT PrimsMarkedRenamed(_batchedRenamedPrimEntries);

    // For batched dirty entries, aggregate entries by prim path. So, if we have
    // multiple entries for the same prim, we will only emit one entry with
    // the union of the dirty locators.
    //
    {
        std::map<SdfPath, HdDataSourceLocatorSet> dirtyPrimMap;
        for (const auto& [primPath, dirtyLocators] :
                _batchedDirtiedPrimEntries) {

            dirtyPrimMap[primPath].insert(dirtyLocators);;
        }

        DirtiedPrimEntries dirtyEntries;
        dirtyEntries.reserve(dirtyPrimMap.size());
        
        for (const auto& [primPath, dirtyLocators] : dirtyPrimMap) {
            dirtyEntries.push_back({ primPath, dirtyLocators });
        }
        
        Q_EMIT PrimsMarkedDirty(dirtyEntries);
    }
    
    _ClearBatchedChanges();
}

// ----------------------------------------------------------------------------
// Qt Signals
//

void
HduiSceneObserver::PrimsAdded(
    const HdSceneIndexBase&,
    const AddedPrimEntries& entries)
{
    TRACE_FUNCTION();

    if (_batching) {
        _batchedAddedPrimEntries.insert(
            _batchedAddedPrimEntries.end(), entries.begin(), entries.end());

        Q_EMIT ChangeBatched();
    }
    else {
        Q_EMIT PrimsMarkedAdded(entries);
    }
}

void
HduiSceneObserver::PrimsRemoved(
    const HdSceneIndexBase&,
    const RemovedPrimEntries& entries)
{
    TRACE_FUNCTION();

    if (_batching) {
        _batchedRemovedPrimEntries.insert(
            _batchedRemovedPrimEntries.end(), entries.begin(), entries.end());

        Q_EMIT ChangeBatched();
    }
    else {
        Q_EMIT PrimsMarkedRemoved(entries);
    }
}

void
HduiSceneObserver::PrimsRenamed(
    const HdSceneIndexBase&,
    const RenamedPrimEntries& entries)
{
    TRACE_FUNCTION();

    if (_batching) {
        TRACE_SCOPE("Batching send");
        _batchedRenamedPrimEntries.insert(
            _batchedRenamedPrimEntries.end(), entries.begin(), entries.end());

        Q_EMIT ChangeBatched();
    }
    else {
        Q_EMIT PrimsMarkedRenamed(entries);
    }
}

void
HduiSceneObserver::PrimsDirtied(
    const HdSceneIndexBase&,
    const DirtiedPrimEntries& entries)
{
    if (_batching) {

        TRACE_SCOPE("Batching send");

        _batchedDirtiedPrimEntries.insert(
            _batchedDirtiedPrimEntries.end(), entries.begin(), entries.end());

        Q_EMIT ChangeBatched();
    }
    else {
        Q_EMIT PrimsMarkedDirty(entries);
    }
}

//
// ----------------------------------------------------------------------------

void
HduiSceneObserver::_ClearBatchedChanges()
{
    _batchedAddedPrimEntries.clear();
    _batchedRemovedPrimEntries.clear();
    _batchedRenamedPrimEntries.clear();
    _batchedDirtiedPrimEntries.clear();
}

PXR_NAMESPACE_CLOSE_SCOPE
