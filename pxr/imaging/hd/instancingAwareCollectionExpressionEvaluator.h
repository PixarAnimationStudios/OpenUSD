//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_INSTANCING_AWARE_COLLECTION_EXPRESSION_EVALUATOR_H
#define PXR_IMAGING_HD_INSTANCING_AWARE_COLLECTION_EXPRESSION_EVALUATOR_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"

#include "pxr/imaging/hd/collectionExpressionEvaluator.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdInstanceProxyViewSceneIndex;
class SdfPathExpression;

TF_DECLARE_REF_PTRS(HdInstanceProxyViewSceneIndex);

///
/// \class HdInstancingAwareCollectionExpressionEvaluator
///
/// A wrapper around HdCollectionExpressionEvaluator that uses the "unrolled"
/// instance proxy view of the scene to provide instancing-aware evaluation of
/// collection path expressions.
/// Outermost instance prims (that correspond to leaf USD instance prims that
/// aren't under a prototype) are classified as fully or partially matched, and
/// matched instance proxy prims can be accumulated into the result.
///
/// This evaluator takes a \p HdInstanceProxyViewSceneIndexRefPtr (and not any
/// \p HdSceneIndexBaseRefPtr) as its c'tor argument to facilitate a two-pass
/// evaluation of the expression in \c AppendAllMatches().
///
///
class HdInstancingAwareCollectionExpressionEvaluator
{
public:
    /// Result type for AppendAllMatches.
    struct MatchResult {
        /// Paths of prims that matched the expression excluding outermost
        /// instance prim paths.
        /// XXX This currently includes prims under /UsdNiPropagatedPrototypes.
        ///     Should we filter those out?
        SdfPathSet nonInstanceMatches;

        /// Outermost instance prim paths where ALL instance proxy descendants
        /// match the expression. This includes the constant-true case
        /// (Match() returned true with IsConstant()=true) and the varying case
        /// where every proxy prim is matched.
        /// XXX If the outermost instance alone is matched, it is currently
        ///     not classified as fully/partially matched. Should it be the
        ///     former? (i.e. /Path/To/OuterInstance// is fully matched, but
        ///     /Path/To/OuterInstance is not)
        SdfPathSet fullyMatchedInstances;

        /// Outermost instance prim paths where SOME but not all instance proxy
        /// descendants match the expression.
        /// It does not matter if the outermost instance itself matches or not.
        SdfPathSet partiallyMatchedInstances;

        /// Instance proxy prim paths that matched the expression. Populated
        /// only when \p includeInstanceProxyMatches=true is passed to
        /// \c AppendAllMatches() or \c GetAllMatches().
        SdfPathSet matchedInstanceProxyPaths;
    };

    /// Default constructor. Constructs an empty evaluator.
    HdInstancingAwareCollectionExpressionEvaluator() = default;

    /// Constructs an evaluator for \p expr evaluated against the input scene of
    /// \p proxyViewSceneIndex using \p predicateLib.
    /// \p proxyViewSceneIndex is held as a non-owning TfWeakPtr and provides
    /// instance proxy traversal for pass 2 of AppendAllMatches().
    HD_API
    HdInstancingAwareCollectionExpressionEvaluator(
        const HdInstanceProxyViewSceneIndexRefPtr &proxyViewSceneIndex,
        const SdfPathExpression &expr,
        const HdCollectionPredicateLibrary &predicateLib);

    /// Returns true if the evaluator that uses the proxy view's input scene
    /// has no valid scene index or expression.
    /// XXX Should this include the proxy view evaluator as well?
    bool IsEmpty() const { return _inputEval.IsEmpty(); }

    /// Evaluates the expression at \p path using the evaluator backed by
    /// the proxy view's input scene.
    ///
    /// The rationale is that clients shouldn't have to concern themselves with
    /// instance proxy prim paths that don't exist in the input scene.
    /// The instance proxy view is only needed when evaluating an expression
    /// over all prims in the scene to drive invalidation.
    HD_API
    SdfPredicateFunctionResult Match(const SdfPath &path) const;

    /// Convenience wrapper that calls AppendAllMatches() on a
    /// default-constructed MatchResult and returns it.
    HD_API
    MatchResult GetAllMatches(
        const SdfPath &rootPath,
        HdCollectionExpressionEvaluator::MatchKind matchKind,
        bool includeInstanceProxyMatches = false) const;

    /// Accumulates two-pass match results into \p result.
    ///
    ///   1. Standard scene traversal via the HdCollectionExpressionEvaluator
    ///      that uses the proxy view's input scene. Outermost instance paths
    ///      are filtered out and classified in pass 2.
    ///
    ///   2. For each outermost instance prim, Match() is called using an
    ///      HdCollectionExpressionEvaluator that uses the proxy view scene
    ///      index.
    ///      If the instance prim matches and is constant over descendants,
    ///      the instance is classified as "fully matched" immediately.
    ///      Otherwise, instance proxy descendants are traversed to determine
    ///      whether the instance is fully matched (all proxy prims match) or
    ///      partially matched (some but not all proxy prims match).
    ///
    /// When \p includeInstanceProxyMatches=true, all proxy paths are traversed
    /// and those that match are accumulated into
    /// result->matchedInstanceProxyPaths.
    ///
    HD_API
    void AppendAllMatches(
        const SdfPath &rootPath,
        HdCollectionExpressionEvaluator::MatchKind matchKind,
        bool includeInstanceProxyMatches,
        MatchResult * const result) const;

private:
    /// Recursively traverses proxy view children of \p path, setting allMatch
    /// and anyMatch. When \p includeProxyPaths is true, matched proxy paths are
    /// accumulated into \p matchedProxyPaths.
    void _CollectProxyMatches(
        const SdfPath &path,
        bool includeProxyPaths,
        bool *allMatch,
        bool *anyMatch,
        SdfPathSet *matchedProxyPaths) const;

    /// Recursively traverses all proxy view descendants of \p path without
    /// evaluating Match(). Used when the match result is already known to be
    /// constant-true over all descendants (i.e. the outer instance is fully
    /// matched).
    void _TraverseProxyDescendants(
        const SdfPath &path,
        SdfPathSet *proxyPaths) const;

private:
    // Data.

    // Evaluator backed by the proxy view's input scene.
    HdCollectionExpressionEvaluator _inputEval;
    // Evaluator backed by the proxy view scene.
    HdCollectionExpressionEvaluator _proxyViewEval;

    // Non-owning handle; lifetime is managed by the owning scene index.
    TfWeakPtr<HdInstanceProxyViewSceneIndex> _proxyViewSi;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_INSTANCING_AWARE_COLLECTION_EXPRESSION_EVALUATOR_H
