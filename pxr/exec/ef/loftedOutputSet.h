//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_LOFTED_OUTPUT_SET_H
#define PXR_EXEC_EF_LOFTED_OUTPUT_SET_H

#include "pxr/pxr.h"

#include "pxr/exec/ef/api.h"

#include "pxr/exec/vdf/types.h"

#include <tbb/concurrent_hash_map.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfMaskedOutput;
class VdfNetwork;
class VdfMask;
using VdfMaskedOutputVector = std::vector<VdfMaskedOutput>;
class VdfOutput;

/// Tracks the set of lofted outputs (outputs whose values were sourced from
/// the page cache during evaluation) on behalf of EfPageCacheBasedExecutor.
///
class Ef_LoftedOutputSet
{
public:
    EF_API
    Ef_LoftedOutputSet();

    EF_API
    ~Ef_LoftedOutputSet();

    /// Returns the number of outputs lofted into the page cache.
    size_t GetSize() const {
        return _loftedOutputs.size();
    }

    /// Allocates storage to accommodate the maximum capacity of the network.
    ///
    /// Callers *must* ensure that the set has been sized to the network before
    /// adding outputs.  This is not an optional performance optimization; if
    /// the network capacity changes, the set must be resized.
    ///
    EF_API
    void Resize(const VdfNetwork &network);

    /// Adds an output to the set of lofted outputs.
    ///
    /// Returns true if this operation succeeds.
    ///
    EF_API
    bool Add(
        const VdfOutput &output,
        const VdfMask &mask);

    /// Removes an output from the set of lofted outputs.
    EF_API
    void Remove(
        const VdfId outputId,
        const VdfId nodeId,
        const VdfMask &mask);

    /// Removes all outputs on node from the set of lofted outputs.
    EF_API
    void RemoveAllOutputsForNode(const VdfNode &node);

    /// Removes all outputs from the set of lofted outputs.
    EF_API
    void Clear();

    /// Inserts any lofted outputs in \p deps into \p processedRequest.
    EF_API
    void CollectLoftedDependencies(
        const VdfOutputToMaskMap &deps,
        VdfMaskedOutputVector *processedRequest) const;

private:
    // A set of outputs, which had their values sourced from the page cache
    // during evaluation (or getting of output values.) We need to keep track
    // of these outputs in order to allows us to later properly invalidate them.
    using _LoftedOutputsMap = tbb::concurrent_hash_map<VdfId, VdfMask>;
    _LoftedOutputsMap _loftedOutputs;

    // An array of node references used to accelerate lookups into the
    // _loftedOutputs map.
    std::unique_ptr<std::atomic<uint32_t>[]> _loftedNodeRefs;

    // The size of _loftedNodeRefs, which grows to accommodate the network's 
    // maximum capacity. 
    size_t _numLoftedNodeRefs = 0;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
