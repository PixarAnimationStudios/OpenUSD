//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_SPARSE_VECTORIZED_OUTPUT_TRAVERSER_H
#define PXR_EXEC_VDF_SPARSE_VECTORIZED_OUTPUT_TRAVERSER_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/maskMemoizer.h"
#include "pxr/exec/vdf/maskedOutput.h"
#include "pxr/exec/vdf/maskedOutputVector.h"
#include "pxr/exec/vdf/poolChainIndex.h"
#include "pxr/base/tf/smallVector.h"

#include <tbb/concurrent_unordered_map.h>

#include <functional>
#include <map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfNode;

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfSparseVectorizedOutputTraverser
///
/// Traverses a VdfNetwork in the input-to-output direction, while treating
/// each output in the traversal request as a separate traversal.
///
class VdfSparseVectorizedOutputTraverser
{
public:

    /// The callback invoked for all all terminal nodes. The integer parameter
    /// indicates which entry in the traversal request lead to the given node.
    ///
    using NodeCallback = std::function<bool (const VdfNode &, size_t)>;

    /// Starts a traversal with the given \p outputs request and node
    /// \p callback. The callback will be invoked for each terminal node
    /// visited. Note that \p callback MUST be thread-safe, since it may be
    /// invoked concurrently!
    /// 
    VDF_API
    void Traverse(
        const VdfMaskedOutputVector &outputs,
        const NodeCallback &callback);

    /// Invalidate the internal traversal cache.
    ///
    VDF_API
    void Invalidate();

private:

    // A cached dependency on a pool output.
    struct _PoolDependency {
        VdfPoolChainIndex poolChainIndex;
        VdfMaskedOutput maskedOutput;
    };

    // An entry with cached dependencies.
    struct _Dependencies {
        TfSmallVector<VdfMaskedOutput, 1> outputs;
        TfSmallVector<_PoolDependency, 1> poolOutputs;
        TfSmallVector<const VdfNode *, 1> nodes;
    };

    // A pair of output pointer and mask pointer used for building the
    // traversal stack (and queue.) The pointer-to-mask is used in order to
    // avoid expensive traffic on the mask ref count.
    struct _OutputAndMask {
        VdfOutput *output;
        const VdfMask *mask;
    };

    // A map with entries of outputs that have been visited, and with which
    // mask these outputs have been visited with.
    using _VisitedOutputs =
        TfHashMap<const VdfOutput *, const VdfMask *, TfHash>;

    // The type of output stack used to guide the traversal.
    using _OutputStack = std::vector<_OutputAndMask>;

    // The type of queue used to guide the traversal along the pool.
    using _PoolQueue = std::map<const VdfPoolChainIndex, _OutputAndMask>;

    // Returns true if the output should be visited and false if it already has
    // been visited with the given mask.
    bool _Visit(
        const _OutputAndMask &outputAndMask,
        _VisitedOutputs *visitedOutputs);

    // Start a new traversal at the given output.
    void _Traverse(
        size_t index,
        const VdfMaskedOutput &maskedOutput,
        const NodeCallback &callback);

    // Visits a single output.
    void _TraverseOutput(
        size_t index,
        const _OutputAndMask &outputAndMask,
        const NodeCallback &callback,
        _OutputStack *stack,
        _PoolQueue *queue);

    // Queue a pool output.
    void _QueuePoolOutput(
        const VdfPoolChainIndex &poolChainIndex,
        const _OutputAndMask &outputAndMask,
        _PoolQueue *queue);

    // Take a shortcut through the pool, if possible.
    bool _TakePoolShortcut(
        const _OutputAndMask &outputAndMask,
        _PoolQueue *queue);

    // Retrieves the dependencies for a single output, if cached, or computes
    // dependencies of uncached.
    const _Dependencies &_GetDependencies(
        const _OutputAndMask &outputAndMask);

    // Computes the dependencies for a single output.
    void _ComputeDependencies(
        const _OutputAndMask &outputAndMask,
        _Dependencies *dependencies);

    // The cached dependencies.
    using _DependencyMap = tbb::concurrent_unordered_map<
        VdfMaskedOutput, _Dependencies, VdfMaskedOutput::Hash>;
    _DependencyMap _dependencyMap;

    // The memoized mask operations.
    VdfMaskMemoizer<tbb::concurrent_unordered_map> _maskMemoizer;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
