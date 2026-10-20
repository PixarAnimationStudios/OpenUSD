//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#include "pxr/imaging/hdsi/purposeFilteringSceneIndex.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/purposeSchema.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexCreateArgsSchema.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/trace/trace.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEBUG_CODES(HDSI_PURPOSE_FILTERING_SCENE_INDEX);

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(HDSI_PURPOSE_FILTERING_SCENE_INDEX,
        "Logging for the purpose filtering scene index");
}

TF_DEFINE_PUBLIC_TOKENS(HdsiPurposeFilteringSceneIndexTokens,
    HDSI_PURPOSE_FILTERING_SCENE_INDEX_TOKENS);

namespace
{

// Returns the array of accepted purposes if present, otherwise nullopt.
std::optional<VtArray<TfToken>>
_GetAcceptedPurposes(HdContainerDataSourceHandle const &inputArgs)
{
    if (!inputArgs) {
        return std::nullopt;
    }
    auto const ds =
        HdTokenArrayDataSource::Cast(
            inputArgs->Get(
                HdsiPurposeFilteringSceneIndexTokens->purposes));
    if (!ds) {
        return std::nullopt;
    }
    return ds->GetTypedValue(0.0f);
}

// Returns the excluded purposes hint if present, otherwise an empty array.
VtArray<TfToken>
_GetExcludedPurposesHint(HdContainerDataSourceHandle const &inputArgs)
{
    const HdSceneIndexCreateArgsSchema createArgsSchema(inputArgs);
    if (!createArgsSchema) {
        return {};
    }

    if (const auto ds = createArgsSchema.GetExcludedPurposesHint()) {
        return ds->GetTypedValue(0.0f);
    }
    return {};
}

// Returns the purpose of the prim if present, otherwise nullopt.
std::optional<TfToken>
_GetPurpose(HdContainerDataSourceHandle const &primDs)
{
    const auto purposeDs = HdPurposeSchema::GetFromParent(primDs).GetPurpose();
    return purposeDs
        ? std::optional<TfToken>(purposeDs->GetTypedValue(0.0f))
        : std::nullopt;
}

// Unified pruning predicate.
//
// - If purposes has a value and is non-empty: prune if purpose is NOT in
//      *purposes
// - If purposes is nullopt (hint mode): prune if purpose IS in hint.
bool
_ComputeShouldPrune(
    const std::optional<TfToken> &primPurpose,
    const std::optional<VtArray<TfToken>> &purposesToKeep,
    const VtArray<TfToken> &excludedPurposesHint)
{
    if (!primPurpose) {
        return false;
    }

    if (purposesToKeep.has_value()) {
        if (purposesToKeep->empty()) {
            return false;
        }
        return std::find(
            purposesToKeep->begin(), purposesToKeep->end(), *primPurpose)
                == purposesToKeep->end();
    }

    // Hint mode.
    return std::find(
        excludedPurposesHint.begin(), excludedPurposesHint.end(), *primPurpose)
            != excludedPurposesHint.end();
}

} // anon namespace

// ----------------------------------------------------------------------------

/* static */
HdsiPurposeFilteringSceneIndexRefPtr
HdsiPurposeFilteringSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    HdContainerDataSourceHandle const &inputArgs)
{
    return TfCreateRefPtr(
        new HdsiPurposeFilteringSceneIndex(inputSceneIndex, inputArgs));
}

HdsiPurposeFilteringSceneIndex::HdsiPurposeFilteringSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    HdContainerDataSourceHandle const &inputArgs)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    , _purposes(_GetAcceptedPurposes(inputArgs))
    , _excludedPurposesHint(_GetExcludedPurposesHint(inputArgs))
{
}

HdsiPurposeFilteringSceneIndex::
~HdsiPurposeFilteringSceneIndex() = default;

void
HdsiPurposeFilteringSceneIndex::SetPurposes(VtArray<TfToken> const &purposes)
{
    // No-op if the explicit value is already set to the same array.
    if (_purposes.has_value() && *_purposes == purposes) {
        return;
    }

    // Optimization: transitioning from hint mode to explicit purposes where
    // the new purposes still excludes every hint token.  In that case no prim
    // pruning state changes, so we skip the expensive full-scene traversal.
    if (!_purposes.has_value() && !_excludedPurposesHint.empty()) {
        bool anyHintIncluded = false;
        for (const TfToken &h : _excludedPurposesHint) {
            if (std::find(
                purposes.begin(), purposes.end(), h) != purposes.end()) {
                anyHintIncluded = true;
                break;
            }
        }
        if (!anyHintIncluded) {
            TF_DEBUG(HDSI_PURPOSE_FILTERING_SCENE_INDEX).Msg(
                "Transitioning from hint mode to explicit purposes. "
                "Don't need full scene traversal.\n");
            _purposes = purposes;
            return;
        }
    }

    const std::optional<VtArray<TfToken>> oldPurposes = _purposes;
    _purposes = purposes;

    if (!_IsObserved()) {
        return;
    }

    TRACE_FUNCTION();

    TF_DEBUG(HDSI_PURPOSE_FILTERING_SCENE_INDEX).Msg(
        "Starting full scene traversal to update pruning state.\n");

    // Step 1 (serial): HdSceneIndexPrimView is a tree walk; collect paths.
    SdfPathVector primPaths;
    for (const SdfPath &primPath :
             HdSceneIndexPrimView(_GetInputSceneIndex())) {
        primPaths.push_back(primPath);
    }
    if (primPaths.empty()) {
        return;
    }

    // Step 2 (parallel): Each slot i is written by exactly one thread.
    struct _Result { bool changed = false; TfToken primType; };
    // XXX Typically expect most entries to not change pruning state, so this
    // allocation is likely over-provisioned.
    std::vector<_Result> results(primPaths.size());

    WorkParallelForN(primPaths.size(), [&](size_t begin, size_t end) {
        for (size_t i = begin; i < end; ++i) {
            const HdSceneIndexPrim prim =
                _GetInputSceneIndex()->GetPrim(primPaths[i]);
            const std::optional<TfToken> purpose =
                _GetPurpose(prim.dataSource);
            const bool wasPruned = _ComputeShouldPrune(
                purpose, oldPurposes, _excludedPurposesHint);
            const bool isPruned = _ComputeShouldPrune(
                purpose, _purposes, _excludedPurposesHint);
            if (wasPruned != isPruned) {
                results[i].changed = true;
                results[i].primType = isPruned ? TfToken() : prim.primType;
            }
        }
    });

    // Step 3 (serial): Collect changed entries preserving traversal order.
    HdSceneIndexObserver::AddedPrimEntries addedEntries;
    for (size_t i = 0; i < primPaths.size(); ++i) {
        if (results[i].changed) {
            addedEntries.emplace_back(primPaths[i], results[i].primType);
        }
    }

    _SendPrimsAdded(addedEntries);
}

VtArray<TfToken> const &
HdsiPurposeFilteringSceneIndex::GetPurposes() const
{
    if (_purposes) {
        return *_purposes;
    }
    static const VtArray<TfToken> empty;
    return empty;
}

HdSceneIndexPrim
HdsiPurposeFilteringSceneIndex::GetPrim(const SdfPath &primPath) const
{
    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    // XXX Since this scene index is stateless, need to fetch the purpose data
    // source on each query to determine if the prim should be pruned.
    if (_ShouldPrune(prim.dataSource)) {
        // Only nuke the prim type.
        prim.primType = TfToken();

        // TF_DEBUG(HDSI_PURPOSE_FILTERING_SCENE_INDEX).Msg(
        //     "Pruning prim at path: %s\n", primPath.GetText());
    }
    return prim;
}

SdfPathVector
HdsiPurposeFilteringSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    // Scene topology is unchanged.
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdsiPurposeFilteringSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    // Fast path: no filtering is active.
    if (!_IsFilteringActive()) {
        TF_DEBUG(HDSI_PURPOSE_FILTERING_SCENE_INDEX).Msg(
            "No filtering active, sending all added notices.\n");

        _SendPrimsAdded(entries);
        return;
    }

    TRACE_FUNCTION();

    // Phase 1 (parallel): Query each added prim to determine if it should be
    // pruned. Because GetPrim() is expensive, we do this in parallel.
    std::vector<bool> pruned(entries.size(), false);
    std::atomic<bool> anythingPruned{false};
    WorkParallelForN(entries.size(), [&](size_t begin, size_t end) {
        for (size_t i = begin; i < end; ++i) {
            if (entries[i].primType.IsEmpty()) {
                continue;
            }
            const HdSceneIndexPrim prim =
                _GetInputSceneIndex()->GetPrim(entries[i].primPath);
            if (_ShouldPrune(prim.dataSource)) {
                pruned[i] = true;
                anythingPruned.store(true, std::memory_order_relaxed);
            }
        }
    });

    if (!anythingPruned.load(std::memory_order_relaxed)) {
        _SendPrimsAdded(entries);
        return;
    }

    // Phase 2 (serial): Build the filtered entry list.
    HdSceneIndexObserver::AddedPrimEntries filteredEntries = entries;
    size_t numPruned = 0;
    for (size_t i = 0; i < filteredEntries.size(); ++i) {
        if (pruned[i]) {
            filteredEntries[i].primType = TfToken();
            ++numPruned;
        }
    }

    TF_DEBUG(HDSI_PURPOSE_FILTERING_SCENE_INDEX).Msg(
        "Number of prims pruned during _PrimsAdded: %zu\n", numPruned);

    _SendPrimsAdded(filteredEntries);
}

void
HdsiPurposeFilteringSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

void
HdsiPurposeFilteringSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    if (!_IsObserved()) {
        return;
    }

    // Fast path: no filtering is active.
    if (!_IsFilteringActive()) {
        TF_DEBUG(HDSI_PURPOSE_FILTERING_SCENE_INDEX).Msg(
            "No filtering active, sending all dirtied notices.\n");

        _SendPrimsDirtied(entries);
        return;
    }

    TRACE_FUNCTION();
    // When the purpose locator is dirtied, a prim's pruning status may have
    // changed. Emit an added notice so downstream observers see the updated
    // (possibly empty) primType. All other dirty entries are passed through
    // as-is.
    // XXX Because we don't track the previous pruning state, we conservatively
    // emit an added notice each time the purpose locator is dirtied (this would
    // be the case with the universal locator set, for example), leading to
    // possibly redundant added notices for the same prim.
    // That can be detrimental to performance because it would likely be
    // treated as a resync.
    //
    const HdDataSourceLocator &purposeLocator =
        HdPurposeSchema::GetDefaultLocator();

    // Serial pass: cheap locator intersection check; record indices of entries
    // that touch the purpose locator. Everything else passes through as-is.
    std::vector<size_t> purposeIdx;
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].dirtyLocators.Intersects(purposeLocator)) {
            purposeIdx.push_back(i);
        }
    }

    if (purposeIdx.empty()) {
        TF_DEBUG(HDSI_PURPOSE_FILTERING_SCENE_INDEX).Msg(
            "No purpose-intersecting dirtied entries, sending all dirtied "
            "notices.\n");

        _SendPrimsDirtied(entries);
        return;
    }

    // Parallel pass: evaluate pruning only for purpose-intersecting entries.
    // GetPrim() is expensive, so we parallelize across the candidates.
    std::vector<TfToken> primTypes(purposeIdx.size());
    WorkParallelForN(purposeIdx.size(), [&](size_t begin, size_t end) {
        for (size_t j = begin; j < end; ++j) {
            const HdSceneIndexPrim prim =
                _GetInputSceneIndex()->GetPrim(entries[purposeIdx[j]].primPath);
            primTypes[j] =
                _ShouldPrune(prim.dataSource) ? TfToken() : prim.primType;
        }
    });

    // Serial collect: build output entry lists from the parallel results.
    HdSceneIndexObserver::AddedPrimEntries addedEntries;
    addedEntries.reserve(purposeIdx.size());
    for (size_t j = 0; j < purposeIdx.size(); ++j) {
        addedEntries.emplace_back(
            entries[purposeIdx[j]].primPath, primTypes[j]);
    }

    // Walk entries once more, skipping purposeIdx slots (which are in sorted
    // order) to collect the pass-through dirty entries.
    HdSceneIndexObserver::DirtiedPrimEntries passedEntries;
    passedEntries.reserve(entries.size() - purposeIdx.size());
    {
        size_t p = 0;
        for (size_t i = 0; i < entries.size(); ++i) {
            if (p < purposeIdx.size() && purposeIdx[p] == i) {
                ++p;
            } else {
                passedEntries.push_back(entries[i]);
            }
        }
    }

    TF_DEBUG(HDSI_PURPOSE_FILTERING_SCENE_INDEX).Msg(
        "Number of re-added prims during _PrimsDirtied: %zu",
        addedEntries.size());

    _SendPrimsAdded(addedEntries);
    _SendPrimsDirtied(passedEntries);
}

bool
HdsiPurposeFilteringSceneIndex::_IsFilteringActive() const
{
    return _purposes.has_value() ? !_purposes->empty()
                                 : !_excludedPurposesHint.empty();
}


bool
HdsiPurposeFilteringSceneIndex::_ShouldPrune(
    const HdContainerDataSourceHandle &dataSource) const
{
    return _ComputeShouldPrune(
        _GetPurpose(dataSource), _purposes, _excludedPurposesHint);
}

PXR_NAMESPACE_CLOSE_SCOPE
