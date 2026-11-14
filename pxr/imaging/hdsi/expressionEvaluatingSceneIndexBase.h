//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_EXPRESSION_EVALUATING_SCENE_INDEX_BASE_H
#define PXR_IMAGING_HDSI_EXPRESSION_EVALUATING_SCENE_INDEX_BASE_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdsi/api.h"

#include "pxr/imaging/hd/instancingAwareCollectionExpressionEvaluator.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"
#include "pxr/imaging/hd/sceneIndexObserver.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/pathExpression.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdInstanceProxyViewSceneIndex;

TF_DECLARE_REF_PTRS(HdsiExpressionEvaluatingSceneIndexBase);
TF_DECLARE_REF_PTRS(HdInstanceProxyViewSceneIndex);

// Alias that's also defined in hd/collectionPredicateLibrary.h
using HdCollectionPredicateLibrary =
    SdfPredicateLibrary<const HdSceneIndexPrim &>;

///
/// \class HdsiExpressionEvaluatingSceneIndexBase
///
/// An abstract base class for scene indices that evaluate path expressions
/// against a scene.
///
/// Derived classes implement the _OnPrimsAdded, _OnPrimsRemoved, and
/// _OnPrimsDirtied hooks to react to scene changes. Each hook receives the
/// incoming entries and returns a NoticeProcessingResult that bundles
/// optional added, removed, and dirtied entries, and indicates
/// whether to forward the incoming notices (possibly saving a copy) as-is to
/// downstream observers.
///
/// GetPrim and GetChildPrimPaths are final. Derived classes customize them
/// by overriding _ModifyPrim and _FilterChildPrimPaths respectively.
/// Because pruning is a common use case for scene indices that evaluate path
/// expressions, derived classes can also override _IsPruned to bail early from
/// GetPrim and GetChildPrimPaths for pruned prims.
///
class HdsiExpressionEvaluatingSceneIndexBase
    : public HdFilteringSceneIndexBase, HdEncapsulatingSceneIndexBase
{
public:
    /// Returns {_inputSceneIndex}.
    HDSI_API
    std::vector<HdSceneIndexBaseRefPtr> GetInputScenes() const final;

    /// Returns {_proxyViewSceneIndex}.
    HDSI_API
    std::vector<HdSceneIndexBaseRefPtr> GetEncapsulatedScenes() const override;

    /// Delegates to the input scene index, then calls _ModifyPrim.
    HDSI_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const final;

    /// Delegates to the input scene index, then calls _FilterChildPrimPaths.
    HDSI_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const final;

protected:
    /// Constructor takes an input scene index and a predicate library.
    /// The predicate library is used in _MakeEvaluator.
    HDSI_API
    explicit HdsiExpressionEvaluatingSceneIndexBase(
        const HdSceneIndexBaseRefPtr &inputSceneIndex,
        const HdCollectionPredicateLibrary &predicateLibrary);

    // -----------------------------------------------------------------------
    // Query hooks

    /// Called by GetPrim after querying the input scene index.
    /// Override to modify or augment the returned prim. The default
    /// implementation returns the input prim unmodified.
    HDSI_API
    virtual void _ModifyPrim(
        const SdfPath &primPath, HdSceneIndexPrim *prim) const;

    /// Called by GetChildPrimPaths after querying the input scene index.
    /// Override to filter the returned children. The default implementation
    /// returns the input children unmodified.
    HDSI_API
    virtual void _FilterChildPrimPaths(
        const SdfPath &primPath, SdfPathVector *children) const;

    /// Override to bail early from GetPrim and GetChildPrimPaths for pruned
    /// prims. The default implementation returns false for all prims.
    HDSI_API
    virtual bool _IsPruned(const SdfPath &primPath) const;

    // -----------------------------------------------------------------------
    // Notice hooks (pure virtual)
    //
    // Each hook provides the input entries and returns a resullt that bundles
    // optional added, removed, and dirtied entries, and indicates whether to
    // forward the input notices as-is to downstream observers.
    //
    // The base class implementations of the non-virtual notice handlers call
    // these hooks and forwards the resulting notices in the following order:
    //     Added, Removed, Dirtied.
    //
    // The _IsObserved() check runs after the hook so derived classes can
    // track state unconditionally.

public:
    using OptionalAddedEntries =
        std::optional<HdSceneIndexObserver::AddedPrimEntries>;
    using OptionalRemovedEntries =
        std::optional<HdSceneIndexObserver::RemovedPrimEntries>;
    using OptionalDirtiedEntries =
        std::optional<HdSceneIndexObserver::DirtiedPrimEntries>;

    struct NoticeProcessingResult {
        static NoticeProcessingResult ForwardInputNotices() {
            return NoticeProcessingResult(true);
        }

        static NoticeProcessingResult Empty() {
            return NoticeProcessingResult(false);
        }

        explicit NoticeProcessingResult(bool forwardInputNotices)
            : forwardInputNotices(forwardInputNotices) {}

        NoticeProcessingResult& SetAddedEntries(
            const HdSceneIndexObserver::AddedPrimEntries &entries) {
            addedEntries = entries;
            return *this;
        }

        NoticeProcessingResult& SetRemovedEntries(
            const HdSceneIndexObserver::RemovedPrimEntries &entries) {
            removedEntries = entries;
            return *this;
        }

        NoticeProcessingResult& SetDirtiedEntries(
            const HdSceneIndexObserver::DirtiedPrimEntries &entries) {
            dirtiedEntries = entries;
            return *this;
        }

        bool forwardInputNotices;
        OptionalAddedEntries   addedEntries;
        OptionalRemovedEntries removedEntries;
        OptionalDirtiedEntries dirtiedEntries;
    };

protected:
    virtual NoticeProcessingResult _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &inputEntries) = 0;

    virtual NoticeProcessingResult _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &inputEntries) = 0;

    virtual NoticeProcessingResult _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &inputEntries) = 0;

    // Default implementation converts the renamed entries to removed and added
    // entries and calls _PrimsRemoved and _PrimsAdded. Derived classes can
    // override this to handle renamed entries directly.
    virtual NoticeProcessingResult _PrimsRenamed(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RenamedPrimEntries &inputEntries);

    // -----------------------------------------------------------------------
    // Utilities

    /// Constructs an HdInstancingAwareCollectionExpressionEvaluator for
    /// the given \p pathExpr using the predicate library provided to the
    /// constructor.
    ///
    HDSI_API
    HdInstancingAwareCollectionExpressionEvaluator _MakeEvaluator(
        const SdfPathExpression &pathExpr) const;

    /// Given a set of outermost instance prim paths (e.g. from
    /// HdInstancingAwareCollectionExpressionEvaluator::MatchResult::
    /// fullyMatchedInstances), returns the corresponding instancer prim paths
    /// as a vector (1:1 mapping).
    /// An empty SdfPath is returned for any input path that isn't an
    /// outermost instance (as tracked by the internal
    /// HdInstanceProxyViewSceneIndex).
    ///
    HDSI_API
    SdfPathVector GetInstancerPrimPaths(
        const SdfPathSet &outermostInstancePaths) const;

    /// Returns the input scene index.
    HDSI_API
    const HdSceneIndexBaseRefPtr &_GetInputSceneIndex() const;

private:

    friend class _Observer;

    /// Observer of the input scene index that delegates to the hooks above.
    /// In the future, to support overrides to a subtree of an instanced
    /// hierarchy, the observer will need to orchestrate a more complex
    /// sequence of operations to handle de-instancing.
    class _Observer : public HdSceneIndexObserver
    {
    public:
        _Observer(HdsiExpressionEvaluatingSceneIndexBase *owner)
            : _owner(owner) {}

        HDSI_API
        void PrimsAdded(
            const HdSceneIndexBase &sender,
            const AddedPrimEntries &entries) override;

        HDSI_API
        void PrimsRemoved(
            const HdSceneIndexBase &sender,
            const RemovedPrimEntries &entries) override;

        HDSI_API
        void PrimsDirtied(
            const HdSceneIndexBase &sender,
            const DirtiedPrimEntries &entries) override;

        HDSI_API
        void PrimsRenamed(
            const HdSceneIndexBase &sender,
            const RenamedPrimEntries &entries) override;

    private:
        HdsiExpressionEvaluatingSceneIndexBase *_owner;
    };

    // -----------------------------------------------------------------------
    // Data
    const HdSceneIndexBaseRefPtr _inputSceneIndex;
    const HdCollectionPredicateLibrary _predicateLibrary;

    // Note: The proxy view scene index should be created *before* the internal
    // observer to ensure that it has processed any notices before the observer
    // receives them.
    const HdInstanceProxyViewSceneIndexRefPtr _proxyViewSceneIndex;
    _Observer _observer;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HDSI_EXPRESSION_EVALUATING_SCENE_INDEX_BASE_H
