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
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;


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
        _LocatorsEntryMap locatorsEntryMap;
        bool flaggedForDeletion = false;
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
    mutable _DependedOnPrimsAffectedPrimsMap _dependedOnPrimToDependentsMap;


    // -----------------------------------------------------------------------

    using _PathSet = tbb::concurrent_unordered_set<SdfPath, SdfPath::Hash>;

    //using _DensePathSet = TfDenseHashSet<SdfPath, SdfPath::Hash>;

    struct _AffectedPrimToDependsOnPathsEntry
    {
        _PathSet dependsOnPaths;
        bool flaggedForDeletion = false;
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

    using _VisitedNodeSet = TfDenseHashSet<
        _VisitedNode,
        _VisitedNode::HashFunctor>;

    // impl for PrimDirtied which handles propogation of PrimDirtied notices
    // for affected prims/dataSources.
    void _PrimDirtied(
        const SdfPath &primPath,
        const HdDataSourceLocator &sourceLocator,
        _VisitedNodeSet *visited,
        HdSceneIndexObserver::DirtiedPrimEntries *moreDirtiedEntries);

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
    // NOTE FOR REVIEWERS: temporarily hiding this explosive public method
    //                     down here while we discuss it. It's public because
    //                     only the application knows when it's safe to call?
    //
    // NOTE: optional arguments are in service of unit testing to provide
    //       insight in to what was removed.
    HD_API
    void RemoveDeletedEntries(
        SdfPathVector *removedAffectedPrimPaths = nullptr,
        SdfPathVector *removedDependedOnPrimPaths = nullptr);

};


inline size_t
HdDependencyForwardingSceneIndex::_VisitedNode::Hash() const
{
    return TfHash()(*this);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
