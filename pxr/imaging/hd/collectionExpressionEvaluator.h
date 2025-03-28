//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_IMAGING_HD_COLLECTION_EXPRESSION_EVALUATOR_H
#define PXR_IMAGING_HD_COLLECTION_EXPRESSION_EVALUATOR_H

#include "pxr/pxr.h"

#include "pxr/imaging/hd/api.h"

#include "pxr/base/tf/declarePtrs.h"

#include "pxr/usd/sdf/pathExpressionEval.h"
#include "pxr/usd/sdf/predicateLibrary.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneIndexBase;
struct HdSceneIndexPrim;
class SdfPathExpression;

using SdfPathVector = std::vector<class SdfPath>;

TF_DECLARE_WEAK_AND_REF_PTRS(HdSceneIndexBase);

using HdCollectionPredicateLibrary =
    SdfPredicateLibrary<const HdSceneIndexPrim &>;

///
/// \class HdCollectionExpressionEvaluator
///
/// Evaluates SdfPathExpressions with prims from a given HdSceneIndex.
///
class HdCollectionExpressionEvaluator
{
public:
    /// Default c'tor. Constructs an empty evaluator.
    HdCollectionExpressionEvaluator() = default;

    /// Constructs an evaluator that evaluates \p expr on prims from
    /// \p sceneIndex using the predicates in HdGetCollectionPredicateLibrary().
    ///
    HD_API
    HdCollectionExpressionEvaluator(
        const HdSceneIndexBaseRefPtr &sceneIndex,
        const SdfPathExpression &expr);

    /// Constructs an evaluator that evaluates \p expr on prims from
    /// \p sceneIndex using the predicates in \p predicateLib.
    ///
    HD_API
    HdCollectionExpressionEvaluator(
        const HdSceneIndexBaseRefPtr &sceneIndex,
        const SdfPathExpression &expr,
        const HdCollectionPredicateLibrary &predicateLib);

    /// Returns true if the evaluator has an invalid scene index or an empty
    /// underlying SdfPathExpressionEval object.
    bool IsEmpty() const {
        return !_sceneIndex || _eval.IsEmpty();
    }

    /// Returns the scene index provided during construction, or nullptr if
    /// it was default constructed.
    HdSceneIndexBaseRefPtr
    GetSceneIndex() const {
        return _sceneIndex;
    }

    /// Returns the result of evaluating the expression (provided on
    /// construction) against the scene index prim at \p path.
    ///
    HD_API
    SdfPredicateFunctionResult
    Match(const SdfPath &path) const;

    /// Updates \p result with the paths of all prims in the
    /// subtree at \p rootPath (including \p rootPath) that match the 
    /// expression (provided on construction).
    ///
    /// An empty evaluator would leave \p result unaffected.
    ///
    /// \note: \p result is guaranteed to have unique paths because a scene
    ///        index prim is traversed at most once.
    ///
    HD_API
    void
    PopulateAllMatches(
        const SdfPath &rootPath,
        SdfPathVector * const result) const;
    
    /// \enum MatchKind
    ///
    /// Option to configure the paths returned by PopulateMatches.
    ///
    /// \li MatchAll: Return the paths of all prims that match the expression.
    ///
    /// \li ShallowestMatches: Return the paths of just the top level
    ///     prims that match, in a level-order or BFS sense.
    ///
    /// \li ShallowestMatchesAndAllDescendants: Returns the paths of the top
    ///     level prims that match the expression, as well as all their 
    ///     descendants.
    ///
    enum MatchKind {
        MatchAll,
        ShallowestMatches,
        ShallowestMatchesAndAllDescendants
    };

    /// Utility that uses \p matchKind to configure the paths returned by
    /// \p result when evaluating the expression for the subtree at \p rootPath
    /// (including \p rootPath).
    ///
    /// If \p matchKind is MatchAll, the result is identical to that returned
    /// by PopulateAllMatches.
    ///
    /// Example:
    /// Consider a scene index with prims:
    /// {"/a", "/a/foobar", "/a/foobar/b", "/a/foobar/bar", "/a/foobar/baz"}
    ///    
    /// PopulateMatches would return different results for the expression
    /// "/a/*bar" depending on \p matchKind as follows:
    ///
    /// MatchAll                          : {"/a/foobar", "/a/foobar/bar"}
    ///
    /// ShallowestMatches                 : {"/a/foobar"}
    ///
    /// ShallowestMatchesAndAllDescendants: {"/a/foobar", "/a/foobar/b",
    ///                                      "/a/foobar/bar", "/a/foobar/baz"}
    ///
    /// \note: \p result is guaranteed to have unique paths because a scene
    ///        index prim is traversed at most once.
    ///
    HD_API
    void
    PopulateMatches(
        const SdfPath &rootPath,
        MatchKind matchKind,
        SdfPathVector * const result) const;

private:
    HdSceneIndexBaseRefPtr _sceneIndex;

    using _PrimEvaluator = SdfPathExpressionEval<const HdSceneIndexPrim &>;
    _PrimEvaluator _eval;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_COLLECTION_EXPRESSION_EVALUATOR_H