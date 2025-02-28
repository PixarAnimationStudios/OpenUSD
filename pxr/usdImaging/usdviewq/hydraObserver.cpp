//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdviewq/hydraObserver.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

#include "pxr/base/tf/stringUtils.h"

#include <deque>
#include <typeinfo>

PXR_NAMESPACE_OPEN_SCOPE

UsdviewqHydraObserver::~UsdviewqHydraObserver()
{
    if (_sceneIndex) {
        _sceneIndex->RemoveObserver(HdSceneIndexObserverPtr(&_observer));
    }
}

/*static*/
std::vector<std::string>
UsdviewqHydraObserver::GetRegisteredSceneIndexNames()
{
    return HdSceneIndexNameRegistry::GetInstance().GetRegisteredNames();
}

bool
UsdviewqHydraObserver::TargetToInputSceneIndex(const IndexList &inputIndices)
{
    HdFilteringSceneIndexBaseRefPtr currentSceneIndex =
        TfDynamic_cast<HdFilteringSceneIndexBaseRefPtr>(_sceneIndex);

    std::vector<HdSceneIndexBaseRefPtr> inputScenes;

    IndexList indices(inputIndices.rbegin(), inputIndices.rend());
    while (!indices.empty() && currentSceneIndex) {
        size_t i = indices.back();
        indices.pop_back();

        inputScenes = currentSceneIndex->GetInputScenes();

        if (i >= inputScenes.size()) {
            return false;
        }

        if (indices.empty()) {
            return _Target(inputScenes[i]);
        } else {
            currentSceneIndex =
                TfDynamic_cast<HdFilteringSceneIndexBaseRefPtr>(
                    inputScenes[i]);
        }
    }

    return false;
}

bool
UsdviewqHydraObserver::TargetToNamedSceneIndex(const std::string &name)
{
    return _Target(HdSceneIndexNameRegistry::GetInstance()
        .GetNamedSceneIndex(name));
}

bool
UsdviewqHydraObserver::_Target(const HdSceneIndexBaseRefPtr &sceneIndex)
{
    _nestedInputSceneIndices.reset();
    
    if (_sceneIndex) {
        _sceneIndex->RemoveObserver(HdSceneIndexObserverPtr(&_observer));
    }

    _observer.notices.clear();

    _sceneIndex = sceneIndex;
    if (_sceneIndex) {
        _sceneIndex->AddObserver(HdSceneIndexObserverPtr(&_observer));
    }

    return _sceneIndex != nullptr;
}

static
std::string
_GetDisplayName(const HdSceneIndexBaseRefPtr &sceneIndex)
{
    if (!sceneIndex) {
        return std::string();
    }

    return sceneIndex->GetDisplayName();
}

std::string
UsdviewqHydraObserver::GetDisplayName()
{
    return _GetDisplayName(_sceneIndex);
}

std::vector<std::string>
UsdviewqHydraObserver::GetInputDisplayNames(const IndexList &inputIndices)
{
    std::vector<std::string> result;
    std::vector<HdSceneIndexBaseRefPtr> inputScenes;

    HdFilteringSceneIndexBaseRefPtr currentSceneIndex =
        TfDynamic_cast<HdFilteringSceneIndexBaseRefPtr>(_sceneIndex);

    IndexList indices(inputIndices.rbegin(), inputIndices.rend());
    while (currentSceneIndex) {
        inputScenes = currentSceneIndex->GetInputScenes();

        if (indices.empty()) {
            for (const HdSceneIndexBaseRefPtr & inputScene : inputScenes) {
                result.push_back(_GetDisplayName(inputScene));
            }

            return result;
        }

        size_t i = indices.back();
        indices.pop_back();

        if (i < inputScenes.size()) {
            currentSceneIndex =
                TfDynamic_cast<HdFilteringSceneIndexBaseRefPtr>(
                    inputScenes[i]);
        } else {
            break;
        }
    }

    return result;
}

std::vector<std::string>
UsdviewqHydraObserver::GetNestedInputDisplayNames()
{
    _ComputeNestedInputSceneIndices();

    std::vector<std::string> result;
    result.reserve(_nestedInputSceneIndices->size());
    for (const HdSceneIndexBaseRefPtr &inputScene : *_nestedInputSceneIndices) {
        result.push_back(_GetDisplayName(inputScene));
    }
    return result;
}

bool
UsdviewqHydraObserver::TargetToNestedInputSceneIndex(
    const size_t nestedInputIndex)
{
    _ComputeNestedInputSceneIndices();

    if (!(nestedInputIndex < _nestedInputSceneIndices->size())) {
        return false;
    }

    return _Target((*_nestedInputSceneIndices)[nestedInputIndex]);
}

void
UsdviewqHydraObserver::_ComputeNestedInputSceneIndices()
{
    if (_nestedInputSceneIndices) {
        return;
    }

    _nestedInputSceneIndices = HdSceneIndexBaseRefPtrVector();

    std::set<HdSceneIndexBaseRefPtr> visited = { };
    std::deque<HdSceneIndexBaseRefPtr> pending = { _sceneIndex };

    while (!pending.empty()) {
        HdSceneIndexBaseRefPtr const sceneIndex = pending.front();
        pending.pop_front();
        if (!visited.insert(sceneIndex).second) {
            continue;
        }
        _nestedInputSceneIndices->push_back(sceneIndex);
        auto const filteringSceneIndex =
            TfDynamic_cast<HdFilteringSceneIndexBaseRefPtr>(sceneIndex);
        if (!filteringSceneIndex) {
            continue;
        }
        for (HdSceneIndexBaseRefPtr const &inputScene :
                 filteringSceneIndex->GetInputScenes()) {
            pending.push_back(inputScene);
        }
    }
}

SdfPathVector
UsdviewqHydraObserver::GetChildPrimPaths(const SdfPath &primPath)
{
    if (_sceneIndex) {
        return _sceneIndex->GetChildPrimPaths(primPath);
    }

    return {};
}

HdSceneIndexPrim
UsdviewqHydraObserver::GetPrim(const SdfPath &primPath)
{
    if (_sceneIndex) {
        return _sceneIndex->GetPrim(primPath);
    }

    return {TfToken(), nullptr};
}

void
UsdviewqHydraObserver::_Observer::PrimsAdded(
        const HdSceneIndexBase &sender,
        const AddedPrimEntries &entries)
{
    if (!notices.empty() && !notices.back().added.empty()) {
        HdSceneIndexObserver::AddedPrimEntries &added =
            notices.back().added;
        added.insert(added.end(), entries.begin(), entries.end());
    } else {
        notices.emplace_back(entries);
    }
}

void
UsdviewqHydraObserver::_Observer::PrimsRemoved(
        const HdSceneIndexBase &sender,
        const RemovedPrimEntries &entries)
{
    if (!notices.empty() && !notices.back().removed.empty()) {
        HdSceneIndexObserver::RemovedPrimEntries &removed =
            notices.back().removed;
        removed.insert(removed.end(), entries.begin(), entries.end());
    } else {
        notices.emplace_back(entries);
    }
}

void
UsdviewqHydraObserver::_Observer::PrimsDirtied(
        const HdSceneIndexBase &sender,
        const DirtiedPrimEntries &entries)
{
    if (!notices.empty() && !notices.back().dirtied.empty()) {
        HdSceneIndexObserver::DirtiedPrimEntries &dirtied =
            notices.back().dirtied;
        dirtied.insert(dirtied.end(), entries.begin(), entries.end());
    } else {
        notices.emplace_back(entries);
    }
}

void
UsdviewqHydraObserver::_Observer::PrimsRenamed(
        const HdSceneIndexBase &sender,
        const RenamedPrimEntries &entries)
{
    ConvertPrimsRenamedToRemovedAndAdded(sender, entries, this);
}


bool
UsdviewqHydraObserver::HasPendingNotices()
{
    return !_observer.notices.empty();
}

UsdviewqHydraObserver::NoticeEntryVector
UsdviewqHydraObserver::GetPendingNotices()
{
    return std::move(_observer.notices);
}

void
UsdviewqHydraObserver::ClearPendingNotices()
{
    _observer.notices.clear();
}

PXR_NAMESPACE_CLOSE_SCOPE
