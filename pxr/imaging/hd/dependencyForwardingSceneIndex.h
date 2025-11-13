//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_DEPENDENCY_FORWARDING_SCENE_INDEX_H
#define PXR_IMAGING_HD_DEPENDENCY_FORWARDING_SCENE_INDEX_H

#include "pxr/base/tf/denseHashSet.h"

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_unordered_set.h>
#include <tbb/concurrent_vector.h>

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

class HdDependencyForwardingSceneIndex;
TF_DECLARE_REF_PTRS(HdDependencyForwardingSceneIndex);


class HdDependencyForwardingSceneIndex
    : public HdSingleInputFilteringSceneIndexBase
{
public:

    static HdDependencyForwardingSceneIndexRefPtr New(
            HdSceneIndexBaseRefPtr inputScene) {
        return TfCreateRefPtr(
            new HdDependencyForwardingSceneIndex(inputScene));
    }

    // satisfying HdSceneIndexBase
    HD_API
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HD_API
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    // Provided for the sake of unit testing.
    // By default, or if SetManualGarbageCollect(false) is called,
    // we call RemoveDeletedEntries at the end of all of the notice
    // handlers...
    HD_API
    void SetManualGarbageCollect(bool manualGarbageCollect);

protected:
    HD_API
    HdDependencyForwardingSceneIndex(HdSceneIndexBaseRefPtr inputScene);

    // satisfying HdSingleInputFilteringSceneIndexBase
    void _PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries) override;

    void _PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries) override;

    void _PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries) override;
private:

    // -----------------------------------------------------------------------

    // XXX: Note for future performance improvements: giving dependency schemas
    // the ability to represent set->set dependencies, and compacting
    // these locator entries into set->set, could potentially be a big perf
    // win, since set/set intersections are vastly more efficient than
    // pairwise locator intersections.
    //
    // Since multiple affected prims could be populating _dependedOn locator
    // entries from different threads, and we use the individual dependency
    // name for lifetime management, we'd probably need to populate these
    // individually and then compact the next time we get a chance from
    // a single threaded API (e.g. notice handler).
    //
    // We can compact as follows:
    //  1. Construct a dictionary of "affectedDataSourceLocator" ->
    //     vector of "dependedOnDataSourceLocator", and then flatten that into
    //     a locator set.
    //  2. Construct a dictionary of "dependedOnDataSourceLocatorSet" ->
    //     vector of "affectedDataSourceLocator", and then flatten that into
    //     a locator set.
    struct _LocatorsEntry
    {
        HdDataSourceLocator dependedOnDataSourceLocator;
        HdDataSourceLocator affectedDataSourceLocator;
    };

    // The token used as a key here corresponds to the first member of an
    // HdDependenciesSchema::EntryPair and provides an identifier for a
    // dependency declaration. An affected prim may depend on more than one
    // data source of another prim. That identifier is used here for updating
    // or removing a dependency.
    using _LocatorsEntryMap = tbb::concurrent_unordered_map<
        TfToken,
        _LocatorsEntry,
        TfToken::HashFunctor>;


    struct _AffectedPrimDependencyEntry
    {
        // Gotta define this stuff because the atomic_bool isn't copyable.
        // We know we'll only be copying in single-threaded cases, so the naive
        // implementation is fine...
        // flaggedForDeletion is an atomic_bool so we can use parallel for in
        // the notice handlers.
        _AffectedPrimDependencyEntry()
            : locatorsEntryMap(), flaggedForDeletion(false) {}
        _AffectedPrimDependencyEntry(const _AffectedPrimDependencyEntry &rhs)
            : locatorsEntryMap(rhs.locatorsEntryMap)
            , flaggedForDeletion(rhs.flaggedForDeletion.load()) {}
        _AffectedPrimDependencyEntry(_AffectedPrimDependencyEntry &&rhs)
            : locatorsEntryMap(std::move(rhs.locatorsEntryMap))
            , flaggedForDeletion(rhs.flaggedForDeletion.load()) {}
        _AffectedPrimDependencyEntry& operator=(
                const _AffectedPrimDependencyEntry& rhs) {
            if (this != &rhs) {
                locatorsEntryMap = rhs.locatorsEntryMap;
                flaggedForDeletion = rhs.flaggedForDeletion.load();
            }
            return *this;
        }

        _LocatorsEntryMap locatorsEntryMap;
        std::atomic_bool flaggedForDeletion;
    };

    // Reverse mapping from a depended on prim to its discovered-thus-far
    // affected prims and data source locators..
    using _AffectedPrimsDependencyMap = tbb::concurrent_unordered_map<
        SdfPath,
        _AffectedPrimDependencyEntry,
        SdfPath::Hash>;


    // Top-level map keyed by paths of depended-on paths
    using _DependedOnPrimsAffectedPrimsMap = tbb::concurrent_unordered_map<
        SdfPath,
        _AffectedPrimsDependencyMap,
        SdfPath::Hash>;


    // Lazily-populated mapping of depended on paths to the affected paths
    // and data source locators used for forwarding of dirtying.
    // NOTE: This is mutable because it can be updated during calls to
    //       GetPrim -- which is defined as const within HdSceneIndexBase.
    //       This is in service of lazy population goals.
    //
    // XXX: This map is "dependedOn" -> "affected" -> vec (loc -> loc)
    // ... a potential optimization we might want to make is to switch the
    // organization to: "dependedOn" -> loc -> vec ("affected" -> loc).
    // This would make dependency management more difficult; since dependencies
    // may be introduced from different threads, we'd probably need to compact
    // them into the latter representation.  But it would introduce more
    // early-out opportunities in PrimsDirtied, which would be great especially
    // for prims that have a high dependency fan-out (e.g. scene globals).
    mutable _DependedOnPrimsAffectedPrimsMap _dependedOnPrimToDependentsMap;


    // -----------------------------------------------------------------------

    using _PathSet = tbb::concurrent_unordered_set<SdfPath, SdfPath::Hash>;

    struct _AffectedPrimToDependsOnPathsEntry
    {
        // Gotta define this stuff because the atomic_bool isn't copyable.
        // We know we'll only be copying in single-threaded cases, so the naive
        // implementation is fine...
        _AffectedPrimToDependsOnPathsEntry()
            : dependsOnPaths(), flaggedForDeletion(false) {}
        _AffectedPrimToDependsOnPathsEntry(
                const _AffectedPrimToDependsOnPathsEntry &rhs)
            : dependsOnPaths(rhs.dependsOnPaths)
            , flaggedForDeletion(rhs.flaggedForDeletion.load()) {}
        _AffectedPrimToDependsOnPathsEntry(
                _AffectedPrimToDependsOnPathsEntry &&rhs)
            : dependsOnPaths(std::move(rhs.dependsOnPaths))
            , flaggedForDeletion(rhs.flaggedForDeletion.load()) {}
        _AffectedPrimToDependsOnPathsEntry& operator=(
                const _AffectedPrimToDependsOnPathsEntry& rhs) {
            if (this != &rhs) {
                dependsOnPaths = rhs.dependsOnPaths;
                flaggedForDeletion = rhs.flaggedForDeletion.load();
            }
            return *this;
        }

        _PathSet dependsOnPaths;
        std::atomic_bool flaggedForDeletion;
    };


    using _AffectedPrimToDependsOnPathsEntryMap = tbb::concurrent_unordered_map<
            SdfPath,
            _AffectedPrimToDependsOnPathsEntry,
            SdfPath::Hash>;

    // lazily-populated set of depended on paths for affected prims. This
    // is used to update _dependedOnPrimToDependentsMap when a prim's
    // __dependencies data source is dirtied (or the prim is removed)
    // NOTE: This is mutable because it can be updated during calls to
    //       GetPrim -- which is defined as const within HdSceneIndexBase.
    //       This is in service of lazy population goals.
    mutable _AffectedPrimToDependsOnPathsEntryMap
        _affectedPrimToDependsOnPathsMap;

    // -----------------------------------------------------------------------

    void _ClearDependencies(const SdfPath &primPath);
    void _UpdateDependencies(const SdfPath &primPath) const;
    void _ResetDependencies();

    // -----------------------------------------------------------------------

    // Dependencies may reasonably describe cycles given that:
    // 1) Dependancies can exist at different levels of data source nesting
    // 2) Dependancy declarations can be present from multiple upstream
    //    scene indices -- each of which draws its value from its input.
    //    In that case, it's not a cycle which affects a computed value but
    //    rather indicates to observers of this scene index that a value
    //    should be repulled.
    //
    // When following affected paths to propogate dirtiness, we need to detect
    // cycles to avoiding hanging. This is done is by sending a "visited" set
    // containing these node keys:
    struct _VisitedNode
    {
        SdfPath primPath;
        HdDataSourceLocator locator;

        inline bool operator==(_VisitedNode const &rhs) const noexcept
        {
            return primPath == rhs.primPath && locator == rhs.locator;
        }

        template <class HashState>
        friend void TfHashAppend(HashState &h, _VisitedNode const &myObj) {
            h.Append(myObj.primPath);
            h.Append(myObj.locator);
        }

        inline size_t Hash() const;
        struct HashFunctor {
            size_t operator()(_VisitedNode const &node) const {
                return node.Hash();
            }
        };
    };

    using _VisitedNodeSet = tbb::concurrent_unordered_set<
        _VisitedNode,
        _VisitedNode::HashFunctor>;

    using _AdditionalDirtiedVector =
        tbb::concurrent_vector<HdSceneIndexObserver::DirtiedPrimEntry>;

    // impl for PrimDirtied which handles propogation of PrimDirtied notices
    // for affected prims/dataSources.
    void _PrimDirtied(
        const SdfPath &primPath,
        const HdDataSourceLocatorSet &sourceLocator,
        _VisitedNodeSet *visited,
        _AdditionalDirtiedVector *moreDirtiedEntries,
        _PathSet *rebuildDependencies);

    // -----------------------------------------------------------------------

    // accumulated depended-on prim paths whose affected prims may have been
    // removed.
    mutable _PathSet _potentiallyDeletedDependedOnPaths;

    // Accumulated affected prim paths who may have been deleted. Normally this
    // is needed to track affected prims which have an entry in
    // _dependedOnPrimToDependentsMap but which is empty -- and therefore
    // won't be handled by their dependencies inclusion in
    // _potentiallyDeletedDependedOnPaths
    mutable _PathSet _potentiallyDeletedAffectedPaths;

    // -----------------------------------------------------------------------

public:
    // XXX does thread-unsafe deletion.
    // NOTE: optional arguments are in service of unit testing to provide
    //       insight in to what was removed.
    HD_API
    void RemoveDeletedEntries(
        SdfPathVector *removedAffectedPrimPaths = nullptr,
        SdfPathVector *removedDependedOnPrimPaths = nullptr);

private:
    bool _manualGarbageCollect;
};


inline size_t
HdDependencyForwardingSceneIndex::_VisitedNode::Hash() const
{
    return TfHash()(*this);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
