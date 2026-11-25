//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdsi/expressionEvaluatingSceneIndexBase.h"

#include "pxr/imaging/hd/instanceProxyViewSceneIndex.h"

#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace
{
// This is a fallback scene index that we use in case an invalid sceneIndex is
// passed in to the c'tor below.
class _NoOpSceneIndex final : public HdSceneIndexBase
{
public:
    static HdSceneIndexBaseRefPtr New()
    {
        return TfCreateRefPtr(new _NoOpSceneIndex());
    }

    HdSceneIndexPrim GetPrim(const SdfPath& primPath) const override final
    {
        return { TfToken(), nullptr };
    }

    SdfPathVector
    GetChildPrimPaths(const SdfPath& priMPath) const override final
    {
        return {};
    }
};

} // anon

HdsiExpressionEvaluatingSceneIndexBase::HdsiExpressionEvaluatingSceneIndexBase(
    const HdSceneIndexBaseRefPtr &inputSceneIndex,
    const HdCollectionPredicateLibrary &predicateLibrary)
  : _inputSceneIndex(inputSceneIndex ? inputSceneIndex : _NoOpSceneIndex::New())
  , _predicateLibrary(predicateLibrary)
  , _proxyViewSceneIndex(HdInstanceProxyViewSceneIndex::New(_inputSceneIndex))
  , _observer(this)
{
    if (inputSceneIndex) {
        inputSceneIndex->AddObserver(HdSceneIndexObserverPtr(&_observer));
    } else {
        TF_CODING_ERROR("Invalid input sceneIndex.");
    }
}

// ---------------------------------------------------------------------------
// HdFilteringSceneIndexBase overrides
// ---------------------------------------------------------------------------

std::vector<HdSceneIndexBaseRefPtr>
HdsiExpressionEvaluatingSceneIndexBase::GetInputScenes() const
{
    return {_inputSceneIndex};
}

// ---------------------------------------------------------------------------
// HdEncapsulatingSceneIndexBase overrides
// ---------------------------------------------------------------------------

std::vector<HdSceneIndexBaseRefPtr>
HdsiExpressionEvaluatingSceneIndexBase::GetEncapsulatedScenes() const
{
    return {_proxyViewSceneIndex};
}

// ---------------------------------------------------------------------------
// HdSceneIndexBase overrides
// ---------------------------------------------------------------------------

HdSceneIndexPrim
HdsiExpressionEvaluatingSceneIndexBase::GetPrim(const SdfPath &primPath) const
{
    if (_IsPruned(primPath)) {
        return HdSceneIndexPrim();
    }
    HdSceneIndexPrim prim = _inputSceneIndex->GetPrim(primPath);
    _ModifyPrim(primPath, &prim);

    return prim;
}

SdfPathVector
HdsiExpressionEvaluatingSceneIndexBase::GetChildPrimPaths(
    const SdfPath &primPath) const
{
    if (_IsPruned(primPath)) {
        return {};
    }
    SdfPathVector children = _inputSceneIndex->GetChildPrimPaths(primPath);
    _FilterChildPrimPaths(primPath, &children);
    return children;
}

// ---------------------------------------------------------------------------
// Default implementations for virtual hooks
// ---------------------------------------------------------------------------

void
HdsiExpressionEvaluatingSceneIndexBase::_ModifyPrim(
    const SdfPath & /*primPath*/, HdSceneIndexPrim *prim) const
{
}

void
HdsiExpressionEvaluatingSceneIndexBase::_FilterChildPrimPaths(
    const SdfPath & /*primPath*/, SdfPathVector *children) const
{
}

bool
HdsiExpressionEvaluatingSceneIndexBase::_IsPruned(
    const SdfPath & /*primPath*/) const
{
    return false;
}

HdsiExpressionEvaluatingSceneIndexBase::NoticeProcessingResult
HdsiExpressionEvaluatingSceneIndexBase::_PrimsRenamed(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RenamedPrimEntries &inputEntries)
{
    HdSceneIndexObserver::RemovedPrimEntries removed;
    HdSceneIndexObserver::AddedPrimEntries   added;
    HdSceneIndexObserver::ConvertPrimsRenamedToRemovedAndAdded(
        sender, inputEntries, &removed, &added);

    const auto removedResult = _PrimsRemoved(sender, removed);
    const auto addedResult = _PrimsAdded(sender, added);

    auto concatResults = [](
        const NoticeProcessingResult &a, const NoticeProcessingResult &b) {
        NoticeProcessingResult result(false);

        if (a.addedEntries && !a.addedEntries->empty()) {
            result.addedEntries = *a.addedEntries;
        }
        if (b.addedEntries && !b.addedEntries->empty()) {
            if (!result.addedEntries) {
                result.addedEntries = *b.addedEntries;
            } else {
                result.addedEntries->insert(
                    result.addedEntries->end(),
                    b.addedEntries->begin(),
                    b.addedEntries->end());
            }
        }
        if (a.removedEntries && !a.removedEntries->empty()) {
            result.removedEntries = *a.removedEntries;
        }
        if (b.removedEntries && !b.removedEntries->empty()) {
            if (!result.removedEntries) {
                result.removedEntries = *b.removedEntries;
            } else {
                result.removedEntries->insert(
                    result.removedEntries->end(),
                    b.removedEntries->begin(),
                    b.removedEntries->end());
            }
        }
        if (a.dirtiedEntries && !a.dirtiedEntries->empty()) {
            result.dirtiedEntries = *a.dirtiedEntries;
        }
        if (b.dirtiedEntries && !b.dirtiedEntries->empty()) {
            if (!result.dirtiedEntries) {
                result.dirtiedEntries = *b.dirtiedEntries;
            } else {
                result.dirtiedEntries->insert(
                    result.dirtiedEntries->end(),
                    b.dirtiedEntries->begin(),
                    b.dirtiedEntries->end());
            }
        }
        return result;
    };

    return concatResults(removedResult, addedResult);
}

// ---------------------------------------------------------------------------
// Protected API for derived classes
// ---------------------------------------------------------------------------

HdInstancingAwareCollectionExpressionEvaluator
HdsiExpressionEvaluatingSceneIndexBase::_MakeEvaluator(
    const SdfPathExpression &pathExpr) const
{
    return HdInstancingAwareCollectionExpressionEvaluator(
        _proxyViewSceneIndex, pathExpr, _predicateLibrary);
}

SdfPathVector
HdsiExpressionEvaluatingSceneIndexBase::GetInstancerPrimPaths(
    const SdfPathSet &outermostInstancePaths) const
{
    SdfPathVector result;
    result.reserve(outermostInstancePaths.size());
    for (const SdfPath &outermostInstancePath : outermostInstancePaths) {
        std::optional<SdfPath> instancerPath =
            _proxyViewSceneIndex->GetInstancerPrimPath(outermostInstancePath);
        if (instancerPath) {
            result.push_back(*instancerPath);
        } else {
            result.push_back(SdfPath());
        }
    }
    return result;
}

const HdSceneIndexBaseRefPtr &
HdsiExpressionEvaluatingSceneIndexBase::_GetInputSceneIndex() const
{
    return _inputSceneIndex;
}

// ---------------------------------------------------------------------------
// _Observer

void
HdsiExpressionEvaluatingSceneIndexBase::_Observer::PrimsAdded(
    const HdSceneIndexBase &sender,
    const AddedPrimEntries &entries)
{
    const auto result = _owner->_PrimsAdded(sender, entries);

    // Check after the hook so that derived classes always have a chance
    // to process incoming notices.
    if (!_owner->_IsObserved()) {
        return;
    }

    // Forward notices in the result in the following order:
    // Added, Removed, Dirtied.
    const bool hasAddedEntries =
        result.addedEntries && !result.addedEntries->empty();

    if (result.forwardInputNotices && hasAddedEntries) {
        auto combinedEntries = entries;
        combinedEntries.insert(
            combinedEntries.end(),
            result.addedEntries->begin(),
            result.addedEntries->end());
        _owner->_SendPrimsAdded(combinedEntries);
    } else if (result.forwardInputNotices) {
        _owner->_SendPrimsAdded(entries);
    } else if (hasAddedEntries) {
        _owner->_SendPrimsAdded(*result.addedEntries);
    }

    if (result.removedEntries) {
        _owner->_SendPrimsRemoved(*result.removedEntries);
    }
    if (result.dirtiedEntries) {
        _owner->_SendPrimsDirtied(*result.dirtiedEntries);
    }
}

void
HdsiExpressionEvaluatingSceneIndexBase::_Observer::PrimsRemoved(
    const HdSceneIndexBase &sender,
    const RemovedPrimEntries &entries)
{
    const auto result = _owner->_PrimsRemoved(sender, entries);

    // Check after the hook so that derived classes always have a chance
    // to process incoming notices.
    if (!_owner->_IsObserved()) {
        return;
    }

    // Forward notices in the result in the following order:
    // Added, Removed, Dirtied.
    if (result.addedEntries) {
        _owner->_SendPrimsAdded(*result.addedEntries);
    }

    const bool hasRemovedEntries =
        result.removedEntries && !result.removedEntries->empty();

    if (result.forwardInputNotices && hasRemovedEntries) {
        auto combinedEntries = entries;
        combinedEntries.insert(
            combinedEntries.end(),
            result.removedEntries->begin(),
            result.removedEntries->end());
        _owner->_SendPrimsRemoved(combinedEntries);
    } else if (result.forwardInputNotices) {
        _owner->_SendPrimsRemoved(entries);
    } else if (hasRemovedEntries) {
        _owner->_SendPrimsRemoved(*result.removedEntries);
    }

    if (result.dirtiedEntries) {
        _owner->_SendPrimsDirtied(*result.dirtiedEntries);
    }
}

void
HdsiExpressionEvaluatingSceneIndexBase::_Observer::PrimsDirtied(
    const HdSceneIndexBase &sender,
    const DirtiedPrimEntries &entries)
{
    const auto result = _owner->_PrimsDirtied(sender, entries);

    // Check after the hook so that derived classes always have a chance
    // to process incoming notices.
    if (!_owner->_IsObserved()) {
        return;
    }

    // Forward notices in the result in the following order:
    // Added, Removed, Dirtied.
    if (result.addedEntries) {
        _owner->_SendPrimsAdded(*result.addedEntries);
    }
    if (result.removedEntries) {
        _owner->_SendPrimsRemoved(*result.removedEntries);
    }
    const bool hasDirtiedEntries =
        result.dirtiedEntries && !result.dirtiedEntries->empty();
    if (result.forwardInputNotices && hasDirtiedEntries) {
        auto combinedEntries = entries;
        combinedEntries.insert(
            combinedEntries.end(),
            result.dirtiedEntries->begin(),
            result.dirtiedEntries->end());
        _owner->_SendPrimsDirtied(combinedEntries);
    } else if (result.forwardInputNotices) {
        _owner->_SendPrimsDirtied(entries);
    } else if (hasDirtiedEntries) {
        _owner->_SendPrimsDirtied(*result.dirtiedEntries);
    }
}

void
HdsiExpressionEvaluatingSceneIndexBase::_Observer::PrimsRenamed(
    const HdSceneIndexBase &sender,
    const RenamedPrimEntries &entries)
{
    const auto result = _owner->_PrimsRenamed(sender, entries);

    // Check after the hook so that derived classes always have a chance
    // to process incoming notices.
    if (!_owner->_IsObserved()) {
        return;
    }

    if (result.addedEntries) {
        _owner->_SendPrimsAdded(*result.addedEntries);
    }
    if (result.removedEntries) {
        _owner->_SendPrimsRemoved(*result.removedEntries);
    }
    if (result.dirtiedEntries) {
        _owner->_SendPrimsDirtied(*result.dirtiedEntries);
    }
    if (result.forwardInputNotices) {
        _owner->_SendPrimsRenamed(entries);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
