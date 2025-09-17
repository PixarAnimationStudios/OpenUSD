//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_UTILS_H
#define PXR_IMAGING_HDSI_UTILS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdsi/api.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/token.h"
#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(HdSceneIndexBase);

class HdCollectionExpressionEvaluator;
class HdCollectionsSchema;
class SdfPathExpression;

/// --------------------------------------------------------------------------
/// General use collection utilities.
/// --------------------------------------------------------------------------

/// Extracts and compiles the membership expression of the collection
/// with the given \p collectionName, if it exists.
///
HDSI_API
void
HdsiUtilsCompileCollection(
    HdCollectionsSchema &collections,
    TfToken const& collectionName,
    HdSceneIndexBaseRefPtr const& sceneIndex,
    SdfPathExpression *expr,
    std::optional<HdCollectionExpressionEvaluator> *eval);

/// --------------------------------------------------------------------------
/// Utilities to evaluate membership expressions for collections with pruning
/// semantics.
/// --------------------------------------------------------------------------

/// Returns whether the prim at \p primPath is pruned by the suppied
/// evaluator.
///
HDSI_API
bool 
HdsiUtilsIsPruned(
    const SdfPath &primPath,
    const HdCollectionExpressionEvaluator &eval);

/// Prunes the given list of children using the supplied evaluator.
HDSI_API
void
HdsiUtilsRemovePrunedChildren(
    const SdfPath &parentPath,
    const HdCollectionExpressionEvaluator &eval,
    SdfPathVector *children);

/// ------------------------------------------------------------------------

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDSI_UTILS_H
