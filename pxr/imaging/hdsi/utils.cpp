//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hdsi/utils.h"

#include "pxr/imaging/hd/collectionExpressionEvaluator.h"
#include "pxr/imaging/hd/collectionsSchema.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/usd/sdf/pathExpression.h"
#include "pxr/usd/sdf/predicateLibrary.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

/*

Pruning via path expressions has some subtlety that deserves explanation.

1. If the path expression is built from patterns that end with '//', then
descendant matching is already built into the expression evaluation. For
example, the expression '/World/Foo// + /World/Bar/Baz//' would prune
</World/Foo> and all its descendants, as well as </World/Bar/Baz> and all its   
descendants.

2. However, if the path expression is built from patterns that do not end with
'//', descendants of matched prims are still pruned.
So the expression '/World/Foo + /World/Bar/Baz' would prune
</World/Foo> and all its descendants, as well as </World/Bar/Baz> and all its   
descendants, even though </World/Foo/Child> and </World/Bar/Baz/Child> do not
explicitly match the expression.

3. Now, consider an expression that attempts to exclude some descendants, like
'/World/Foo - /World/Foo/Child'.
As in the above scenario, </World/Foo/Child> does not match the expression.
It is being explicitly excluded. Yet, because its parent </World/Foo> is
matched, we choose to prune </World/Foo/Child> as well.
This behavior is intentional to keep the behavior consistent with (2) and also
for performance reasons. Pruning is often used to remove entire branches of the
namespace.
To exclude some descendants of a subtree, the recommendation is to use
inclusion operators ('+') to explicitly specify the sibling trees to be pruned.
E.g. '/World/Foo/A// + /World/Foo/B//' ensures that </World/Foo/Child> is not
pruned.

4. Finally, consider an expression that has a predicate, like
'/World/Foo//{hdPurpose:"guide"}'. We want to prune all guide prims at/under
</World/Foo>.
If a prim path matches the path pattern '/World/Foo//', the predicate is then
evaluated. This requires querying the scene index for the prim at that path.
Because scene index queries can be expensive, we want to be judicious about it.

And we want to be consistent with the ancestral pruning behavior above, which
requires us to evaluate the predicate not just at the prim path itself, but also
at its ancestor paths. 

For example, consider the prim at </World/Foo/A/B/C>. If we were to
evaluate from the absolute root path down to the prim path, we would evaluate
the predicate at each of the path matches: </World/Foo>, </World/Foo/A>,
</World/Foo/A/B>, and </World/Foo/A/B/C>.
If only </World/Foo/A/B/C> is a guide (reasonable expectation since guides are
usually leaf prims), we can instead evaluate from the prim path upwards. This
can reduce the number of scene index queries in deep hierarchies when the leaf
prim matches. Note that for non-matches, we still need to evaluate all the way 
up to the root, which is unfortunate, but necessary for correctness.

Evaluating from the prim path upwards loses out on short-circuiting
opportunities in the absence of predicates wherein the result of an ancestor
path match is constant over descendants.

Future work:
Given these considerations and to aid performance, it may help to "sanitize"
the pruning path expression by
a) removing any exclusion operators (i.e. '-' and '~') since they have no effect
   on pruning, and
b) converting patterns that do not end with '//' to ones that do

This ensures that descendant matching is built into the expression evaluation,
and it would thus suffice to evaluate only at the prim path itself.

*/

// Implements the ancestal pruning semantics described in (2) above.
SdfPredicateFunctionResult
_GetPruneMatchResult(
    const SdfPath &primPath,
    const HdCollectionExpressionEvaluator &eval)
{
    TRACE_FUNCTION();

    // For pruning collections, an ancestral match counts.
    //
    const SdfPathVector prefixes = primPath.GetPrefixes();
    for (const SdfPath &path : prefixes) {
        const auto result = eval.Match(path);

        // Short circuit when possible:
        // 1. Path matches.
        if (result) {
            return result;
        }

        // 2. Path doesn't match, nor does any of its descendants.
        //    Note that expressions with predicates currently always return
        //    varying over descendants.
        //    (because e.g. /A being a guide doesn't imply /A/B is a guide)
        if (result.IsConstant()) {
            return result;
        }
    }
    
    return SdfPredicateFunctionResult(false);
}

} // anon

void
HdsiUtilsCompileCollection(
    HdCollectionsSchema &collections,
    TfToken const& collectionName,
    HdSceneIndexBaseRefPtr const& sceneIndex,
    SdfPathExpression *expr,
    std::optional<HdCollectionExpressionEvaluator> *eval)
{
    if (HdCollectionSchema collection =
        collections.GetCollection(collectionName)) {
        if (HdPathExpressionDataSourceHandle pathExprDs =
            collection.GetMembershipExpression()) {
            *expr = pathExprDs->GetTypedValue(0.0);
            if (!expr->IsEmpty()) {
                *eval = HdCollectionExpressionEvaluator(sceneIndex, *expr);
            }
        }
    }
}

bool
HdsiUtilsIsPruned(
    const SdfPath &primPath,
    const HdCollectionExpressionEvaluator &eval)
{
    if (eval.IsEmpty() || primPath.IsEmpty()) {
        return false;
    }

    return _GetPruneMatchResult(primPath, eval);
}

void
HdsiUtilsRemovePrunedChildren(
    const SdfPath &parentPath,
    const HdCollectionExpressionEvaluator &eval,
    SdfPathVector *children)
{
    if (eval.IsEmpty()) {
        return;
    }
    if (!children) {
        TF_CODING_ERROR("Received null vector.");
        return;
    }
    if (children->empty()) {
        return;
    }

    const auto result = _GetPruneMatchResult(parentPath, eval);
    if (result) {
        // If the parent is pruned, all its children are also pruned.
        children->clear();
        return;
    }

    // Parent isn't pruned. We have two possibilities:
    // 1. Result is constant over descendants, meaning that none of the children
    //    are pruned.
    // 2. Result varies over descendants. We need to evaluate the expression at
    //    each child.

    // #1.
    if (result.IsConstant()) {
        return;
    }

    // #2.
    // We only care about the result at the child path and do not need to 
    // evaluate its descendants.
    //
    children->erase(
        std::remove_if(
            children->begin(), children->end(),
            [&eval](const SdfPath &childPath) {
                return eval.Match(childPath);
            }),
        children->end());
}

PXR_NAMESPACE_CLOSE_SCOPE
