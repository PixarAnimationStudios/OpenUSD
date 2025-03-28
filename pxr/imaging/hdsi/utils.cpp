//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hdsi/utils.h"

#include "pxr/imaging/hd/collectionExpressionEvaluator.h"
#include "pxr/usd/sdf/predicateLibrary.h"
#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// For pruning collections that use a path expression without a trailing '//',
// an ancestral match counts.
// e.g. The path /World/Foo/Bar should be matched by the expression
//      /World/Foo (or //Foo) because pruning /World/Foo also prunes all of its
//      descendants.
//
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
        if (result.IsConstant()) {
            return result;
        }
    }
    
    return SdfPredicateFunctionResult(false);
}

} // anon

bool
HdsiUtilsIsPruned(
    const SdfPath &primPath,
    const HdCollectionExpressionEvaluator &eval)
{
    if (eval.IsEmpty()) {
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
