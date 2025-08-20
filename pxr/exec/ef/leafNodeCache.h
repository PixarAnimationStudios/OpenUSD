//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_LEAF_NODE_CACHE_H
#define PXR_EXEC_EF_LEAF_NODE_CACHE_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/ef/api.h"
#include "pxr/exec/ef/dependencyCache.h"
#include "pxr/exec/ef/leafNodeIndexer.h"

#include "pxr/base/tf/hashmap.h"
#include "pxr/exec/vdf/maskedOutputVector.h"
#include "pxr/exec/vdf/sparseVectorizedOutputTraverser.h"
#include "pxr/exec/vdf/types.h"

#include <atomic>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// \class EfLeafNodeCache
///
/// This cache is a thin wrapper around the EfDependencyCache.
///
/// It caches node and output dependencies on EfLeafNodes, given an arbitrary
/// input request.
///
class EfLeafNodeCache
{
public:
    /// Constructor.
    ///
    EF_API
    EfLeafNodeCache();

    /// Returns the current edit version of the leaf node cache. This number
    /// will be incremented whenever leaf node dependency changes due to
    /// network edits, or time dependency modifications. Note, that no
    /// assumptions can be made about the absolute value returned from this
    /// function. The only guarantee provided is that if two versions compare
    /// equal, leaf node dependencies have not changed.
    ///
    size_t GetVersion() const {
        return _version.load(std::memory_order_relaxed);
    }

    /// Find outputs dependent on the given \p outputs.
    ///
    EF_API
    const VdfOutputToMaskMap &FindOutputs(
        const VdfMaskedOutputVector &outputs,
        bool updateIncrementally);

    /// Find leaf nodes dependent on the given \p outputs
    ///
    EF_API
    const std::vector<const VdfNode *> &FindNodes(
        const VdfMaskedOutputVector &outputs,
        bool updateIncrementally);

    /// Find all leaf nodes dependent on the given \p outputs, but only return
    /// the nodes dependent on the requested outputs not filtered out by the
    /// \p outputsMask. A previously passed in request of \p outputs will return
    /// a cache hit regardless of the value of \p outputsMask.
    ///
    EF_API
    const std::vector<const VdfNode *> &FindNodes(
        const VdfMaskedOutputVector &outputs,
        const TfBits &outputsMask);

    /// Clear the entire cache.
    ///
    EF_API
    void Clear();

    /// Call this to notify the cache of connections that have been deleted.
    ///
    /// \note It is safe to call WillDeleteConnection() and DidConnect()
    /// concurrently.
    ///
    EF_API
    void WillDeleteConnection(const VdfConnection &connection);

    /// Call this to notify the cache of newly added connections.
    ///     
    /// \note It is safe to call WillDeleteConnection() and DidConnect()
    /// concurrently.
    ///
    EF_API
    void DidConnect(const VdfConnection &connection);

private:
    // Holds the sets of leaf nodes, one for each output in the request, along
    // with cached, combined sets of leaf nodes given a mask of the requested
    // outputs. The sets contain indices into the leaf node indexer.
    struct _VectorizedCacheEntry {
        std::vector<TfBits> leafNodes;
        TfHashMap<TfBits, TfBits, TfBits::FastHash> combinedLeafNodes;
    };

    // Stores an array of leaf nodes, and outputs connected to these leaf nodes.
    struct _SparseCacheEntry {
        std::vector<const VdfNode *> nodes;
        VdfOutputToMaskMap outputs;
    };

    // Clears the vectorized and sparse caches along with the traverser used to
    // populate those caches, if their internal state has been flagged as being
    // invalid.
    void _ClearCachesIfInvalid();

    // Combine separate sets of leaf nodes into a single set.
    TfBits _CombineLeafNodes(
        const TfBits &outputsMask,
        const std::vector<TfBits> &leafNodes) const;

    // Populates the vectorized cache by doing a vectorized traversal.
    _VectorizedCacheEntry *_PopulateVectorizedEntry(
        const VdfMaskedOutputVector &outputs);

    // Populates the sparse cache by using the results from a previous,
    // vectorized traversal.
    _SparseCacheEntry *_PopulateSparseEntry(
        const VdfMaskedOutputVector &outputs,
        const TfBits &leafNodes);

    // The version of the cache. Incremented with every edit.
    std::atomic<size_t> _version;

    // Indicates that the internal state pertaining to vectorized and sparse
    // caches is invalid and that those caches be cleared.
    std::atomic<bool> _cachesAreInvalid;

    // The dependency cache used for fast lookups of input-to-output
    // dependencies.
    EfDependencyCache _dependencyCache;

    // The leaf node indexer associates each leaf node with a unique index.
    Ef_LeafNodeIndexer _indexer;

    // A cache of requested outputs to leaf node dependencies for each
    // individual output in the request.
    using _VectorizedCache = TfHashMap<
        VdfMaskedOutputVector,
        _VectorizedCacheEntry,
        VdfMaskedOutputVector_Hash>;
    _VectorizedCache _vectorizedCache;

    // A cache of requested outputs to leaf nodes and leaf node connected
    // outputs.
    using _SparseCache = TfHashMap<
        VdfMaskedOutputVector,
        _SparseCacheEntry,
        VdfMaskedOutputVector_Hash>;
    _SparseCache _sparseCache;

    // The traverser used to populate the vectorized cache.
    VdfSparseVectorizedOutputTraverser _traverser;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
