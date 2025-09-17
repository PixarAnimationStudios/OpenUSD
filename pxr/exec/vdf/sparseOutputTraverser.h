//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_SPARSE_OUTPUT_TRAVERSER_H
#define PXR_EXEC_VDF_SPARSE_OUTPUT_TRAVERSER_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/maskedOutputVector.h"
#include "pxr/exec/vdf/types.h"

#include "pxr/base/tf/hashmap.h"

#include <queue>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfInput;
class VdfMask;
class VdfOutput;

/// A class used for fast traversals of VdfNetworks in the input-to-output
/// direction.
///
/// The VdfSparseOutputTraverser class can be used to quickly traverse networks.
/// The main API are the TraverseOnce() and Traverse() methods.  The static 
/// and non-static methods should have exactly the same behavior.  The 
/// non-static method has the opportunity to perform much faster traversals 
/// if similar traversals are repeatedly invoked using the same traverser
/// object (when caching is enabled).
///
class VdfSparseOutputTraverser
{
public:
    /// The number of requests to remain in the cache.
    ///
    /// An attempt to store more than MaxRequestsBeforeEviction requests in
    /// the traversal cache will result in the eviction of the m oldest
    /// cache entries, such that:
    ///     cacheSize - m = MaxRequestsBeforeEviction.
    ///
    /// This enviction policy is enforced, every time a new
    /// traversal is started with TraverseWithCaching.
    ///
    /// NOTE:   Setting MaxRequestsBeforeEviction to -1 will disable
    ///         the eviction algorithm all together. 0 will disable caching.
    static const int MaxRequestsBeforeEviction = 10;

    /// Creates a new VdfSparseOutputTraverser.
    ///
    /// \p enableCaching controls whether or not traversals should cache
    /// information that can be used to speed up similar traversals later.
    /// By default traversal caching is enabled.
    ///
    VDF_API
    VdfSparseOutputTraverser(bool enableCaching = true);

    /// Callback used when traversing a network.
    ///
    /// The callback is supplied with the current output, a mask for the
    /// output, and the input through which the current output was reached,
    /// if any.
    ///
    /// Called for each output traversed. If the callback returns false, 
    /// traversal will be stopped at that output.
    ///
    typedef std::function<
        bool (const VdfOutput &, const VdfMask &, const VdfInput *) >
        OutputCallback;


    /// \name Output Traversal
    /// @{

    /// Traverses the network, starting from the masked outputs in
    /// \p outputs.
    ///
    /// Performs an optimized vectorized traversal.
    ///
    /// Calls \p outputCallback for each output that is visited, passing the
    /// accumulated dependency mask.  If the callback returns \c true,
    /// traversal continues; otherwise, it terminates. The \p outputCallback
    /// is optional and may be null.
    ///
    /// Calls \p nodeCallback for each node that is visited. The \p nodeCallback
    /// is optional and may be null.
    /// 
    VDF_API
    static void Traverse(
        const VdfMaskedOutputVector &outputs,
        const OutputCallback        &outputCallback,
        const VdfNodeCallback       &nodeCallback);

    /// Traverses the network, starting from the masked outputs in
    /// \p request.
    ///
    /// In addition to the functionality of the static method Traverse(),
    /// this non-static method may be faster because the
    /// VdfSparseOutputTraverser object will cache some traversals.
    /// 
    VDF_API
    void TraverseWithCaching(
        const VdfMaskedOutputVector &outputs,
        const OutputCallback        &outputCallback,
        const VdfNodeCallback       &nodeCallback);

    /// @}


    /// Invalidates all cached traversals.
    ///
    /// To be called if network changes.
    ///
    VDF_API
    void InvalidateAll();

private:
    
    struct _CacheEntry;

    // A cache line stored in the traversal cache map
    struct _Cache
    {
        // The indices to all the root cache entries, representing the root
        // nodes in the cached request
        std::vector<int> rootIndices;

        // A vector of cache entries
        typedef std::vector<_CacheEntry> CacheEntries;
        CacheEntries cacheEntries;
    };

    // Returns a new or existing cache entry keyed off of the sorted request.
    // This method will also enforce the eviction policy.
    _Cache* _GetOrCreateCacheEntry(const VdfMaskedOutputVector &sortedOutputs);

    class _TraversalHelper;

private:

    // Flag to switch caching on and off (defaults to on).
    bool _enableCaching;

    // A map from masked outputs to _Cache objects. 
    using _TraversalCache =
        TfHashMap<VdfMaskedOutputVector, _Cache, VdfMaskedOutputVector_Hash>;

    // Cache used to speed up repeated traversals.
    _TraversalCache _cache;

    // The cache history is defined as a queue adaptor with an underlaying deque
    using _CacheHistory = std::queue<_TraversalCache::iterator>;

    // Maintain a history of added cache entries to allow for eviction
    // of oldest cache entries
    _CacheHistory _cacheHistory;

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
