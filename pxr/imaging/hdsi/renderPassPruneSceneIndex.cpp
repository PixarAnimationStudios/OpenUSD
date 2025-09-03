// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hdsi/renderPassPruneSceneIndex.h"
#if PXR_VERSION >= 2408

#include "pxr/imaging/hd/version.h"

#include "pxr/imaging/hd/collectionsSchema.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/schema.h"
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
    return pruneEval && pruneEval->Match(primPath);
}

HdSceneIndexPrim 
HdsiRenderPassPruneSceneIndex::GetPrim(
    const SdfPath &primPath) const
{
    // Note that we also apply pruning in GetChildPrimPaths(), but
    // this ensures that even if a downstream scene index asks
    // for a pruned path, it will remain pruned.
    if (_activeRenderPass.DoesPrune(primPath)) {
        return HdSceneIndexPrim();
    } else {
        return _GetInputSceneIndex()->GetPrim(primPath);
    }
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
        if (pruneEval->Match(entry.primPath)) {
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
            if (!pruneEval->Match(entry.primPath)) {
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
    HdSceneIndexObserver::AddedPrimEntries extraAddedEntries;
    HdSceneIndexObserver::DirtiedPrimEntries extraDirtyEntries;
    HdSceneIndexObserver::RemovedPrimEntries extraRemovedEntries;

    // Check if any entry could affect the active render pass.
    if (_EntryCouldAffectPass(entries, _activeRenderPass.renderPassPath)) {
        _UpdateActiveRenderPassState(&extraAddedEntries, &extraRemovedEntries);
    }

    // Filter entries against any active render pass prune collection.
    if (!_PruneEntries(_activeRenderPass.pruneEval, entries,
                       &extraDirtyEntries)) {
        _SendPrimsDirtied(entries);
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
