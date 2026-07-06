// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hdsi/renderPassPruneSceneIndex.h"
#if PXR_VERSION >= 2408

#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/collectionsSchema.h"
#include "pxr/imaging/hd/containerDataSourceEditor.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/schema.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hdsi/utils.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (prune)
);

/* static */
HdsiRenderPassPruneSceneIndexRefPtr
HdsiRenderPassPruneSceneIndex::New(
    const HdSceneIndexBaseRefPtr& inputSceneIndex)
{
    return TfCreateRefPtr(  
        new HdsiRenderPassPruneSceneIndex(inputSceneIndex));
}

HdsiRenderPassPruneSceneIndex::HdsiRenderPassPruneSceneIndex(
    const HdSceneIndexBaseRefPtr &inputSceneIndex)
: HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
{
}

bool
HdsiRenderPassPruneSceneIndex::_RenderPassPruneState::DoesPrune(
    SdfPath const& primPath) const
{
    return pruneEval ? HdsiUtilsIsPruned(primPath, *pruneEval) : false;
}

HdSceneIndexPrim 
HdsiRenderPassPruneSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    // Note that we also apply pruning in GetChildPrimPaths(), but
    // this ensures that even if a downstream scene index asks
    // for a pruned path, it will remain pruned.
    if (primPath.IsEmpty()) {
        // Avoid triggering a runtime warning when passing an empty
        // path to the collection expression evaluator, since it
        // can generate a lot of spew.
        return HdSceneIndexPrim();
    }
    if (_activeRenderPass.DoesPrune(primPath)) {
        return HdSceneIndexPrim();
    }

    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    // For instancers: populate the instance-location→instancer map, and
    // apply pruning to the instance mask if pruning is active.
    if (prim.primType == HdPrimTypeTokens->instancer) {
        if (HdInstancerTopologySchema topologySchema =
            HdInstancerTopologySchema::GetFromParent(prim.dataSource))
        {
            HdPathArrayDataSourceHandle instanceLocationPathsDs =
                topologySchema.GetInstanceLocations();
            VtArray<SdfPath> instanceLocationPaths =
                instanceLocationPathsDs
                ? instanceLocationPathsDs->GetTypedValue(0.0)
                : VtArray<SdfPath>();

            // Track which instancers use each instance location so that
            // prune changes can dirty only the relevant instancer masks.
            // GetPrim() can be called from worker threads, so guard writes.
            {
                std::unique_lock<std::mutex> instancerLock(_instancerMutex);
                for (const SdfPath &path : instanceLocationPaths) {
                    _instanceToInstancerMap[path].insert(primPath);
                }
            }

            // Apply pruning to the instance mask.
            if (!instanceLocationPaths.empty() && _activeRenderPass.pruneEval) {
                HdBoolArrayDataSourceHandle maskDs = topologySchema.GetMask();
                VtArray<bool> mask =
                    maskDs
                    ? maskDs->GetTypedValue(0.0)
                    : VtArray<bool>(instanceLocationPaths.size(), true);

                if (instanceLocationPaths.size() == mask.size()) {
                    bool maskWasModified = false;
                    for (size_t i = 0, n = mask.size(); i < n; ++i) {
                        if (!mask[i]) {
                            continue;
                        }
                        mask[i] = !HdsiUtilsIsPruned(
                            instanceLocationPaths[i],
                            *_activeRenderPass.pruneEval);
                        maskWasModified |= !mask[i];
                    }
                    if (maskWasModified) {
                        static const auto instancerMaskLocator =
                            HdInstancerTopologySchema::GetDefaultLocator()
                                .Append(HdInstancerTopologySchemaTokens->mask);
                        HdContainerDataSourceEditor editor(prim.dataSource);
                        editor.Set(instancerMaskLocator,
                                   HdCreateTypedRetainedDataSource(
                                       VtValue(mask)));
                        prim.dataSource = editor.Finish();
                    }
                } else {
                    TF_RUNTIME_ERROR("Instancer topology mismatch in <%s>",
                                     primPath.GetText());
                }
            }
        }
    }

    return prim;
}

SdfPathVector 
HdsiRenderPassPruneSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    if (_activeRenderPass.pruneEval) {
        SdfPathVector childPathVec =
            _GetInputSceneIndex()->GetChildPrimPaths(primPath);
        HdsiUtilsRemovePrunedChildren(primPath, *_activeRenderPass.pruneEval,
                                      &childPathVec);
        return childPathVec;
    } else {
        return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
    }
}

/*

General notes on change processing and invalidation:

- Rather than lazily evaluate the active render pass state,
  and be prepared to do so from multiple caller threads, we
  instead greedily set up the active render pass state.
  Though greedy, this is a small amount of computation,
  and only triggered on changes to two specific scene locations:
  the root scope where HdSceneGlobalsSchema lives, and the
  scope where the designated active render pass lives.

- The list of entries for prims added, dirtied, or removed
  must be filtered against the active render pass prune collection.

- The list of entries for prims added, dirtied, or removed
  can imply changes to which render pass is active, or to the
  contents of the active render pass.  In either case, if the
  effective render pass state changes, downstream observers
  must be notified about the effects.

- The use of HdContainerDataSourceEditor requires sentinel
  invalidation; use of see ComputeDirtyLocators().

*/

namespace {

// Helper to scan an entry vector for an entry that
// could affect the active render pass.
template <typename ENTRIES>
inline bool
_EntryCouldAffectPass(
    const ENTRIES &entries,
    SdfPath const& activeRenderPassPath)
{
    for (const auto& entry: entries) {
        // The prim at the root path contains the HdSceneGlobalsSchema.
        // The prim at the render pass path controls its behavior.
        if (entry.primPath.IsAbsoluteRootPath()
            || entry.primPath == activeRenderPassPath) {
            return true;
        }
    }
    return false;
}

// Helper to apply pruning to an entry list.
// Returns true if any pruning was applied, putting surviving entries
// into *postPruneEntries.
template <typename ENTRIES>
inline bool
_PruneEntries(
    std::optional<HdCollectionExpressionEvaluator> &pruneEval,
    const ENTRIES &entries, ENTRIES *postPruneEntries)
{
    if (!pruneEval) {
        // No pruning active.
        return false;
    }
    // Pre-pass to see if any prune applies to the list.
    bool foundEntryToPrune = false;
    for (const auto& entry: entries) {
        if (HdsiUtilsIsPruned(entry.primPath, *pruneEval)) {
            foundEntryToPrune = true;
            break;
        }
    }
    if (!foundEntryToPrune) {
        // No entries to prune.
        return false;
    } else {
        // Prune matching entries.
        for (const auto& entry: entries) {
            if (!HdsiUtilsIsPruned(entry.primPath, *pruneEval)) {
                // Accumulate survivors.
                postPruneEntries->push_back(entry);
            }
        }
        return true;
    }
}

} // anon

void
HdsiRenderPassPruneSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    HdSceneIndexObserver::AddedPrimEntries extraAddedEntries;
    HdSceneIndexObserver::RemovedPrimEntries extraRemovedEntries;

    // Check if any entry could affect the active render pass.
    if (_EntryCouldAffectPass(entries, _activeRenderPass.renderPassPath)) {
        _UpdateActiveRenderPassState(&extraAddedEntries, &extraRemovedEntries);
    }

    // Mark that we have populated prims.
    _hasPopulated = true;

    // Filter entries against any active render pass prune collection.
    if (!_PruneEntries(_activeRenderPass.pruneEval, entries,
                       &extraAddedEntries)) {
        _SendPrimsAdded(entries);
    }

    _SendPrimsAdded(extraAddedEntries);
    _SendPrimsRemoved(extraRemovedEntries);
}

void
HdsiRenderPassPruneSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    HdSceneIndexObserver::AddedPrimEntries extraAddedEntries;
    HdSceneIndexObserver::RemovedPrimEntries extraRemovedEntries;

    // Check if any entry could affect the active render pass.
    if (_EntryCouldAffectPass(entries, _activeRenderPass.renderPassPath)) {
        _UpdateActiveRenderPassState(&extraAddedEntries, &extraRemovedEntries);
    }

    // Filter entries against any active render pass prune collection.
    if (!_PruneEntries(_activeRenderPass.pruneEval, entries,
                       &extraRemovedEntries)) {
        _SendPrimsRemoved(entries);
    }

    _SendPrimsAdded(extraAddedEntries);
    _SendPrimsRemoved(extraRemovedEntries);
}

void
HdsiRenderPassPruneSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    HdSceneIndexObserver::AddedPrimEntries extraAddedEntries;
    HdSceneIndexObserver::DirtiedPrimEntries extraDirtyEntries;
    HdSceneIndexObserver::RemovedPrimEntries extraRemovedEntries;

    // Check if any entry could affect the active render pass.
    if (_EntryCouldAffectPass(entries, _activeRenderPass.renderPassPath)) {
        _UpdateActiveRenderPassState(&extraAddedEntries, &extraRemovedEntries);
    }

    // Filter incoming entries against any active render pass prune collection.
    if (!_PruneEntries(_activeRenderPass.pruneEval, entries,
                       &extraDirtyEntries)) {
        _SendPrimsDirtied(entries);
    }

    // Dirty the instance mask only on instancers whose instance locations
    // were affected by the prune change (newly pruned or un-pruned paths).
    if (!_instanceToInstancerMap.empty()
        && (!extraRemovedEntries.empty() || !extraAddedEntries.empty()))
    {
        static const HdDataSourceLocatorSet instancerMaskLocatorSet =
            HdContainerDataSourceEditor::ComputeDirtyLocators(
                {HdInstancerTopologySchema::GetDefaultLocator()
                    .Append(HdInstancerTopologySchemaTokens->mask)});

        SdfPathSet affectedInstancers;
        for (const auto &entry : extraRemovedEntries) {
            const auto [begin, end] =
                _instanceToInstancerMap.FindSubtreeRange(entry.primPath);
            for (auto i = begin; i != end; ++i) {
                affectedInstancers.insert(i->second.begin(), i->second.end());
            }
        }
        for (const auto &entry : extraAddedEntries) {
            const auto [begin, end] =
                _instanceToInstancerMap.FindSubtreeRange(entry.primPath);
            for (auto i = begin; i != end; ++i) {
                affectedInstancers.insert(i->second.begin(), i->second.end());
            }
        }
        for (const SdfPath &instancerPath : affectedInstancers) {
            extraDirtyEntries.emplace_back(instancerPath,
                                           instancerMaskLocatorSet);
        }
    }

    _SendPrimsAdded(extraAddedEntries);
    _SendPrimsRemoved(extraRemovedEntries);
    _SendPrimsDirtied(extraDirtyEntries);
}

HdsiRenderPassPruneSceneIndex::~HdsiRenderPassPruneSceneIndex() = default;

void
HdsiRenderPassPruneSceneIndex::_UpdateActiveRenderPassState(
    HdSceneIndexObserver::AddedPrimEntries *addedEntries,
    HdSceneIndexObserver::RemovedPrimEntries *removedEntries)
{
    TRACE_FUNCTION();

    // Swap out the prior pass state to compare against.
    _RenderPassPruneState &state = _activeRenderPass;
    _RenderPassPruneState priorState;
    std::swap(state, priorState);

    // Check upstream scene index for an active render pass.
    HdSceneIndexBaseRefPtr inputSceneIndex = _GetInputSceneIndex();
    HdSceneGlobalsSchema globals =
        HdSceneGlobalsSchema::GetFromSceneIndex(inputSceneIndex);
    if (HdPathDataSourceHandle pathDs = globals.GetActiveRenderPassPrim()) {
        state.renderPassPath = pathDs->GetTypedValue(0.0);
    }
    if (state.renderPassPath.IsEmpty() && priorState.renderPassPath.IsEmpty()) {
        // Avoid further work if no render pass was or is active.
        return;
    }
    if (!state.renderPassPath.IsEmpty()) {
        const HdSceneIndexPrim passPrim =
            inputSceneIndex->GetPrim(state.renderPassPath);
        if (HdCollectionsSchema collections =
            HdCollectionsSchema::GetFromParent(passPrim.dataSource)) {
            HdsiUtilsCompileCollection(collections, _tokens->prune,
                                       inputSceneIndex,
                                       &state.pruneExpr,
                                       &state.pruneEval);
        }
    }

    if (state.pruneExpr == priorState.pruneExpr) {
        // Pattern unchanged; nothing to invalidate.
        return;
    }

    // Generate change entries for affected prims.
    // Consider all upstream prims.
    //
    // TODO: HdCollectionExpressionEvaluator::PopulateAllMatches()
    // should be used here instead, since in the future it will handle
    // instance matches as well as parallel traversal.
    //
    // We can skip this part if this is the initial round of population.
    if (!_hasPopulated) {
        return;
    }
    for (const SdfPath &path: HdSceneIndexPrimView(_GetInputSceneIndex())) {
        if (priorState.DoesPrune(path)) {
            // The prim had been pruned.
            if (!state.DoesPrune(path)) {
                // The prim is no longer pruned, so add it back.
                HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(path);
                addedEntries->push_back({path, prim.primType});
            } else {
                // The prim is still pruned, so nothing to do.
            }
        } else if (state.DoesPrune(path)) {
            // The prim is newly pruned, so remove it.
            removedEntries->push_back({path});
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
#endif //PXR_VERSION >= 2408
