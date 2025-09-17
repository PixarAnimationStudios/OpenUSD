//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_CACHING_SCENE_H
#define PXR_IMAGING_HD_CACHING_SCENE_H

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/filteringSceneIndex.h"

#include "pxr/usd/sdf/pathTable.h"

#include <tbb/concurrent_hash_map.h>

#include <optional>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(HdCachingSceneIndex);

///
/// \class HdCachingSceneIndex
///
/// A scene index that caches the prim data source and child prim paths.
///
class HdCachingSceneIndex : public HdSingleInputFilteringSceneIndexBase
{
public:
    /// Creates a new caching scene index.
    ///
    static HdCachingSceneIndexRefPtr New(
            HdSceneIndexBaseRefPtr const &inputScene) {
        return TfCreateRefPtr(
            new HdCachingSceneIndex(inputScene));
    }

    HD_API
    ~HdCachingSceneIndex() override;

    HD_API 
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    HD_API 
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

protected:

    HD_API
    HdCachingSceneIndex(
        HdSceneIndexBaseRefPtr const &inputScene);

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
    // Implemented similarly to HdFlatteningSceneIndex - without flattening.

    // Consolidate _recentPrims into _prims.
    void _ConsolidateRecentPrims();

    // Consolidate _recentChildPaths into _childPaths.
    void _ConsolidateRecentChildPaths();

    // Consolidate both.
    void _ConsolidateRecent();
    
    struct _PathHashCompare {
        static bool equal(const SdfPath &a, const SdfPath &b) {
            return a == b;
        }
        static size_t hash(const SdfPath &path) {
            return hash_value(path);
        }
    };
    
    // members
    using _PrimTable = SdfPathTable<std::optional<HdSceneIndexPrim>>;
    _PrimTable _prims;

    using _RecentPrimTable =
        tbb::concurrent_hash_map<SdfPath, HdSceneIndexPrim, _PathHashCompare>;
    mutable _RecentPrimTable _recentPrims;

    using _ChildPathsTable = SdfPathTable<std::optional<SdfPathVector>>;
    _ChildPathsTable _childPaths;

    using _RecentChildPathsTable =
        tbb::concurrent_hash_map<SdfPath, SdfPathVector, _PathHashCompare>;
    mutable _RecentChildPathsTable _recentChildPaths;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
