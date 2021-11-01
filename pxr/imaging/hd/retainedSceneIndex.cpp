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
#include "pxr/imaging/hd/retainedSceneIndex.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

HdRetainedSceneIndex::HdRetainedSceneIndex() = default;

void
HdRetainedSceneIndex::AddPrims(const AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    HdSceneIndexObserver::AddedPrimEntries observerEntries;
    observerEntries.reserve(entries.size());

    for (const AddedPrimEntry &entry : entries) {
        observerEntries.emplace_back(entry.primPath, entry.primType);
        _entries[entry.primPath] = {entry.primType, entry.dataSource};
    }

    _SendPrimsAdded(observerEntries);
}

void
HdRetainedSceneIndex::RemovePrims(
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        _entries.erase(entry.primPath);
    }

    _SendPrimsRemoved(entries);
}

void
HdRetainedSceneIndex::DirtyPrims(
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    // NOTE: Filtering the DirtyPrims notices to include only paths which are
    //       present in the internal table. This is (currently) useful as
    //       front-end emulation makes use of an HdRetainedSceneIndex in order
    //       to transfer population and value queries into a scene index. The
    //       current implementation shares a single render index and some
    //       emulated actions cause all prims to be dirtied -- which can
    //       include prims not within this scene index.
    //
    //       This filtering behavior may still be desired independent of the
    //       emulation case which inspired it.
    HdSceneIndexObserver::DirtiedPrimEntries observerEntries;
    observerEntries.reserve(entries.size());
    for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
        if (_entries.find(entry.primPath) == _entries.end()) {
            continue;
        }
        observerEntries.emplace_back(entry.primPath, entry.dirtyLocators);
    }

    _SendPrimsDirtied(observerEntries);
}

HdSceneIndexPrim
HdRetainedSceneIndex::GetPrim(const SdfPath & primPath) const {
    const auto it = _entries.find(primPath);

    if (it != _entries.end()) {
        return it->second.prim;
    }

    return {TfToken(), nullptr};

}

SdfPathVector
HdRetainedSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    SdfPathVector result;

    _PrimEntryTable::const_iterator it = _entries.find(primPath);
    if (it == _entries.end()) {
        return result;
    }

    // increment is depth-first so this will get first child
    ++it;
    while (it != _entries.end()
            && it->first.GetParentPath() == primPath) {

        result.push_back(it->first);

        // we want a sibling so we can't use increment
        it = it.GetNextSubtree();
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE
