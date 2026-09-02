//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/instancingAwareCollectionExpressionEvaluator.h"
#include "pxr/imaging/hd/instanceProxyViewSceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"

#include "pxr/base/trace/trace.h"

#include <iterator> // std::inserter, std::make_move_iterator

PXR_NAMESPACE_OPEN_SCOPE

HdInstancingAwareCollectionExpressionEvaluator::HdInstancingAwareCollectionExpressionEvaluator(
    const HdInstanceProxyViewSceneIndexRefPtr &proxyViewSceneIndex,
    const SdfPathExpression &expr,
    const HdCollectionPredicateLibrary &predicateLib)
    : _inputEval( proxyViewSceneIndex
                ? proxyViewSceneIndex->GetInputScenes()[0]
                : HdSceneIndexBaseRefPtr(),
                expr,
                predicateLib)
    , _proxyViewEval(proxyViewSceneIndex, expr, predicateLib)
    , _proxyViewSi(proxyViewSceneIndex)
{
    if (!proxyViewSceneIndex) {
        TF_WARN("HdInstancingAwareCollectionExpressionEvaluator constructed "
                "with null HdInstanceProxyViewSceneIndex for expression '%s'. "
                "Evaluation will not be instancing-aware.",
                expr.GetText().c_str());
    }
}

SdfPredicateFunctionResult
HdInstancingAwareCollectionExpressionEvaluator::Match(const SdfPath &path) const
{
    return _inputEval.Match(path);
}

HdInstancingAwareCollectionExpressionEvaluator::MatchResult
HdInstancingAwareCollectionExpressionEvaluator::GetAllMatches(
    const SdfPath &rootPath,
    HdCollectionExpressionEvaluator::MatchKind matchKind,
    bool includeInstanceProxyMatches) const
{
    MatchResult result;
    AppendAllMatches(rootPath, matchKind, includeInstanceProxyMatches, &result);
    return result;
}

void
HdInstancingAwareCollectionExpressionEvaluator::AppendAllMatches(
    const SdfPath &rootPath,
    HdCollectionExpressionEvaluator::MatchKind matchKind,
    bool includeInstanceProxyMatches,
    MatchResult * const result) const
{
    if (!result) {
        return;
    }

    TRACE_FUNCTION();

    // Pass 1: standard traversal on the input scene (no proxy-view overhead).
    // XXX Should we skip /UsdNiPropagatedPrototypes altogether here?
    SdfPathVector primMatches;
    _inputEval.PopulateMatches(rootPath, matchKind, &primMatches);
    result->nonInstanceMatches.insert(
        std::make_move_iterator(primMatches.begin()),
        std::make_move_iterator(primMatches.end()));

    if (!_proxyViewSi) {
        // No proxy view — cannot remove outermost instance paths.
        // Warning already issued in constructor.
        return;
    }

    // Filter out outermost instance paths — they are classified in pass 2.
    // Both sets are sorted, so std::set_difference gives us a linear pass.
    const SdfPathSet &outermostInstances =
        _proxyViewSi->GetAllOutermostInstancePrimPaths();
    {
        SdfPathSet filtered;
        std::set_difference(
            result->nonInstanceMatches.begin(),
            result->nonInstanceMatches.end(),
            outermostInstances.begin(),
            outermostInstances.end(),
            std::inserter(filtered, filtered.end()));
        result->nonInstanceMatches = std::move(filtered);
    }

    // Pass 2: classify each outermost instance prim.
    for (const SdfPath &outerInst : outermostInstances) {
        const SdfPredicateFunctionResult r = _proxyViewEval.Match(outerInst);
        const bool matches = r.GetValue();
        const bool constantOverDescendants = r.IsConstant();
        if (constantOverDescendants) {
            if (matches) {
                result->fullyMatchedInstances.insert(outerInst);

                if (includeInstanceProxyMatches) {
                    // All proxy descendants match — traverse and add them.
                    _TraverseProxyDescendants(
                        outerInst, &result->matchedInstanceProxyPaths);
                }
            }
            // else: instance doesn't match, nor does any of its descendants.
            continue;
        }

        // Result varies over descendants.
        // XXX: If the instance matches, but none of its proxy descendants
        //      match, the instance is neither fully nor partially matched.
        //      What should the correct behavior be here?
        //
        bool allMatch = true, anyMatch = false;
        _CollectProxyMatches(outerInst, includeInstanceProxyMatches,
                             &allMatch, &anyMatch,
                             &(result->matchedInstanceProxyPaths));

        if (allMatch) {
            result->fullyMatchedInstances.insert(outerInst);
        } else if (anyMatch) {
            result->partiallyMatchedInstances.insert(outerInst);
        }
    }
}

void
HdInstancingAwareCollectionExpressionEvaluator::_CollectProxyMatches(
    const SdfPath &path,
    bool includeProxyPaths,
    bool *allMatch,
    bool *anyMatch,
    SdfPathSet *matchedProxyPaths) const
{
    // XXX There is a cost to invoking GetChildPrimPaths() on the proxy view..
    //     It may make sense to cache instance proxy hierarchies in the
    //     HdInstanceProxyViewSceneIndex to avoid repeated calls here.
    //
    for (const SdfPath &child : _proxyViewSi->GetChildPrimPaths(path)) {
        const SdfPredicateFunctionResult r = _proxyViewEval.Match(child);
        if (r.GetValue()) {
            *anyMatch = true;
            if (includeProxyPaths && matchedProxyPaths) {
                matchedProxyPaths->insert(child);
            }
            if (r.IsConstant()) {
                if (includeProxyPaths && matchedProxyPaths) {
                    _TraverseProxyDescendants(child, matchedProxyPaths);
                }
                continue;
            }
        } else {
            *allMatch = false;
            if (r.IsConstant()) {
                continue;
            }
        }

        _CollectProxyMatches(child, includeProxyPaths,
                                allMatch, anyMatch, matchedProxyPaths);
    }
}

void
HdInstancingAwareCollectionExpressionEvaluator::_TraverseProxyDescendants(
    const SdfPath &path,
    SdfPathSet *proxyPaths) const
{
    if (!proxyPaths) {
        return;
    }

    // XXX Same performance concern as _CollectProxyMatches().
    HdSceneIndexPrimView view(_proxyViewSi, path);
    // Note the ++it to skip the root path (it can be either an instance or
    // instance proxy; if the latter, we'd have added it in
    // _CollectProxyMatches() already).
    auto it = view.begin();
    ++it;
    for (; it != view.end(); ++it) {
        proxyPaths->insert(*it);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
