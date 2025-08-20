//
// Copyright 2021 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/mergingSceneIndex.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/base/tf/denseHashSet.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/dispatcher.h"
#include <tbb/concurrent_queue.h>

PXR_NAMESPACE_OPEN_SCOPE

HdMergingSceneIndex::HdMergingSceneIndex()
: _observer(this)
{
}

// Concurrent queue of added entries that worker threads produce.
using _AddedPrimEntryQueue =
    tbb::concurrent_queue<HdSceneIndexObserver::AddedPrimEntry>;

static void
_FillAddedChildEntriesRecursively(
    WorkDispatcher *dispatcher,
    HdMergingSceneIndex *mergingSceneIndex,
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    SdfPath parentPath,
    _AddedPrimEntryQueue *queue)
{
    for (SdfPath childPath : inputSceneIndex->GetChildPrimPaths(parentPath)) {
        // Old scene indices might have a prim of different type at the given path,
        // so we need to query the merging scene index itself here.
        const TfToken resolvedPrimType =
            mergingSceneIndex->GetPrim(childPath).primType;

        queue->emplace(childPath, resolvedPrimType);

        dispatcher->Run([=]() {
            _FillAddedChildEntriesRecursively(
                dispatcher, mergingSceneIndex, 
                inputSceneIndex, childPath, queue);
            });
    }
}

static
bool
_Contains(const SdfPath &path, const SdfPathVector &v)
{
    return std::find(v.begin(), v.end(), path) != v.end();
}

static
bool
_HasPrim(HdSceneIndexBase * const sceneIndex, const SdfPath &path)
{
    TRACE_FUNCTION();
    
    return _Contains(path, sceneIndex->GetChildPrimPaths(path.GetParentPath()));
}

void
HdMergingSceneIndex::AddInputScene(
    const HdSceneIndexBaseRefPtr &inputScene,
    const SdfPath &activeInputSceneRoot)
{
    InsertInputScene(_inputs.size(), inputScene, activeInputSceneRoot);
}

void
HdMergingSceneIndex::InsertInputScene(
    size_t pos,
    const HdSceneIndexBaseRefPtr &inputScene,
    const SdfPath &activeInputSceneRoot)
{
    InsertInputScenes({{inputScene, activeInputSceneRoot, pos}});
}   

const HdMergingSceneIndex::_InputEntries&
HdMergingSceneIndex::_GetInputEntriesByPath(SdfPath const& primPath) const
{
    TRACE_FUNCTION();

    if (_inputs.size() < 5) {
        // It is common for merging scene indexes to have few inputs,
        // ex: 2 or 3.
        // In that case, skip looking through the path table and use the full
        // list.
        return _inputs;
    }

    // Find the closest enclosing path table entry.
    for (SdfPath p = primPath; !p.IsEmpty(); p = p.GetParentPath()) {
        const _InputEntriesByPathTable::const_iterator it =
            _inputsPathTable.find(p);
        if (it != _inputsPathTable.end()) {
            return it->second;
        }
    }

    static const HdMergingSceneIndex::_InputEntries empty;
    return empty;
}

void
HdMergingSceneIndex::_RebuildInputsPathTable()
{
    TRACE_FUNCTION();

    // Make a table entry for each sceneRoot and (implicitly) its ancestors,
    // then populate the table entries with relevant inputs.
    _inputsPathTable.clear();
    for (auto const &inputEntry: _inputs) {
        _inputsPathTable[inputEntry.sceneRoot];
    }
    for (auto& [path, entriesForPath]: _inputsPathTable) {
        for (auto const &inputEntry: _inputs) {
            if (path.HasPrefix(inputEntry.sceneRoot)) {
                entriesForPath.push_back(inputEntry);
            }
        }
    }
}

void
HdMergingSceneIndex::InsertInputScenes(
    const std::vector<InputScene> &inputScenes)
{
    TRACE_FUNCTION();

    if (inputScenes.empty()) {
        return;
    }

    HdSceneIndexObserver::AddedPrimEntries addedEntries;
    if (_IsObserved()) {
        // Add prefixes of activeInputSceneRoot.
        //
        // If adding a scene inde at, e.g., /A/B/C, make
        // AddedPrimEntries for /A and /A/B.

        // Set to prevent sending the same AddedPrimEntries for
        // prefixes multiple times.
        std::unordered_set<SdfPath, SdfPath::Hash> visited;

        for (const InputScene &inputScene : inputScenes) {
            if (!inputScene.scene) {
                continue;
            }

            if (!inputScene.activeInputSceneRoot.IsAbsoluteRootOrPrimPath()) {
                // TF_CODING_ERROR raised later outside if (_IsObserved()).
                continue;
            }

            
            // Before adding the new scene index, check for which prefixes
            // of the activeInputSceneRoot another scene index was giving
            // a prim already.
            // If no other scene index was giving a prim for a prefix,
            // send message that prim with empty type was added.
            //
            const SdfPathVector prefixes =
                inputScene.activeInputSceneRoot.GetPrefixes();
            size_t i = 0;
            // Add 1 to skip the activeInputSceneRoot itself.
            for ( ; i + 1 < prefixes.size(); i++) {
                if (!(_HasPrim(this, prefixes[i]) ||
                      visited.count(prefixes[i]))) {
                    break;
                }
            }
            // For this and all following prefixes, add empty prim.
            for ( ; i + 1 < prefixes.size(); i++) {
                addedEntries.emplace_back(prefixes[i], TfToken());
                visited.insert(prefixes[i]);
            }
        }
    }

    for (const InputScene &inputScene : inputScenes) {
        if (!inputScene.scene) {
            continue;
        }
        if (!inputScene.activeInputSceneRoot.IsAbsoluteRootOrPrimPath()) {
            TF_CODING_ERROR(
                "Non-prim path '%s' as activeInputSceneRoot for "
                "HdMergingSceneIndex.",
                inputScene.activeInputSceneRoot.GetText());
            continue;
        }
        _inputs.insert(
            _inputs.begin() + std::min(inputScene.pos, _inputs.size()),
            {inputScene.scene, inputScene.activeInputSceneRoot});
    }

    _RebuildInputsPathTable();

    for (const InputScene &inputScene : inputScenes) {
        if (!inputScene.scene) {
            continue;
        }
        if (!inputScene.activeInputSceneRoot.IsAbsoluteRootOrPrimPath()) {
            // TF_CODING_ERROR already raised.
            continue;
        }
        inputScene.scene->AddObserver(HdSceneIndexObserverPtr(&_observer));
    }

    if (!_IsObserved()) {
        return;
    }

    // Add entries for input scene
    for (const InputScene &inputScene : inputScenes) {
        if (!inputScene.scene) {
            continue;
        }

        if (!inputScene.activeInputSceneRoot.IsAbsoluteRootOrPrimPath()) {
            // TF_CODING_ERROR already raised.
            continue;
        }

        _AddedPrimEntryQueue queue;

        // Old scene indices might have a prim of different type at the given path,
        // so we need to query the merging scene index itself here.
        queue.emplace(inputScene.activeInputSceneRoot,
                       GetPrim(inputScene.activeInputSceneRoot).primType);

        WorkDispatcher dispatcher;
        _FillAddedChildEntriesRecursively(
            &dispatcher, this,
            inputScene.scene, inputScene.activeInputSceneRoot, &queue);
        dispatcher.Wait();

        addedEntries.insert(
            addedEntries.end(),
            queue.unsafe_begin(), queue.unsafe_end());
    }

    _SendPrimsAdded(addedEntries);
}

void
HdMergingSceneIndex::RemoveInputScenes(
    const std::vector<HdSceneIndexBaseRefPtr> &sceneIndices)
{
    TRACE_FUNCTION();

    // Vectorization not implemented yet :(

    for (const HdSceneIndexBaseRefPtr &sceneIndex : sceneIndices) {
        RemoveInputScene(sceneIndex);
    }
}

void
HdMergingSceneIndex::RemoveInputScene(const HdSceneIndexBaseRefPtr &sceneIndex)
{
    TRACE_FUNCTION();

    auto it = std::find_if(
        _inputs.begin(), _inputs.end(),
        [&sceneIndex](const _InputEntry &entry) {
            return sceneIndex == entry.sceneIndex; });

    if (it == _inputs.end()) {
        return;
    }

    std::vector<SdfPath> removalTestQueue = { it->sceneRoot };

    sceneIndex->RemoveObserver(HdSceneIndexObserverPtr(&_observer));
    _inputs.erase(it);
    _RebuildInputsPathTable();

    if (!_IsObserved()) {
        return;
    }

    // prims unique to this input get removed
    HdSceneIndexObserver::RemovedPrimEntries removedEntries;

    // prims which this input contributed to are resynced via
    // PrimsAdded.
    HdSceneIndexObserver::AddedPrimEntries addedEntries;

    // signal removal for anything not present once this scene is
    // removed
    while (!removalTestQueue.empty()) {
        const SdfPath path = removalTestQueue.back();
        removalTestQueue.pop_back();

        const HdSceneIndexPrim prim = GetPrim(path);
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
    for (const _InputEntry &entry: _GetInputEntriesByPath(primPath)) {
        if (primPath.HasPrefix(entry.sceneRoot)) {
            HdSceneIndexPrim prim = entry.sceneIndex->GetPrim(primPath);

            // Use first non-empty prim type so that sparsely overlaid
            // inputs can contribute data sources without defining type or type
            // without providing a data source.
            if (result.primType.IsEmpty() && !prim.primType.IsEmpty()) {
                result.primType = prim.primType;
            }

            if (prim.dataSource) {
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

    for (const _InputEntry &entry: _GetInputEntriesByPath(primPath)) {
        if (primPath.HasPrefix(entry.sceneRoot)) {
            SdfPathVector paths = entry.sceneIndex->GetChildPrimPaths(primPath);
            childPaths.insert(paths.begin(), paths.end());
        }
    }
    // Insert any intermediate children implied by the existence of
    // nested inputs at deeper sceneRoot paths.
    auto range = _inputsPathTable.FindSubtreeRange(primPath);
    for (auto i = range.first; i != range.second; ++i) {
        if (i->first.GetParentPath() == primPath) {
            childPaths.insert(i->first);
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

    // if there's only one input, no additional interpretation is required.
    if (_inputs.size() < 2) {
        _SendPrimsAdded(entries);
        return;
    }

    TRACE_FUNCTION();

    // Confirm that the type here is not masked by a stronger contributing
    // input. We still send it along as an add because a weaker input providing
    // potential data sources (at any container depth) does not directly
    // indicate which data sources might be relevant. The trade-off is
    // potential over-invalidation for correctness. This ensures that the
    // primType is equivalent to what would be returned from GetPrim.

    HdSceneIndexObserver::AddedPrimEntries filteredEntries;

    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        TfToken resolvedPrimType;

        for (const _InputEntry &inputEntry:
             _GetInputEntriesByPath(entry.primPath)) {
            if (!entry.primPath.HasPrefix(inputEntry.sceneRoot)) {
                continue;
            }

            // Avoid calling GetPrim to get the prim type on scene index
            // when the scene index is the sender.
            const TfToken primType =
                get_pointer(inputEntry.sceneIndex) == &sender
                ? entry.primType
                : inputEntry.sceneIndex->GetPrim(entry.primPath).primType;
            
            // If the primType is not empty, use it.
            // Break so that we stop after the first contributing data source.
            if (!primType.IsEmpty()) {
                resolvedPrimType = primType;
                break;
            }
        }

        if (resolvedPrimType != entry.primType) {
            if (filteredEntries.empty()) {
                // copy up to this entry
                filteredEntries.reserve(entries.size());
                for (const HdSceneIndexObserver::AddedPrimEntry &origEntry :
                        entries) {
                    if (&origEntry == &entry) {
                        break;
                    }
                    filteredEntries.push_back(origEntry);
                }
            }

            // add altered entry
            filteredEntries.emplace_back(entry.primPath, resolvedPrimType);

        } else {
            // add unaltered entry if we've started to fill filteredEntries
            // otherwise, do nothing as we are meaningful in the original
            // entries until we need to copy some.
            if (!filteredEntries.empty()) {
                filteredEntries.push_back(entry);
            }
        }
    }

    if (!filteredEntries.empty()) {
        _SendPrimsAdded(filteredEntries);
    } else {
        _SendPrimsAdded(entries);
    }
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

    // Note: if a prim is removed from an input scene, but exists in another
    // input scene, we trigger that as a resync (signaled by PrimsAdded).
    HdSceneIndexObserver::AddedPrimEntries addedEntries;

    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        const SdfPathVector childPaths = GetChildPrimPaths(entry.primPath);
        const HdSceneIndexPrim prim = GetPrim(entry.primPath);

        if (!childPaths.empty() || prim.dataSource || !prim.primType.IsEmpty()) {
            addedEntries.emplace_back(entry.primPath, prim.primType);
        }

        if (childPaths.empty()) {
            continue;
        }

        HdMergingSceneIndexRefPtr const self(this);
        for (const SdfPath &childPath : childPaths) {
            for (const SdfPath& descendantPath
                     : HdSceneIndexPrimView(self, childPath)) {
                addedEntries.emplace_back(
                    descendantPath, GetPrim(descendantPath).primType);
            }
        }
    }

    _SendPrimsRemoved(entries);
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

void
HdMergingSceneIndex::_Observer::PrimsRenamed(
    const HdSceneIndexBase &sender,
    const RenamedPrimEntries &entries)
{
    // initial implementation converts to adds and removes
    ConvertPrimsRenamedToRemovedAndAdded(sender, entries, this);
}


PXR_NAMESPACE_CLOSE_SCOPE
