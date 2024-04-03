//
// Copyright 2023 Pixar
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

#include "pxr/imaging/hdsi/primTypeNoticeBatchingSceneIndex.h"

#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdsiPrimTypeNoticeBatchingSceneIndexTokens,
                        HDSI_PRIM_TYPE_NOTICE_BATCHING_SCENE_INDEX_TOKENS);

HdsiPrimTypeNoticeBatchingSceneIndex::
PrimTypePriorityFunctor::~PrimTypePriorityFunctor() = default;

namespace
{

using _PriorityFunctorHandle =
    HdsiPrimTypeNoticeBatchingSceneIndex::PrimTypePriorityFunctorHandle;

_PriorityFunctorHandle
_GetPrimTypePriorityFunctor(HdContainerDataSourceHandle const &inputArgs)
{
    if (!inputArgs) {
        return nullptr;
    }

    using DataSource = HdTypedSampledDataSource<_PriorityFunctorHandle>;
    using Handle = typename DataSource::Handle;

    static const TfToken &key =
        HdsiPrimTypeNoticeBatchingSceneIndexTokens->primTypePriorityFunctor;

    Handle const ds = DataSource::Cast(inputArgs->Get(key));
    if (!ds) {
        return nullptr;
    }
    return ds->GetTypedValue(0.0f);
}   

size_t
_GetNumPriorities(_PriorityFunctorHandle const &h)
{
    if (!h) {
        return 1;
    }
    const size_t result = h->GetNumPriorities();
    if (result == 0) {
        return 1;
    }
    return result;
}

}

HdsiPrimTypeNoticeBatchingSceneIndex::HdsiPrimTypeNoticeBatchingSceneIndex(
    HdSceneIndexBaseRefPtr const &inputScene,
    HdContainerDataSourceHandle const &inputArgs)
  : HdSingleInputFilteringSceneIndexBase(inputScene)
  , _primTypePriorityFunctor(_GetPrimTypePriorityFunctor(inputArgs))
  , _numPriorities(_GetNumPriorities(_primTypePriorityFunctor))
  , _populated(false)
{
    TRACE_FUNCTION();

    // Turn prims from input scene into queued up added prim entries.
    for (const SdfPath &path : HdSceneIndexPrimView(_GetInputSceneIndex())) {
        _addedOrDirtiedPrims[path] = _PrimAddedEntry{
            _GetInputSceneIndex()->GetPrim(path).primType};
    }
}

HdsiPrimTypeNoticeBatchingSceneIndex::
~HdsiPrimTypeNoticeBatchingSceneIndex() = default;

HdSceneIndexPrim
HdsiPrimTypeNoticeBatchingSceneIndex::GetPrim(const SdfPath &primPath) const
{
    if (!_populated) {
        return {};
    }

    return _GetInputSceneIndex()->GetPrim(primPath);
}

SdfPathVector
HdsiPrimTypeNoticeBatchingSceneIndex::
GetChildPrimPaths(const SdfPath &primPath) const
{
    if (!_populated) {
        return {};
    }

    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdsiPrimTypeNoticeBatchingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries)
    {
        // Override previous added or dirtied entry.
        _addedOrDirtiedPrims[entry.primPath] = _PrimAddedEntry{entry.primType};
    }
}

void
HdsiPrimTypeNoticeBatchingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries)
    {
        // If there was no item at given path, this will create a
        // _PrimDirtiedEntry (using default c'tor of variant).
        _PrimAddedOrDirtiedEntry &addedOrDirtiedEntry =
            _addedOrDirtiedPrims[entry.primPath];

        // Prim added entry is stronger than prim dirty entry.
        // That is, if we already had a prim added entry for this path,
        // we do nothing.

        if (_PrimDirtiedEntry * const dirtiedEntry =
                std::get_if<_PrimDirtiedEntry>(&addedOrDirtiedEntry)) {
            // If we had a previous or new prim dirty entry, add dirty
            // locators.
            dirtiedEntry->dirtyLocators.insert(entry.dirtyLocators);
        }
    }
}

void
HdsiPrimTypeNoticeBatchingSceneIndex::_RemovePathFromAddedOrDirtiedPrims(
    const SdfPath &path)
{
    // Remove all items having path has prefix.
    auto it = _addedOrDirtiedPrims.lower_bound(path);
    while (it != _addedOrDirtiedPrims.end() && it->first.HasPrefix(path)) {
        it = _addedOrDirtiedPrims.erase(it);
    }
}

void
HdsiPrimTypeNoticeBatchingSceneIndex::_AddPathToRemovedPrims(
    const SdfPath &path)
{
    auto it = _removedPrims.lower_bound(path);
    
    if (it != _removedPrims.end()) {
        if (path == *it) {
            // The path is already in _removedPrims, nothing to do.
            return;
        }
    }

    if (it != _removedPrims.begin()) {
        // Could point to the first ancestor of the given path in
        // _removedPrims.
        auto prev = it;
        --prev;
        if (path.HasPrefix(*prev)) {
            // A prefix of path is already in _removedPrims, nothing to do.
            return;
        }
    }

    // We need to add the path to _removedPrims. But delete any descendants
    // the path first.
    while (it != _removedPrims.end() && it->HasPrefix(path)) {
        it = _removedPrims.erase(it);
    }

    _removedPrims.insert(path);
}


void
HdsiPrimTypeNoticeBatchingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries)
    {
        // Update _addedOrDirtiedPrims and _removedPrims.
        _RemovePathFromAddedOrDirtiedPrims(entry.primPath);
        _AddPathToRemovedPrims(entry.primPath);
    }
}    

size_t
HdsiPrimTypeNoticeBatchingSceneIndex::
_GetPriority(const TfToken &primType) const
{
    if (!_primTypePriorityFunctor) {
        return 0;
    }

    const size_t priority =
        _primTypePriorityFunctor->GetPriorityForPrimType(primType);

    if (priority >= _numPriorities) {
        TF_CODING_ERROR(
            "Priority %zu for prim type %s exceeds _numPriorities %zu\n",
            priority, primType.GetText(), _numPriorities);
        return _numPriorities - 1;
    }

    return priority;
}

void
HdsiPrimTypeNoticeBatchingSceneIndex::Flush()
{
    TRACE_FUNCTION();

    // Filtering scene index is empty until first call to Flush.
    _populated = true;

    if (_IsObserved()) {
        if (!_removedPrims.empty()) {
            // First send all removed entries.
            HdSceneIndexObserver::RemovedPrimEntries removedEntries;
            removedEntries.reserve(_removedPrims.size());
            for (const SdfPath &path : _removedPrims) {
                removedEntries.push_back({path});
            }
            _SendPrimsRemoved(removedEntries);
        }

        if (!_addedOrDirtiedPrims.empty()) {
            // Then send alternatingly added and dirtied entries
            // by priority.

            // The entries grouped by priority.
            std::vector<HdSceneIndexObserver::AddedPrimEntries>
                addedEntries(_numPriorities);
            std::vector<HdSceneIndexObserver::DirtiedPrimEntries>
                dirtiedEntries(_numPriorities);

            for (const auto &pathAndEntry : _addedOrDirtiedPrims) {
                const SdfPath &path = pathAndEntry.first;
                const _PrimAddedOrDirtiedEntry &entry = pathAndEntry.second;

                if (const _PrimAddedEntry * const addedEntry =
                            std::get_if<_PrimAddedEntry>(&entry)) {
                    // Prim type from added entry
                    const TfToken &primType = addedEntry->primType;
                    const size_t priority = _GetPriority(primType);
                    addedEntries[priority].push_back(
                        {path, primType});
                } else {
                    const _PrimDirtiedEntry &dirtiedEntry =
                        std::get<_PrimDirtiedEntry>(entry);
                    // Prim type needs to be pulled from input scene.
                    const TfToken primType =
                        _GetInputSceneIndex()->GetPrim(path).primType;
                    const size_t priority = _GetPriority(primType);
                    dirtiedEntries[priority].push_back(
                        {path, dirtiedEntry.dirtyLocators});
                }
            }

            // Send out grouped entries.
            for (size_t priority = 0; priority < _numPriorities; priority++) {
                if (!addedEntries[priority].empty()) {
                    _SendPrimsAdded(addedEntries[priority]);
                }
                if (!dirtiedEntries[priority].empty()) {
                    _SendPrimsDirtied(dirtiedEntries[priority]);
                }
            }
        }
    }

    // Clear.
    _removedPrims.clear();
    _addedOrDirtiedPrims.clear();
}

PXR_NAMESPACE_CLOSE_SCOPE
