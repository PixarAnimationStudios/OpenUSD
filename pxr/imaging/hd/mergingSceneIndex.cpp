//
// Copyright 2021 Pixar
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
#include "pxr/imaging/hd/mergingSceneIndex.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/base/tf/denseHashSet.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

HdMergingSceneIndex::HdMergingSceneIndex()
: _observer(this)
{
}

void
HdMergingSceneIndex::AddInputScene(
    const HdSceneIndexBaseRefPtr &inputScene,
    const SdfPath &activeInputSceneRoot)
{
    // XXX: Note: when scenes are added, we don't generate PrimsAdded;
    // the caller is responsible for notifying the downstream scene indices of
    // any previously populated prims.
    if (inputScene) {
        _inputs.emplace_back(inputScene, activeInputSceneRoot);
        inputScene->AddObserver(HdSceneIndexObserverPtr(&_observer));
    }
}

void
HdMergingSceneIndex::RemoveInputScene(const HdSceneIndexBaseRefPtr &sceneIndex)
{
    for (_InputEntries::iterator it = _inputs.begin(); it != _inputs.end();
            ++it) {
        if (sceneIndex == it->sceneIndex) {
            std::vector<SdfPath> removalTestQueue = { it->sceneRoot };

            // prims unique to this input get removed
            HdSceneIndexObserver::RemovedPrimEntries removedEntries;

            // prims which this input contributed to are resynced via
            // PrimsAdded.
            HdSceneIndexObserver::AddedPrimEntries addedEntries;

            sceneIndex->RemoveObserver(HdSceneIndexObserverPtr(&_observer));
            _inputs.erase(it);

            if (!_IsObserved()) {
                return;
            }

            // signal removal for anything not present once this scene is
            // removed
            while (!removalTestQueue.empty()) {
                const SdfPath path = removalTestQueue.back();
                removalTestQueue.pop_back();

                HdSceneIndexPrim prim = GetPrim(path);
                if (!prim.dataSource
                        && GetChildPrimPaths(path).empty()) {
                    removedEntries.emplace_back(path);
                } else {
                    addedEntries.emplace_back(path, prim.primType);
                    for (const SdfPath &childPath :
                            sceneIndex->GetChildPrimPaths(path)) {
                        removalTestQueue.push_back(childPath);
                    }
                }
            }

            if (!removedEntries.empty()) {
                _SendPrimsRemoved(removedEntries);
            }
            if (!addedEntries.empty()) {
                _SendPrimsAdded(addedEntries);
            }
            return;
        }
    }
}

std::vector<HdSceneIndexBaseRefPtr>
HdMergingSceneIndex::GetInputScenes() const
{
    std::vector<HdSceneIndexBaseRefPtr> result;
    result.reserve(_inputs.size());

    for (const _InputEntry &entry : _inputs) {
        result.push_back(entry.sceneIndex);
    }

    return result;
}

HdSceneIndexPrim
HdMergingSceneIndex::GetPrim(const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    HdSceneIndexPrim result;

    if (_inputs.size() == 0) {
        return result;
    }

    if (_inputs.size() == 1) {
        return _inputs[0].sceneIndex->GetPrim(primPath);
    }

    TfSmallVector<HdContainerDataSourceHandle, 8> contributingDataSources;
    for (const _InputEntry &entry : _inputs) {
        if (primPath.HasPrefix(entry.sceneRoot)) {
            HdSceneIndexPrim prim = entry.sceneIndex->GetPrim(primPath);
            if (prim.dataSource) {
                if (contributingDataSources.empty()) {
                    result.primType = prim.primType;
                }
                contributingDataSources.push_back(prim.dataSource);
            }
        }
    }

    switch (contributingDataSources.size())
    {
    case 0:
        break;
    case 1:
        result.dataSource = contributingDataSources[0];
        break;
    default:
        result.dataSource = HdOverlayContainerDataSource::New(
            contributingDataSources.size(), contributingDataSources.data());
    };

    return result;
}

SdfPathVector
HdMergingSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    TfDenseHashSet<SdfPath, SdfPath::Hash, std::equal_to<SdfPath>, 32>
        childPaths;

    for (const _InputEntry &entry : _inputs) {
        if (primPath.HasPrefix(entry.sceneRoot)) {
            
            for (const SdfPath &childPath :
                    entry.sceneIndex->GetChildPrimPaths(primPath)) {
                childPaths.insert(childPath);
            }
        } else {
            // need to make sure we include intermediate scopes
            if (entry.sceneRoot.HasPrefix(primPath)) {
                SdfPathVector v;
                entry.sceneRoot.GetPrefixes(&v);
                const SdfPath &childPath = v[primPath.GetPathElementCount()];
                childPaths.insert(childPath);
            }
        }
    }

    return SdfPathVector(childPaths.begin(), childPaths.end());
}

void
HdMergingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    _SendPrimsAdded(entries);
}

void
HdMergingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    if (!_IsObserved()) {
        return;
    }

    if (_inputs.size() < 2) {
        _SendPrimsRemoved(entries);
        return;
    }

    HdSceneIndexObserver::RemovedPrimEntries filteredEntries;
    filteredEntries.reserve(entries.size());

    // Note: if a prim is removed from an input scene, but exists in another
    // input scene, we trigger that as a resync (signaled by PrimsAdded).
    HdSceneIndexObserver::AddedPrimEntries addedEntries;

    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        bool primFullyRemoved = true;

        for (const _InputEntry &inputEntry : _inputs) {
            if (get_pointer(inputEntry.sceneIndex) == &sender) {
                continue;
            }

            // another input having either a data source or children of the
            // specified prim considers this not a full removal
            if (inputEntry.sceneIndex->GetPrim(entry.primPath).dataSource
                    || !inputEntry.sceneIndex->GetChildPrimPaths(
                            entry.primPath).empty()) {
                primFullyRemoved = false;
                break;
            }
        }

        if (primFullyRemoved) {
            filteredEntries.push_back(entry);
        } else {
            addedEntries.emplace_back(entry.primPath,
                    GetPrim(entry.primPath).primType);
        }
    }

    _SendPrimsRemoved(filteredEntries);
    _SendPrimsAdded(addedEntries);
}

void
HdMergingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    _SendPrimsDirtied(entries);
}

void
HdMergingSceneIndex::_Observer::PrimsAdded(
    const HdSceneIndexBase &sender,
    const AddedPrimEntries &entries)
{
    _owner->_PrimsAdded(sender, entries);
}

void
HdMergingSceneIndex::_Observer::PrimsRemoved(
    const HdSceneIndexBase &sender,
    const RemovedPrimEntries &entries)
{
    _owner->_PrimsRemoved(sender, entries);
}

void
HdMergingSceneIndex::_Observer::PrimsDirtied(
    const HdSceneIndexBase &sender,
    const DirtiedPrimEntries &entries)
{
    _owner->_PrimsDirtied(sender, entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
