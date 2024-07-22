//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
#include "pxr/imaging/hdsi/primTypeAndPathPruningSceneIndex.h"

#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(HdsiPrimTypeAndPathPruningSceneIndexTokens,
                        HDSI_PRIM_TYPE_AND_PATH_PRUNING_SCENE_INDEX_TOKENS);

namespace
{

TfTokenVector _GetPrimTypes(HdContainerDataSourceHandle const &container)
{
    if (!container) {
        return {};
    }
    using DataSource = HdTypedSampledDataSource<TfTokenVector>;
    auto const ds =
        DataSource::Cast(
            container->Get(
                HdsiPrimTypeAndPathPruningSceneIndexTokens->primTypes));
    if (!ds) {
        return {};
    }
    return ds->GetTypedValue(0.0f);
}

}

/* static */
HdsiPrimTypeAndPathPruningSceneIndexRefPtr
HdsiPrimTypeAndPathPruningSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    HdContainerDataSourceHandle const &inputArgs)
{
    return TfCreateRefPtr(
        new HdsiPrimTypeAndPathPruningSceneIndex(
            inputSceneIndex, inputArgs));
}

bool
HdsiPrimTypeAndPathPruningSceneIndex::_PruneType(const TfToken &primType) const {
    for (const TfToken &type : _primTypes) {
        if (primType == type) {
            return true;
        }
    }
    return false;
}

void
HdsiPrimTypeAndPathPruningSceneIndex::SetPathPredicate(
    PathPredicate pathPredicate)
{
    TRACE_FUNCTION();

    const PathPredicate oldPathPredicate = std::move(_pathPredicate);
    // Set new predicate before we call _SendPrimsAdded which might
    // make client scene indices pull on this scene index.
    _pathPredicate = std::move(pathPredicate);

    if (!_IsObserved()) {
        return;
    }

    HdSceneIndexObserver::AddedPrimEntries addedEntries;

    for (const SdfPath &primPath: HdSceneIndexPrimView(_GetInputSceneIndex())) {
        // We assume that evaluating path predicate is fast compared to
        // calling GetPrim on the input scene.
        const bool oldValue = oldPathPredicate && oldPathPredicate(primPath);
        const bool newValue =   _pathPredicate &&   _pathPredicate(primPath);

        if (oldValue == newValue) {
            continue;
        }

        TfToken primType = _GetInputSceneIndex()->GetPrim(primPath).primType;
        if (!_PruneType(primType)) {
            continue;
        }

        if (newValue) {
            primType = TfToken();
        }

        addedEntries.emplace_back(primPath, primType);
    }
    if (!addedEntries.empty()) {
        _SendPrimsAdded(addedEntries);
    }
}

HdSceneIndexPrim
HdsiPrimTypeAndPathPruningSceneIndex::GetPrim(const SdfPath &primPath) const
{
    // We assume that only few prims are gonna be pruned.

    const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);
    if (!_pathPredicate) {
        return prim;
    }
    if (_PruneType(prim.primType) && _pathPredicate(primPath)) {
        return { TfToken(), nullptr };
    }
    return prim;
}

SdfPathVector
HdsiPrimTypeAndPathPruningSceneIndex::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdsiPrimTypeAndPathPruningSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    if (!_IsObserved()) {
        return;
    }

    if (!_pathPredicate) {
        _SendPrimsAdded(entries);
        return;
    }

    // Fast path: if there are no pruned prims, reuse the entry list.
    bool anythingToFilter = false;
    for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
        if (_PruneType(entry.primType) && _pathPredicate(entry.primPath)) {
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
        if (_PruneType(entry.primType) && _pathPredicate(entry.primPath)) {
            entry.primType = TfToken();
        }
    }
    _SendPrimsAdded(filteredEntries);
}

void
HdsiPrimTypeAndPathPruningSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    _SendPrimsRemoved(entries);
}

void
HdsiPrimTypeAndPathPruningSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    // XXX We could, potentially, filter out entries for prims
    // we have pruned.  For now, we pass through (potentially
    // unnecessary) dirty notification.
    _SendPrimsDirtied(entries);
}

HdsiPrimTypeAndPathPruningSceneIndex::HdsiPrimTypeAndPathPruningSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    HdContainerDataSourceHandle const &inputArgs)
    : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
    , _primTypes(_GetPrimTypes(inputArgs))
{
    if (_primTypes.empty()) {
        TF_CODING_ERROR(
            "Empty prim types given to HdsiPrimTypeAndPathPruningSceneIndex");
    }
}

HdsiPrimTypeAndPathPruningSceneIndex::
~HdsiPrimTypeAndPathPruningSceneIndex() = default;

PXR_NAMESPACE_CLOSE_SCOPE
