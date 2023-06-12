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
#include "pxr/imaging/hdsi/materialPruningSceneIndex.h"

#include "pxr/imaging/hd/materialBindingSchema.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/trace/trace.h"

#include <algorithm>
#include <queue>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// XXX We want to filter scene materials but retain materials used by
// applications, such as guides and 3D interaction widgets.
//
// For now, we use the heuristic of detecting such materials as
// non-prim-SdfPaths.  We would prefer something more explicit
// about intent, such as adding "purpose" to materials.  Currently,
// the Hydra1 object model of Sprims does not support purpose.
//
static bool
_IsCandidateForFiltering(const SdfPath &path)
{
    return path.IsPrimPath();
}


// This _PrimDataSource filters out materialBinding.
class _PrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource);

    _PrimDataSource(
        const HdContainerDataSourceHandle& input,
        const HdsiMaterialPruningSceneIndex *si)
        : _input(input), _si(si)
    {
    }

    TfTokenVector GetNames() override
    {
        if (!_input) {
            return {};
        }
        TfTokenVector names = _input->GetNames();
        if (!_si->GetSceneMaterialsEnabled()) {
            // Filter out materialBinding
            names.erase(
                std::remove(names.begin(), names.end(),
                            HdMaterialBindingSchemaTokens->materialBinding));
        }
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken& name) override
    {
        if (!_input) {
            return nullptr;
        }
        if (!_si->GetSceneMaterialsEnabled()) {
            // Filter out materialBinding
            if (name == HdMaterialBindingSchemaTokens->materialBinding) {
                return nullptr;
            }
        }
        return _input->Get(name);
    }

private:
    HdContainerDataSourceHandle _input;
    const HdsiMaterialPruningSceneIndex *_si;
};

};

// static
HdsiMaterialPruningSceneIndexRefPtr
HdsiMaterialPruningSceneIndex::New(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    return TfCreateRefPtr(new HdsiMaterialPruningSceneIndex(
        inputSceneIndex));
}

bool
HdsiMaterialPruningSceneIndex::GetSceneMaterialsEnabled() const
{
    return _materialsEnabled;
}

void
HdsiMaterialPruningSceneIndex::SetSceneMaterialsEnabled(bool materialsEnabled)
{
    if (_materialsEnabled == materialsEnabled) {
        return;
    }
    HdSceneIndexBaseRefPtr inputSceneIndex = _GetInputSceneIndex();
    if (!inputSceneIndex) {
        return;
    }

    TRACE_FUNCTION();

    // Precondition: _pruneMap can only have entries if we had previously been
    // pruning materials.
    TF_VERIFY(_pruneMap.empty() || !_materialsEnabled);

    _materialsEnabled = materialsEnabled;

    HdSceneIndexObserver::AddedPrimEntries addedEntries;
    HdSceneIndexObserver::RemovedPrimEntries removedEntries;
    HdSceneIndexObserver::DirtiedPrimEntries dirtiedEntries;

    // Invalidate all data source locators.  Even though the prim
    // data source we use here will double-check whether scene
    // materials are enabled, we only insert those sparsely,
    // and only when needed, at the cost of making the required
    // invalidation more extensive.
    const HdDataSourceLocatorSet materialBindingLocators =
        {HdDataSourceLocator()};

    for (const SdfPath &primPath: HdSceneIndexPrimView(inputSceneIndex)) {
        if (!_IsCandidateForFiltering(primPath)) {
            continue;
        }
        // Consider changes to this prim
        HdSceneIndexPrim prim = inputSceneIndex->GetPrim(primPath);
        if (prim.primType == HdPrimTypeTokens->material) {
            _PruneMap::iterator i = _pruneMap.find(primPath);
            if (_materialsEnabled) {
                if (i != _pruneMap.end() && i->second) {
                    // Add back this previously-pruned material.
                    addedEntries.emplace_back(primPath, prim.primType);
                    // Don't bother erasing the _pruneMap entry;
                    // will clear below.
                }
            } else {
                // Prune this material.
                _pruneMap[primPath] = true;
                removedEntries.emplace_back(primPath);
            }
        } else if (prim.dataSource && prim.dataSource->Get(
                   HdMaterialBindingSchemaTokens->materialBinding)) {
            // Dirty this prim's materialBinding.
            dirtiedEntries.emplace_back(primPath, materialBindingLocators);
        }
    }

    // Clear _pruneMap when turning pruning off.
    if (_materialsEnabled) {
        _pruneMap.clear();
    }

    // Notify observers
    if (!addedEntries.empty()) {
        _SendPrimsAdded(addedEntries);
    }
    if (!removedEntries.empty()) {
        _SendPrimsRemoved(removedEntries);
    }
    if (!dirtiedEntries.empty()) {
        _SendPrimsDirtied(dirtiedEntries);
    }
}

HdSceneIndexPrim
HdsiMaterialPruningSceneIndex::GetPrim(const SdfPath& primPath) const
{
    if (auto input = _GetInputSceneIndex()) {
        HdSceneIndexPrim prim = input->GetPrim(primPath);
        if (_materialsEnabled || !_IsCandidateForFiltering(primPath)) {
            return prim;
        } else if (prim.primType != HdPrimTypeTokens->material) {
            // Filter out scene material prim entries
            if (prim.dataSource) {
                prim.dataSource = _PrimDataSource::New(prim.dataSource, this);
            }
            return prim;
        }
    }
    return { TfToken(), nullptr };
}

SdfPathVector
HdsiMaterialPruningSceneIndex::GetChildPrimPaths(
    const SdfPath& primPath) const
{
    TRACE_FUNCTION();

    SdfPathVector result;
    if (auto input = _GetInputSceneIndex()) {
        result = input->GetChildPrimPaths(primPath);
        if (!_materialsEnabled) {
            // Filter out scene material prim entries
            result.erase(
                std::remove_if(result.begin(), result.end(),
                    [&](const SdfPath &path) {
                        return _IsCandidateForFiltering(path) &&
                            input->GetPrim(path).primType
                            == HdPrimTypeTokens->material;
                    }),
                result.end());
        }
    }
    return result;
}

void
HdsiMaterialPruningSceneIndex::_PrimsAdded(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::AddedPrimEntries& entries)
{
    TRACE_FUNCTION();

    // Fast path: not filtering.
    if (_materialsEnabled) {
        _SendPrimsAdded(entries);
        return;
    }

    // Fast path: if there are no materials, reuse the entry list.
    bool anythingToFilter = false;
    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        if (_IsCandidateForFiltering(entry.primPath) &&
            entry.primType == HdPrimTypeTokens->material) {
            anythingToFilter = true;
            break;
        }
    }
    if (!anythingToFilter) {
        _SendPrimsAdded(entries);
        return;
    }

    // Materials are present.  Filter them out of the entries.
    HdSceneIndexObserver::AddedPrimEntries filteredEntries;
    filteredEntries.reserve(entries.size());
    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        if (_IsCandidateForFiltering(entry.primPath) &&
            entry.primType == HdPrimTypeTokens->material) {
            _pruneMap[entry.primPath] = true;
        } else {
            filteredEntries.push_back(entry);
        }
    }
    _SendPrimsAdded(filteredEntries);
}

void
HdsiMaterialPruningSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::RemovedPrimEntries& entries)
{
    TRACE_FUNCTION();

    // Fast path: not filtering.
    if (_materialsEnabled) {
        _SendPrimsRemoved(entries);
        return;
    }

    // Fast path: if there are no materials, we can reuse the entry list.
    bool anythingToFilter = false;
    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        _PruneMap::iterator i = _pruneMap.find(entry.primPath);
        if (i != _pruneMap.end() && i->second) {
            // Found a material.
            anythingToFilter = true;
            break;
        }
    }
    if (!anythingToFilter) {
        _SendPrimsRemoved(entries);
        return;
    }

    // Materials are present.  Filter them out of the entries.
    HdSceneIndexObserver::RemovedPrimEntries filteredEntries;
    filteredEntries.reserve(entries.size());
    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        _PruneMap::iterator i = _pruneMap.find(entry.primPath);
        if (i == _pruneMap.end() || !i->second) {
            filteredEntries.push_back(entry);
        }
        if (i != _pruneMap.end()) {
            _pruneMap.erase(i);
        }
    }
    _SendPrimsRemoved(filteredEntries);
}

void
HdsiMaterialPruningSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase& sender,
    const HdSceneIndexObserver::DirtiedPrimEntries& entries)
{
    TRACE_FUNCTION();

    // Fast path: not filtering.
    if (_materialsEnabled) {
        _SendPrimsDirtied(entries);
        return;
    }

    // Fast path: if there are no materials, we can reuse the entry list.
    bool anythingToFilter = false;
    for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
        _PruneMap::iterator i = _pruneMap.find(entry.primPath);
        if (i != _pruneMap.end() && i->second) {
            anythingToFilter = true;
            break;
        }
    }
    if (!anythingToFilter) {
        _SendPrimsDirtied(entries);
        return;
    }

    // Materials are present.  Filter them out of the entries.
    HdSceneIndexObserver::DirtiedPrimEntries filteredEntries;
    filteredEntries.reserve(entries.size());
    for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
        _PruneMap::iterator i = _pruneMap.find(entry.primPath);
        if (i == _pruneMap.end() || !i->second) {
            filteredEntries.push_back(entry);
        }
    }
    _SendPrimsDirtied(filteredEntries);
}

HdsiMaterialPruningSceneIndex::HdsiMaterialPruningSceneIndex(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    , _materialsEnabled(true)
{
}

HdsiMaterialPruningSceneIndex::
    ~HdsiMaterialPruningSceneIndex()
    = default;

PXR_NAMESPACE_CLOSE_SCOPE
