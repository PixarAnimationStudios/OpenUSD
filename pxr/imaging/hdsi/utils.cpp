//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.

#include "pxr/imaging/hdsi/utils.h"
#include "pxr/imaging/hd/collectionExpressionEvaluator.h"

PXR_NAMESPACE_OPEN_SCOPE

void
HdsiUtilsRemovePrunedChildren(
    const SdfPath &parentPath,
    const HdCollectionExpressionEvaluator &eval,
    SdfPathVector *children)
{
    if (!children) {
        TF_CODING_ERROR("Received null vector.");
        return;
    }
    if (children->empty()) {
        return;
    }

    // Evaluate the expression at parentPath.
    const SdfPredicateFunctionResult res = eval.Match(parentPath);
    const bool matchesParent = res.GetValue();
    const bool resultVariesOverDescendants =
        res.GetConstancy() ==
        SdfPredicateFunctionResult::MayVaryOverDescendants;

    // Four possibilities:
    //
    // # |   result  |   MayVaryOverDescendants | outcome
    // --|-----------|--------------------------|------------------------------
    // 1 |   False   |          False           | nothing to prune
    //   |           |                          |
    // 2 |   False   |          True            | traverse children to evaluate
    //   |           |                          | if they are pruned
    //   |           |                          |
    // 3 |    True   |          False           | all children are pruned
    //   |           |                          |
    // 4 |    True   |          True            | all children are pruned
    // ------------------------------------------------------------------------
    //
    // #4 is interesting. Even though the result may vary over descendants,
    // since the parent matches, all of its descendants are pruned regardless.
    //

    // #3 and #4.
    if (matchesParent) {
        children->clear();
        return;
    }

    // #1.
    if (!resultVariesOverDescendants) {
        return;
    }

    // #2.
    // Evaluate the expression at each child. Like #4, we only care about the
    // result at the child path and do not need to evaluate its descendants.
    children->erase(
        std::remove_if(
            children->begin(), children->end(),
            [&eval, &parentPath](const SdfPath &childPath) {
                return eval.Match(childPath);
            }),
        children->end());
}

PXR_NAMESPACE_CLOSE_SCOPE
