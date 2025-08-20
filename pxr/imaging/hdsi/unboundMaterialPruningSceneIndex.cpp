//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hdsi/unboundMaterialPruningSceneIndex.h"

#include "pxr/imaging/hd/materialBindingSchema.h"
#include "pxr/imaging/hd/materialBindingsSchema.h"
#include "pxr/imaging/hd/materialSchema.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"

#include "tbb/concurrent_vector.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(
    HdsiUnboundMaterialPruningSceneIndexTokens,
    HDSI_UNBOUND_MATERIAL_PRUNING_SCENE_INDEX_TOKENS);

namespace
{
VtArray<TfToken>
_GetMaterialBindingPurposes(
    HdContainerDataSourceHandle const &inputArgs)
{
    const HdDataSourceBaseHandle ds =
        inputArgs->Get(
            HdsiUnboundMaterialPruningSceneIndexTokens->
                materialBindingPurposes);

    if (HdTokenArrayDataSourceHandle tokensDs =
            HdTokenArrayDataSource::Cast(ds)) {
        
        return tokensDs->GetTypedValue(0.0f);
    }

    return {};
}

bool
_Contains(
    const std::unordered_set<SdfPath, SdfPath::Hash> &paths,
    const SdfPath &primPath)
{
    return paths.find(primPath) != paths.end();
}

SdfPathVector
_GetBoundMaterialPaths(
    const HdContainerDataSourceHandle &primContainer,
    const VtArray<TfToken> &bindingPurposes)
{
    HdMaterialBindingsSchema bindingsSchema =
        HdMaterialBindingsSchema::GetFromParent(primContainer);
    
    if (!bindingsSchema) {
        return {};
    }

    SdfPathVector materialBindingPaths;

    for (const TfToken &purpose : bindingPurposes) {
        HdMaterialBindingSchema bindingSchema =
            bindingsSchema.GetMaterialBinding(purpose);
        
        const HdPathDataSourceHandle ds = bindingSchema.GetPath();
        if (!ds) {
            continue;
        }

        materialBindingPaths.push_back(ds->GetTypedValue(0.0f));
    }

    return materialBindingPaths;
}

HdDataSourceLocatorSet _ComputeBindingLocators(
    const VtArray<TfToken> &bindingPurposes)
{
    HdDataSourceLocatorSet locators;
    for (const TfToken &purpose : bindingPurposes) {
        locators.insert(
            HdMaterialBindingsSchema::GetDefaultLocator()
                .Append(purpose));
    }
    return locators;
}

} // anon

// static
HdsiUnboundMaterialPruningSceneIndexRefPtr
HdsiUnboundMaterialPruningSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    HdContainerDataSourceHandle const &inputArgs)
{
    return TfCreateRefPtr(new HdsiUnboundMaterialPruningSceneIndex(
        inputSceneIndex, inputArgs));
}

HdSceneIndexPrim
HdsiUnboundMaterialPruningSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (prim.primType == HdPrimTypeTokens->material &&
        !_IsBoundMaterial(primPath)) {        
        // Clear both prim type and container. We don't need to be lazy here
        // because we re-add the material when it is bound.
        prim.primType = TfToken();
        prim.dataSource = nullptr;
    }

    return prim;
}

SdfPathVector
HdsiUnboundMaterialPruningSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    // This scene index does not remove unbound material prims from the scene
    // topology. It only overrides their prim type and container.
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdsiUnboundMaterialPruningSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{    
    TRACE_FUNCTION();
    using _ConcurrentIndexVector = tbb::concurrent_vector<size_t>;
    using _ConcurrentPathVector = tbb::concurrent_vector<SdfPath>;

    _ConcurrentIndexVector addedMaterialIndices;
    _ConcurrentPathVector boundMaterialPaths;

    // Querying each prim to get the material bindings can be expensive, so we
    // amortize this cost.
    {
        TRACE_FUNCTION_SCOPE("Parallel notice processing");
        
        WorkParallelForN(
            entries.size(),
            [&](size_t begin, size_t end) {
                for (size_t i = begin; i < end; ++i) {
                    const HdSceneIndexObserver::AddedPrimEntry &entry =
                        entries[i];
        
                    if (entry.primType.IsEmpty()) {
                        // Ignore bindings on intermediate prims (like scopes 
                        // and xforms) for whom material bindings are not 
                        // relevant but present from flattening.
                        continue;
                    }
                    if (entry.primType == HdPrimTypeTokens->material) {
                        // Track the index for processing below.
                        addedMaterialIndices.push_back(i);
                        continue;
                    }
        
                    const HdSceneIndexPrim prim =
                        _GetInputSceneIndex()->GetPrim(entry.primPath);
        
                    const SdfPathVector materialPaths =
                        _GetBoundMaterialPaths(
                            prim.dataSource, _bindingPurposes);
        
                    for (const SdfPath &materialPath : materialPaths) {
                        boundMaterialPaths.push_back(materialPath);
                    }
                }
            });
    }

    if (addedMaterialIndices.empty() && boundMaterialPaths.empty()) {
        // No materials nor prims with bound materials were added.
        _SendPrimsAdded(entries);
        return;
    }

    // Track bound materials that were previously added, but never bound ...
    SdfPathVector newlyBoundMaterials;
    for (const SdfPath &boundMatPath : boundMaterialPaths) {
        if (_WasAdded(boundMatPath) && !_IsBoundMaterial(boundMatPath)) {
            newlyBoundMaterials.push_back(boundMatPath);
        }
    }

    // ... and update our tracking set of bound materials.
    _boundMaterialPaths.insert(
        boundMaterialPaths.begin(), boundMaterialPaths.end());

    // Track added materials that aren't bound ...
    std::vector<size_t> addedUnboundMaterialIndices;
    for (size_t i : addedMaterialIndices) {
        const auto &matPath = entries[i].primPath;
        if (!_IsBoundMaterial(matPath)) {
            addedUnboundMaterialIndices.push_back(i);
        }
        // ... and update our tracking set of added materials.
        _addedMaterialPaths.insert(matPath);
    }
    
    // Avoid copying the notice entries.
    if (newlyBoundMaterials.empty() && addedUnboundMaterialIndices.empty()) {
        _SendPrimsAdded(entries);
        return;
    }

    // Clear the prim type on added-but-still-unbound material notices ...
    HdSceneIndexObserver::AddedPrimEntries editedEntries(entries);
    for (size_t i : addedUnboundMaterialIndices) {
        editedEntries[i].primType = TfToken();
    }
    // ... and re-add previously-added-but-now-bound materials.
    for (const SdfPath &materialPath : newlyBoundMaterials) {
        editedEntries.push_back(
            {materialPath, HdPrimTypeTokens->material});
    }

    _SendPrimsAdded(editedEntries);
}

void
HdsiUnboundMaterialPruningSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        _addedMaterialPaths.erase(entry.primPath);
        _boundMaterialPaths.erase(entry.primPath);
   }
    _SendPrimsRemoved(entries);
}

void
HdsiUnboundMaterialPruningSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();
    
    // Below, we check if the material binding locators have changed and update
    // our tracking set of bound materials and re-add newly bound materials
    // we've seen before similar to the logic in _PrimsAdded.
    //
    HdSceneIndexObserver::AddedPrimEntries newlyBoundEntries;
       
    for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
        if (entry.dirtyLocators.Intersects(_bindingLocators)) {

            const HdSceneIndexPrim prim =
                _GetInputSceneIndex()->GetPrim(entry.primPath);

            if (prim.primType.IsEmpty()) {
                // Ignore bindings on intermediate prims, like in _PrimsAdded.
                continue;
            }

            const SdfPathVector boundMaterialPaths =
                _GetBoundMaterialPaths(prim.dataSource, _bindingPurposes);

            for (const SdfPath &materialPath : boundMaterialPaths) {
                if (!_IsBoundMaterial(materialPath)) {
                    if (_WasAdded(materialPath)) {
                        newlyBoundEntries.push_back(
                            {materialPath, HdPrimTypeTokens->material});
                    }
                    _boundMaterialPaths.insert(materialPath);
                }
            }
        }
    }

    _SendPrimsAdded(newlyBoundEntries);
    _SendPrimsDirtied(entries);
}

HdsiUnboundMaterialPruningSceneIndex::HdsiUnboundMaterialPruningSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    HdContainerDataSourceHandle const &inputArgs)
: HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
, _bindingPurposes(_GetMaterialBindingPurposes(inputArgs))
, _bindingLocators(_ComputeBindingLocators(_bindingPurposes))
{
    _PopulateFromInputSceneIndex();
}

HdsiUnboundMaterialPruningSceneIndex::
~HdsiUnboundMaterialPruningSceneIndex() = default;

void
HdsiUnboundMaterialPruningSceneIndex::_PopulateFromInputSceneIndex()
{
    TRACE_FUNCTION();
    
    // Track all material prim paths to find unbound materials.
    // Having sorted paths (SdfPathSet) lets us use set_difference below.
    SdfPathSet allMaterialPaths;

    for (const SdfPath &primPath :
            HdSceneIndexPrimView(_GetInputSceneIndex())) {

        const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

        if (prim.primType.IsEmpty()) {
            // Ignore bindings on non-geometry prims. This captures most of
            // the intermediate prims (like scopes and xforms) for whom material
            // bindings are not relevant but present from flattening.
            continue;
        }

        if (prim.primType == HdPrimTypeTokens->material) {
            _addedMaterialPaths.insert(primPath);
            continue;
        }

        // Check if this prim has bound materials.
        const SdfPathVector boundMaterialPaths =
            _GetBoundMaterialPaths(prim.dataSource, _bindingPurposes);
        
        if (boundMaterialPaths.empty()) {
            continue;
        }
        
        // Add the bound material paths to our tracking set.
        _boundMaterialPaths.insert(
            boundMaterialPaths.begin(), boundMaterialPaths.end());

    }

    // Prune unbound materials by sending an added notice with an empty prim
    // type. Note that we *don't* remove the unbound materials.
    SdfPathVector unboundMaterialPaths;
    std::set_difference(
        allMaterialPaths.begin(), allMaterialPaths.end(),
        _boundMaterialPaths.begin(), _boundMaterialPaths.end(),
        std::back_inserter(unboundMaterialPaths));
    
    if (!unboundMaterialPaths.empty()) {
        HdSceneIndexObserver::AddedPrimEntries addedEntries;
        for (const SdfPath &primPath : unboundMaterialPaths) {
            addedEntries.push_back({primPath, TfToken()});
        }
        _SendPrimsAdded(addedEntries);
    }
}

bool
HdsiUnboundMaterialPruningSceneIndex::_IsBoundMaterial(
    const SdfPath &primPath) const
{
    return _Contains(_boundMaterialPaths, primPath);
}

bool
HdsiUnboundMaterialPruningSceneIndex::_WasAdded(
    const SdfPath &primPath) const
{
    return _Contains(_addedMaterialPaths, primPath);
}

PXR_NAMESPACE_CLOSE_SCOPE