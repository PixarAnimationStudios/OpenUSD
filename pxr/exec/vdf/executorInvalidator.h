//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_EXECUTOR_INVALIDATOR_H
#define PXR_EXEC_VDF_EXECUTOR_INVALIDATOR_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/lruCache.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/maskMemoizer.h"
#include "pxr/exec/vdf/maskedOutput.h"
#include "pxr/exec/vdf/maskedOutputVector.h"
#include "pxr/exec/vdf/poolChainIndex.h"
#include "pxr/exec/vdf/types.h"

#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/smallVector.h"

#include <map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfExecutorInterface;

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfExecutorInvalidator
/// 
/// Invalidates state and temporary buffers of all outputs dependent on the
/// outputs supplied in the invalidation request. Stores internal state in order
/// to accelerate subsequent invalidation requests.
/// 
class VdfExecutorInvalidator
{
public:

    /// Construct an executor invalidator for the given executor. The
    /// invalidator will mutate the state of the given executor.
    ///
    VdfExecutorInvalidator(VdfExecutorInterface *executor) :
        _executor(executor),
        _timestamp(0),
        _replayLRU(16)
    {}

    /// Invalidate all the outputs in the \p request, as well as all the
    /// outputs dependent on the \p request.
    ///
    VDF_API
    void Invalidate(const VdfMaskedOutputVector &request);

    /// Reset the internal state of the invalidator. This method must be called
    /// on topological state changes.
    ///
    VDF_API
    void Reset();

private:

    // Information about a visited output.
    struct _Visited {
        explicit _Visited(uint32_t ts) : timestamp(ts), index(0) {}
        uint32_t timestamp;
        uint32_t index;
        VdfMask mask;
    };

    // A cached dependency on a pool output.
    struct _PoolDependency {
        VdfPoolChainIndex poolChainIndex;
        VdfMaskedOutput maskedOutput;
    };

    // An entry with cached dependencies.
    struct _Dependencies {
        TfSmallVector<VdfMaskedOutput, 1> outputs;
        TfSmallVector<_PoolDependency, 1> poolOutputs;
        TfSmallVector<const VdfInput *, 1> inputs;
    };

    // A cached invalidation entry for fast invalidation replay. The entry
    // stores two masks, one for visits for which the invalidation callback
    // returned true, and one for visits where it returned false.
    struct _ReplayEntry {
        explicit _ReplayEntry(const VdfOutput *o) : output(o) {}
        const VdfOutput *output;
        VdfMask masks[2];
    };

    // The cache of invalidation entries for fast replay. Every output has a
    // unique entry in the cache, so that it can be replayed in parallel
    // without risk of racing on the same output.
    struct _ReplayCache {
        std::vector<_ReplayEntry> entries;
        VdfNodeToInputPtrVectorMap inputs;
    };

    // Array of visited outputs to information about the visit, indexed by
    // output id.
    using _VisitedMap = std::vector<_Visited>;

    // The type of output stack used to guide the traversal.
    using _OutputStack = std::vector<VdfMaskedOutput>;

    // The type of queue used to guide the traversal along the pool.
    using _PoolQueue = std::map<const VdfPoolChainIndex, VdfMaskedOutput>;

    // Returns true if all the outputs in the given request are already invalid.
    bool _IsInvalid(const VdfMaskedOutputVector &request) const;

    // Returns a pointer to an existing replay cache for the given outputs.
    // Creates a new cache if one does not already exist.
    _ReplayCache* _GetReplayCache(const VdfMaskedOutputVector &outputs);

    // Returns a valid pointer to an index if this output should be visited, or
    // nullptr if the output has already been visited with the given mask. If
    // the returned index equals nextIndex, the output is being visited for the
    // first time.
    uint32_t *_Visit(const VdfMaskedOutput &maskedOutput, uint32_t nextIndex);

    // Initiates a new traversal starting at the outputs in request.
    void _Traverse(
        const VdfMaskedOutputVector &request,
        _ReplayCache *replayCache);

    // Visits a single output.
    bool _TraverseOutput(
        const VdfMaskedOutput &maskedOutput,
        _OutputStack *stack,
        _PoolQueue *queue,
        VdfNodeToInputPtrVectorMap *inputs);

    // Replay a cached invalidation traversal. Returns false if the cache could
    // not be successfully replayed and a new traversal must be started.
    bool _Replay(const _ReplayCache &replayCache);

    // Retrieves the dependencies for a single output, if cached, or computes
    // dependencies of uncached.
    const _Dependencies &_GetDependencies(
        const VdfMaskedOutput &maskedOutput);

    // Computes the dependencies for a single output.
    void _ComputeDependencies(
        const VdfMaskedOutput &maskedOutput,
        _Dependencies *dependencies);

    // Pointer to the executor that is being invalidated.
    VdfExecutorInterface *_executor;

    // The map of visited outputs.
    _VisitedMap _visited;

    // A timestamp denoting the current round of invalidation. Will be
    // incremented for every subsequent round of invalidaiton.
    uint32_t _timestamp;

    // The cached dependencies.
    using _DependencyMap = TfHashMap<
        VdfMaskedOutput, _Dependencies, VdfMaskedOutput::Hash>;
    _DependencyMap _dependencyMap;

    // A list of recently used replay caches.
    using _ReplayLRU = VdfLRUCache<
        VdfMaskedOutputVector, _ReplayCache, VdfMaskedOutputVector_Hash>;
    _ReplayLRU _replayLRU;

    // The memoized mask operations.
    VdfMaskMemoizer<TfHashMap> _maskMemoizer;

};

////////////////////////////////////////////////////////////////////////////////

PXR_NAMESPACE_CLOSE_SCOPE

#endif
