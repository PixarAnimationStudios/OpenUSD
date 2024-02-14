//
// Copyright 2024 Pixar
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
//

#include "pxr/imaging/hd/collectionExpressionEvaluator.h"

#include "pxr/imaging/hd/collectionPredicateLibrary.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"

#include "pxr/usd/sdf/pathExpression.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// SdfPathExpressionEval::Match currently takes in a domain object and two 
// functors to provide the translation from path-to-object and vice versa.
// See comment below in HdCollectionExpressionEvaluator::Match.
// 
struct _PathToSceneIndexPrim
{
    const HdSceneIndexPrim operator() (const SdfPath &path) const
    {
        if (sceneIndex) {
            return sceneIndex->GetPrim(path);
        }
        return {};
    }

    const HdSceneIndexBaseRefPtr &sceneIndex;
};

// XXX A scene index prim cannot be queried for its path.
//     We'd need to traverse the scene index and test for equality to get the
//     path given a prim.
//     For now, just take the path on construction and return it.
//     Revisit this when adding support for incremental searcher.
//
struct _SceneIndexPrimToPath
{
    SdfPath operator() (const HdSceneIndexPrim &prim) const
    {
        TF_UNUSED(prim);
        return path;
    }

    SdfPath const &path;
};

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

    // It seems redundant to pass the domain object (scene index prim) in
    // addition to both functors. The SdfPathExpressionEval::Match API
    // doesn't provide an overload that takes just the path and path-to-object
    // functor as arguments.
    //
    const HdSceneIndexPrim prim = _sceneIndex->GetPrim(path);

    // XXX For a prim path that isn't in the scene index, we'll get an empty
    //     prim entry. The only way to determine if a prim exists at a path is
    //     to query GetChildPrimPaths with its parent path and check if it is
    //     indeed its child.
    //     While we could choose to return MakeVarying(false) for empty prim
    //     entries, that would come at the cost of additional evaluation.
    //
    //     Consider a scene "/world/sets/room/..." where descendants of room
    //     have non-empty prim entries.
    //     The expression "//room//" matches /world/sets/room/ and all its
    //     descendants. If we were to restrict evaluation to non-empty prim
    //     entries, then we'd have to evaluate the expression on each of the
    //     children of room instead of stopping the evaluation at
    //     /world/sets/room.
    //     
    return
        _eval.Match(
            prim,
            _SceneIndexPrimToPath {path},
            _PathToSceneIndexPrim {_sceneIndex});
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

    // Serial traversal for now. Couple of ways to improve it:
    // - Use a work queue to farm off subtree traversals.
    // - Add support for incremental search in the evaluator to make evaluation
    //   stateful over a subtree. However, this seems tricky if using the
    //   HdSceneIndexPrim as the domain object for the evaluator since obtaining
    //   its path isn't straightforward.
    // 
    const HdSceneIndexBaseRefPtr &si = GetSceneIndex();
    HdSceneIndexPrimView view(si, rootPath);

    for (auto it = view.begin(); it != view.end(); ++it) {
        const SdfPath &primPath = *it;

        const SdfPredicateFunctionResult r = Match(primPath);

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