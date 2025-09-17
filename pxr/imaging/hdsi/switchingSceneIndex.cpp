//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hdsi/switchingSceneIndex.h"

PXR_NAMESPACE_OPEN_SCOPE

HdsiSwitchingSceneIndex::HdsiSwitchingSceneIndex(
    const std::vector<HdSceneIndexBaseRefPtr>& inputs,
    size_t initialIndex,
    ComputeDiffFn computeDiffFn)
    : _observer(this)
    , _inputs(inputs.begin(), inputs.end())
    , _computeDiffFn(std::move(computeDiffFn))
{
    _UpdateCurrentSceneIndex(initialIndex);
}

size_t
HdsiSwitchingSceneIndex::GetIndex() const
{
    return _index;
}

void
HdsiSwitchingSceneIndex::SetIndex(size_t index)
{
    _UpdateCurrentSceneIndex(index);
}

std::vector<HdSceneIndexBaseRefPtr>
HdsiSwitchingSceneIndex::GetInputScenes() const
{
    return std::vector<HdSceneIndexBaseRefPtr>(_inputs.begin(), _inputs.end());
}

HdSceneIndexPrim
HdsiSwitchingSceneIndex::GetPrim(const SdfPath& primPath) const
{
    if (_currentSceneIndex) {
        return _currentSceneIndex->GetPrim(primPath);
    }
    return {};
}

SdfPathVector
HdsiSwitchingSceneIndex::GetChildPrimPaths(const SdfPath& primPath) const
{
    if (_currentSceneIndex) {
        return _currentSceneIndex->GetChildPrimPaths(primPath);
    }
    return {};
}

void
HdsiSwitchingSceneIndex::_UpdateCurrentSceneIndex(size_t index)
{
    const HdSceneIndexBaseRefPtr prevInputScene = _currentSceneIndex;
    _index = index;

    if (_index < _inputs.size()) {
        _currentSceneIndex = _inputs[_index];
    }
    else {
        TF_CODING_ERROR(
            "Invalid index %zu (size %zu).", _index, _inputs.size());
        _currentSceneIndex.Reset();
    }

    if (prevInputScene) {
        prevInputScene->RemoveObserver(HdSceneIndexObserverPtr(&_observer));
    }

    if (_computeDiffFn && _IsObserved()) {
        HdSceneIndexObserver::RemovedPrimEntries removedEntries;
        HdSceneIndexObserver::AddedPrimEntries addedEntries;
        HdSceneIndexObserver::RenamedPrimEntries renamedEntries;
        HdSceneIndexObserver::DirtiedPrimEntries dirtiedEntries;
        _computeDiffFn(
            prevInputScene, _currentSceneIndex, &removedEntries, &addedEntries,
            &renamedEntries, &dirtiedEntries);
        _SendPrimsRemoved(removedEntries);
        _SendPrimsAdded(addedEntries);
        _SendPrimsRenamed(renamedEntries);
        _SendPrimsDirtied(dirtiedEntries);
    }

    if (_currentSceneIndex) {
        _currentSceneIndex->AddObserver(HdSceneIndexObserverPtr(&_observer));
    }
}

void
HdsiSwitchingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{
    if (!_IsObserved()) {
        return;
    }
    _SendPrimsAdded(entries);
}

void
HdsiSwitchingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    if (!_IsObserved()) {
        return;
    }
    _SendPrimsRemoved(entries);
}

void
HdsiSwitchingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    if (!_IsObserved()) {
        return;
    }
    _SendPrimsDirtied(entries);
}

void
HdsiSwitchingSceneIndex::_PrimsRenamed(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::RenamedPrimEntries& entries)
{
    if (!_IsObserved()) {
        return;
    }
    _SendPrimsRenamed(entries);
}

void
HdsiSwitchingSceneIndex::_Observer::PrimsAdded(
    const HdSceneIndexBase& sender, const AddedPrimEntries& entries)
{
    _owner->_PrimsAdded(sender, entries);
}

void
HdsiSwitchingSceneIndex::_Observer::PrimsRemoved(
    const HdSceneIndexBase& sender, const RemovedPrimEntries& entries)
{
    _owner->_PrimsRemoved(sender, entries);
}

void
HdsiSwitchingSceneIndex::_Observer::PrimsDirtied(
    const HdSceneIndexBase& sender, const DirtiedPrimEntries& entries)
{
    _owner->_PrimsDirtied(sender, entries);
}

void
HdsiSwitchingSceneIndex::_Observer::PrimsRenamed(
    const HdSceneIndexBase& sender, const RenamedPrimEntries& entries)
{
    _owner->_PrimsRenamed(sender, entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
