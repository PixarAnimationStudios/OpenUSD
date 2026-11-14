//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hd/collectionExpressionEvaluator.h"

#include "pxr/imaging/hd/collectionPredicateLibrary.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"

#include "pxr/usd/sdf/pathExpression.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Traverse the subtree at `rootPath` and add descendant prim paths to `result`.
void
_AddAllDescendants(
    const HdSceneIndexBaseRefPtr &si,
    const SdfPath &rootPath,
    SdfPathVector *result)
{
    HdSceneIndexPrimView view(si, rootPath);
    auto it = view.begin();
    ++it; // skip adding the rootPath iterator entry; we only care about
          // descendants.

    for (; it != view.end(); ++it) {
        const SdfPath &primPath = *it;
        result->push_back(primPath);
    }
}

} // anon

HdCollectionExpressionEvaluator::HdCollectionExpressionEvaluator(
    const HdSceneIndexBaseRefPtr &sceneIndex,
    const SdfPathExpression &expr)
    : HdCollectionExpressionEvaluator(
        sceneIndex, expr, HdGetCollectionPredicateLibrary())
{
}

HdCollectionExpressionEvaluator::HdCollectionExpressionEvaluator(
    const HdSceneIndexBaseRefPtr &sceneIndex,
    const SdfPathExpression &expr,
    const HdCollectionPredicateLibrary &predLib)
    : _sceneIndex(sceneIndex)
    , _eval(SdfMakePathExpressionEval(expr, predLib))
{
}

SdfPredicateFunctionResult
HdCollectionExpressionEvaluator::Match(
    const SdfPath &path) const
{
    if (IsEmpty()) {
        return SdfPredicateFunctionResult::MakeConstant(false);
    }

    // What we want below is simply:
    constexpr bool useSimpleImplementation = false;
    if (useSimpleImplementation) {
        if (_sceneIndex->GetPrim(path).IsDefined()) {
            return _eval.Match(
                path,
                [sceneIndex = get_pointer(_sceneIndex)](SdfPath const &path) {
                    return sceneIndex->GetPrim(path);
                });
        }
        return SdfPredicateFunctionResult::MakeConstant(false);
    }

    // But, that would involve querying the prim _prior_ to evaluation.
    // Scene index prim queries get expensive at scale, so avoid a query if
    // the path didn't match, or if we already queried the prim to handle
    // predicate evaluation.
    //
    std::optional<HdSceneIndexPrim> optPrim;
    auto pathToObjFn =
        [&optPrim, sceneIndex = get_pointer(_sceneIndex)](SdfPath const &path) {
            optPrim = sceneIndex->GetPrim(path);
            return *optPrim;
        };

    const auto result = _eval.Match(path, pathToObjFn);
    if (result == SdfPredicateFunctionResult::MakeConstant(false)) {
        return result;
    }
    if (optPrim) {
        // Prim was queried. Factor existence into the result.
        if (optPrim->IsDefined()) {
            return result;
        }
        return SdfPredicateFunctionResult::MakeConstant(false);
    }

    // Prim wasn't queried during evaluation.
    if (_sceneIndex->GetPrim(path).IsDefined()) {
        return result;
    }
    return SdfPredicateFunctionResult::MakeConstant(false);
}

void
HdCollectionExpressionEvaluator::PopulateAllMatches(
    const SdfPath &rootPath,
    SdfPathVector * const result) const
{
    constexpr MatchKind matchKind = MatchAll;
    PopulateMatches(rootPath, matchKind, result);
}

void
HdCollectionExpressionEvaluator::PopulateMatches(
    const SdfPath &rootPath,
    MatchKind matchKind,
    SdfPathVector * const result) const
{
    if (IsEmpty() || !result) {
        return;
    }

    HD_TRACE_FUNCTION();

    // Serial traversal for now. This could be improved by using a work queue
    // to farm off subtree traversals, using copies of the incremental searcher.
    //
    const HdSceneIndexBaseRefPtr &si = GetSceneIndex();

    // Note that si must be non-null due to the IsEmpty() check above.
    auto searcher = _eval.MakeIncrementalSearcher(
        [sceneIndex = get_pointer(si)](SdfPath const &path) {
            return sceneIndex->GetPrim(path);
        });

    HdSceneIndexPrimView view(si, rootPath);

    for (auto it = view.begin(); it != view.end(); ++it) {
        const SdfPath &primPath = *it;

        const SdfPredicateFunctionResult r = searcher.Next(primPath);

        const bool matches = r.GetValue();
        const bool constantOverDescendants = r.IsConstant();

        if (matches) {
            result->push_back(primPath);

            const bool addDescendantPrims =
                (constantOverDescendants && matchKind == MatchAll) ||
                (matchKind == ShallowestMatchesAndAllDescendants);
            
            if (addDescendantPrims) {
                _AddAllDescendants(si, primPath, result);
            }

            const bool skipDescendantTraversal =
                addDescendantPrims ||
                (matchKind == ShallowestMatches);

            if (skipDescendantTraversal) {
                it.SkipDescendants();
            }

        } else {
            // Result does not match at primPath ...
            if (constantOverDescendants) {
                // ... nor does it on any descendants.
                it.SkipDescendants();
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
