//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDSI_PRIMTYPE_NOTICE_BATCHING_SCENE_INDEX_H
#define PXR_IMAGING_HDSI_PRIMTYPE_NOTICE_BATCHING_SCENE_INDEX_H

/// \file hdsi/primTypeNoticeBatchingSceneIndex.h

#include "pxr/imaging/hdsi/api.h"

#include "pxr/imaging/hd/filteringSceneIndex.h"

#include <variant>

PXR_NAMESPACE_OPEN_SCOPE

#define HDSI_PRIM_TYPE_NOTICE_BATCHING_SCENE_INDEX_TOKENS \
    (primTypePriorityFunctor)

TF_DECLARE_PUBLIC_TOKENS(HdsiPrimTypeNoticeBatchingSceneIndexTokens, HDSI_API,
                         HDSI_PRIM_TYPE_NOTICE_BATCHING_SCENE_INDEX_TOKENS);

TF_DECLARE_REF_PTRS(HdsiPrimTypeNoticeBatchingSceneIndex);

/// \class HdsiPrimTypeNoticeBatchingSceneIndex
///
/// A filtering scene index batching prim notices by type using a given
/// priority functor. The notices are held back until a call to Flush.
///
/// The scene index consolidates prim notices.
/// For example, if we get a several prim dirtied entries for the same path, it
/// will turn into a single entry with the dirty locator set being the union.
/// If we get several prim added and dirtied entries for the same path, it
/// results in a single prim added entry. Added and dirtied entries for paths
/// prefixed by a later prim removed entry will be effectively ignored. A
/// removed entry for a name space ancestor of another removed entry will also
/// be effectively removed.
///
/// When Flush is called all removed entries are sent out and then followed
/// by the cummulated added and dirtied prim entries grouped by their prim
/// priority.
///
/// The filtering scene index is empty until the first call to Flush.
///
class HdsiPrimTypeNoticeBatchingSceneIndex
    : public HdSingleInputFilteringSceneIndexBase
{
public:
    /// \class HdsiPrimTypeNoticeBatchingSceneIndex::PrimTypePriorityFunctor
    ///
    /// Base class for functor mapping prim types to priorities.
    ///
    class PrimTypePriorityFunctor
    {
    public:
        HDSI_API
        virtual ~PrimTypePriorityFunctor();

        /// Priority for given prim type. Prims with lower priority number
        /// are handled before prims with higher priority number.
        /// Result must be less than GetNumPriorities().
        ///
        virtual size_t GetPriorityForPrimType(
            const TfToken &primType) const = 0;
        
        /// Number of priorities - that is 1 + the highest number ever
        /// returned by GetPriorityForPrimType().
        ///
        /// This number should be small as it affects the pre-allocation
        /// in Flush().
        ///
        virtual size_t GetNumPriorities() const = 0;
    };
    using PrimTypePriorityFunctorHandle =
        std::shared_ptr<PrimTypePriorityFunctor>;

    /// Creates a new notice batching scene index. It expects a priority functor
    /// in a PrimTypePriorityFunctorHandle typed data source at
    /// HdsiPrimTypeNoticeBatchingSceneIndexTokens->primTypePriorityFunctor in
    /// the given inputArgs.
    ///
    static HdsiPrimTypeNoticeBatchingSceneIndexRefPtr New(
            HdSceneIndexBaseRefPtr const &inputScene,
            HdContainerDataSourceHandle const &inputArgs) {
        return TfCreateRefPtr(
            new HdsiPrimTypeNoticeBatchingSceneIndex(
                inputScene,
                inputArgs));
    }

    HDSI_API
    ~HdsiPrimTypeNoticeBatchingSceneIndex() override;

    /// Forwards to input scene after first call to Flush. Empty before that.
    ///
    HDSI_API 
    HdSceneIndexPrim GetPrim(const SdfPath &primPath) const override;

    /// Forwards to input scene after first call to Flush. Empty before that.
    ///
    HDSI_API 
    SdfPathVector GetChildPrimPaths(const SdfPath &primPath) const override;

    /// Sends out all notices queued and commulated since the last call to
    /// Flush. The first call to Flush will also send out notices for prims
    /// that were in the input scene index when it was added to this filtering
    /// scene index.
    ///
    HDSI_API
    void Flush();

protected:

    HDSI_API
    HdsiPrimTypeNoticeBatchingSceneIndex(
        HdSceneIndexBaseRefPtr const &inputScene,
        HdContainerDataSourceHandle const &inputArgs);

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

    // Removes items from _addedOrDirtiedPrims prefixed by path.
    void _RemovePathFromAddedOrDirtiedPrims(const SdfPath &path);
    // Adds path to _removedPrims and normalizes _removedPrims if necessary.
    void _AddPathToRemovedPrims(const SdfPath &path);

    size_t _GetPriority(const TfToken &primType) const;

    PrimTypePriorityFunctorHandle const _primTypePriorityFunctor;
    const size_t _numPriorities;

    struct _PrimAddedEntry
    {
        TfToken primType;
    };

    struct _PrimDirtiedEntry
    {
        HdDataSourceLocatorSet dirtyLocators;
    };

    // True after first call to flush.
    bool _populated;

    // Default constructored _PrimAddedOrDirtiedEntry contains a
    // _PrimDirtiedEntry. This is used by _addedOrDirtiedPrims.
    using _PrimAddedOrDirtiedEntry =
        std::variant<_PrimDirtiedEntry, _PrimAddedEntry>;

    std::map<SdfPath, _PrimAddedOrDirtiedEntry> _addedOrDirtiedPrims;

    // Normalized, so a prefix of an element in _removedPrims will never be in
    // _removedPrims.
    std::set<SdfPath> _removedPrims;
};


PXR_NAMESPACE_CLOSE_SCOPE


#endif
