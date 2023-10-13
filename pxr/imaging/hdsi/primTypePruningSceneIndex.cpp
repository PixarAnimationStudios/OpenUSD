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
#include "pxr/imaging/hdsi/primTypePruningSceneIndex.h"

#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/trace/trace.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdsiPrimTypePruningSceneIndexTokens,
                        HDSI_PRIM_TYPE_PRUNING_SCENE_INDEX_TOKENS);

namespace {

// This _PrimDataSource filters out bindings at binding token
class _PrimDataSource final : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource);

    _PrimDataSource(
        HdContainerDataSourceHandle const &input,
        const HdsiPrimTypePruningSceneIndex * const si)
        : _input(input), _si(si)
    {
    }

    TfTokenVector GetNames() override
    {
        if (!_input) {
            return {};
        }
        TfTokenVector names = _input->GetNames();
        if (_si->GetEnabled()) {
            // Filter out binding
            names.erase(
                std::remove(names.begin(), names.end(),
                            _si->GetBindingToken()),
                names.end());
        }
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (!_input) {
            return nullptr;
        }
        if (_si->GetEnabled()) {
            // Filter out binding
            if (name == _si->GetBindingToken()) {
                return nullptr;
            }
        }
        return _input->Get(name);
    }

private:
    HdContainerDataSourceHandle const _input;
    const HdsiPrimTypePruningSceneIndex * const _si;
};

template<typename T>
T _Get(HdContainerDataSourceHandle const &container,
       const TfToken &token)
{
    if (!container) {
        return {};
    }
    using DataSource = HdTypedSampledDataSource<T>;
    typename DataSource::Handle const ds =
        DataSource::Cast(container->Get(token));
    if (!ds) {
        return {};
    }
    return ds->GetTypedValue(0.0f);
}

};

// static
HdsiPrimTypePruningSceneIndexRefPtr
HdsiPrimTypePruningSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    HdContainerDataSourceHandle const &inputArgs)
{
    return TfCreateRefPtr(new HdsiPrimTypePruningSceneIndex(
        inputSceneIndex, inputArgs));
}

bool
HdsiPrimTypePruningSceneIndex::_PruneType(const TfToken &primType) const {
    for (const TfToken &type : _primTypes) {
        if (primType == type) {
            return true;
        }
    }
    return false;
}

bool
HdsiPrimTypePruningSceneIndex::_PrunePath(const SdfPath &path) const {
    if (_doNotPruneNonPrimPaths) {
        return path.IsPrimPath();
    } else {
        return true;
    }
}

bool
HdsiPrimTypePruningSceneIndex::GetEnabled() const
{
    return _enabled;
}

void
HdsiPrimTypePruningSceneIndex::SetEnabled(const bool enabled)
{
    if (_enabled == enabled) {
        return;
    }
    HdSceneIndexBaseRefPtr const inputSceneIndex = _GetInputSceneIndex();

    TRACE_FUNCTION();

    // Precondition: _pruneMap can only have entries if we had previously been
    // pruning primTypes.
    TF_VERIFY(_pruneMap.empty() || _enabled);

    _enabled = enabled;

    HdSceneIndexObserver::DirtiedPrimEntries dirtiedEntries;

    // Invalidate all data source locators.  Even though the prim
    // data source we use here will double-check whether the scene index
    // has been enabled, we only insert those sparsely,
    // and only when needed, at the cost of making the required
    // invalidation more extensive.
    const HdDataSourceLocatorSet locators{ HdDataSourceLocator() };

    for (const SdfPath &primPath: HdSceneIndexPrimView(inputSceneIndex)) {
        // Note that we make the assumption here that a material and the prim
        // binding the material are either both at prim paths or non-prim paths.
        //
        // If this assumption is violated, we might remove the material without
        // updating the material binding or vice versa.
        if (!_PrunePath(primPath)) {
            continue;
        }
        // Consider changes to this prim
        const HdSceneIndexPrim prim = inputSceneIndex->GetPrim(primPath);
        if (_PruneType(prim.primType)) {
            if (_enabled) {
                // Prune this primType.
                _pruneMap[primPath] = true;
                dirtiedEntries.emplace_back(primPath, locators);
            } else {
                const _PruneMap::iterator i = _pruneMap.find(primPath);
                if (i != _pruneMap.end() && i->second) {
                    // Add back this previously-pruned prim.
                    //addedEntries.emplace_back(primPath, prim.primType);
                    // Don't bother erasing the _pruneMap entry;
                    // will clear below.
                    dirtiedEntries.emplace_back(primPath, locators);
                }
            }
        } else if (!_bindingToken.IsEmpty()) {
            if (prim.dataSource && prim.dataSource->Get(_bindingToken)) {
                // Dirty this prim's binding.
                dirtiedEntries.emplace_back(primPath, locators);
            }
        }
    }

    // Clear _pruneMap when turning pruning off.
    if (!_enabled) {
        _pruneMap.clear();
    }

    // Notify observers
    if (!dirtiedEntries.empty()) {
        _SendPrimsDirtied(dirtiedEntries);
    }
}

HdSceneIndexPrim
HdsiPrimTypePruningSceneIndex::GetPrim(const SdfPath &primPath) const
{
    auto const input = _GetInputSceneIndex();
    HdSceneIndexPrim prim = input->GetPrim(primPath);
    if (!_enabled) {
        return prim;
    }
    if (!_PrunePath(primPath)) {
        return prim;
    }
    if (_PruneType(prim.primType)) {
        // For pruned prims, we clear out the primType and null out the
        // dataSource.
        return { TfToken(), nullptr };
    } else {
        // Filter out scene primType prim entries
        if (!_bindingToken.IsEmpty()) {
            if (prim.dataSource) {
                prim.dataSource = _PrimDataSource::New(prim.dataSource, this);
            }
        }
        return prim;
    }
}

SdfPathVector
HdsiPrimTypePruningSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    SdfPathVector result;
    if (auto const input = _GetInputSceneIndex()) {
        result = input->GetChildPrimPaths(primPath);
    }
    return result;
}

void
HdsiPrimTypePruningSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    // Fast path: not filtering.
    if (!_enabled) {
        _SendPrimsAdded(entries);
        return;
    }

    // Fast path: if there are no primTypes, reuse the entry list.
    bool anythingToFilter = false;
    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        if (_PrunePath(entry.primPath) && _PruneType(entry.primType)) {
            anythingToFilter = true;
            break;
        }
    }
    if (!anythingToFilter) {
        _SendPrimsAdded(entries);
        return;
    }

    // PrimTypes are present.  Filter them out of the entries.
    HdSceneIndexObserver::AddedPrimEntries filteredEntries = entries;
    for (HdSceneIndexObserver::AddedPrimEntry &entry : filteredEntries) {
        if (_PrunePath(entry.primPath) && _PruneType(entry.primType)) {
            entry.primType = TfToken();
            _pruneMap[entry.primPath] = true;
        }
    }
    _SendPrimsAdded(filteredEntries);
}

void
HdsiPrimTypePruningSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

void
HdsiPrimTypePruningSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    // XXX We could, potentially, filter out entries for prims
    // we have pruned.  For now, we pass through (potentially
    // unnecessary) dirty notification.
    _SendPrimsDirtied(entries);
}

HdsiPrimTypePruningSceneIndex::HdsiPrimTypePruningSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    HdContainerDataSourceHandle const &inputArgs)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    , _primTypes(
        _Get<TfTokenVector>(
            inputArgs,
            HdsiPrimTypePruningSceneIndexTokens->primTypes))
    , _bindingToken(
        _Get<TfToken>(
            inputArgs,
            HdsiPrimTypePruningSceneIndexTokens->bindingToken))
    , _doNotPruneNonPrimPaths(
        _Get<bool>(
            inputArgs,
            HdsiPrimTypePruningSceneIndexTokens->doNotPruneNonPrimPaths))
    , _enabled(false)
{
    if (_primTypes.empty()) {
        TF_CODING_ERROR(
            "Empty prim types given to HdsiPrimTypePruningSceneIndex");
    }
}

HdsiPrimTypePruningSceneIndex::
~HdsiPrimTypePruningSceneIndex() = default;

PXR_NAMESPACE_CLOSE_SCOPE
