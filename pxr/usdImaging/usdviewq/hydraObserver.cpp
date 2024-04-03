//
// Copyright 2022 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/usdImaging/usdviewq/hydraObserver.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

#include "pxr/base/tf/stringUtils.h"

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