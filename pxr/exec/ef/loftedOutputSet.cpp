//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/ef/loftedOutputSet.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/output.h"

PXR_NAMESPACE_OPEN_SCOPE

Ef_LoftedOutputSet::Ef_LoftedOutputSet() = default;

Ef_LoftedOutputSet::~Ef_LoftedOutputSet() = default;

void
Ef_LoftedOutputSet::RemoveAllOutputsForNode(const VdfNode &node)
{
    // If there are no lofted outputs, bail out right away.
    if (_loftedOutputs.empty()) {
        return;
    }

    // If the node is not referenced, bail out right away.
    const VdfIndex nodeIndex = VdfNode::GetIndexFromId(node.GetId());
    if (_numLoftedNodeRefs <= nodeIndex || !_loftedNodeRefs[nodeIndex]) {
        return;
    }

    TRACE_FUNCTION_SCOPE("removing lofted outputs");

    // Iterate over all outputs on the node, and remove them
    // from the set of lofted outputs.
    for (const std::pair<TfToken, VdfOutput *> &i : node.GetOutputsIterator()) {
        _loftedOutputs.erase(i.second->GetId());
    }

    // Reset the node reference count to 0 to indicate that no
    // outputs have been lofted from this node.
    _loftedNodeRefs[nodeIndex] = 0;
}

bool
Ef_LoftedOutputSet::Add(
    const VdfOutput &output,
    const VdfMask &mask)
{
    // Note, this method will be called concurrently, if the engine type is
    // a parallel engine.

    // First make sure that the output can be lofted. _Run() is responsible for
    // resizing the _loftedNodeRefs array, but we may end up here before having
    // called run (e.g. client calling GetOutputValue() on this executor.)
    // Note, we could dynamically resize _loftedNodeRefs here as long as that
    // operation is thread safe. We are not currently doing that for performance
    // reasons.
    const VdfNode &node = output.GetNode();
    const VdfIndex nodeIndex = VdfNode::GetIndexFromId(node.GetId());
    if (_numLoftedNodeRefs <= nodeIndex) {
        return false;
    }

    // Try to insert the output and mask into the set of lofted outputs.
    {
        _LoftedOutputsMap::accessor accessor;

        // If the output had previously been inserted, simply append the mask
        // and we are done here.
        if (!_loftedOutputs.insert(accessor, { output.GetId(), mask })) {
            accessor->second.SetOrAppend(mask);
            return true;
        }
    }

    // If this is the first time this output is being inserted into the map,
    // make sure to also increment the node reference count for the node that
    // owns the output.

    // Increment the reference count.
    ++_loftedNodeRefs[nodeIndex];

    // Success
    return true;
}

void
Ef_LoftedOutputSet::Remove(
    const VdfId outputId,
    const VdfId nodeId,
    const VdfMask &mask)
{
    // If the map of lofted outputs remains empty, we can bail out right away.
    if (_loftedOutputs.empty()) {
        return;
    }

    // Look at the reference count for the node owning this output. If the
    // node is not referenced, we can bail out without even looking at the map.
    const VdfIndex nodeIndex = VdfNode::GetIndexFromId(nodeId);
    if (_numLoftedNodeRefs <= nodeIndex || !_loftedNodeRefs[nodeIndex]) {
        return;
    }

    // Lookup the output in the map.
    _LoftedOutputsMap::accessor accessor;
    if (!_loftedOutputs.find(accessor, outputId)) {
        return;
    }

    // If the entire mask is being removed, simply drop the output from the map
    // and decrement the reference count for the owning node.
    if (accessor->second == mask || mask.IsEmpty()) {
        _loftedOutputs.erase(accessor);
        --_loftedNodeRefs[nodeIndex];
    }

    // If some subset of the mask is being removed, merely update the
    // stored mask.
    else {
        accessor->second -= mask;

        // If at this point the mask is all zeros, we can drop the output
        // entirely, and also decrement the reference count for the
        // node owning the output.
        if (accessor->second.IsAllZeros()) {
            _loftedOutputs.erase(accessor);
            --_loftedNodeRefs[nodeIndex];
        }
    }
}

void
Ef_LoftedOutputSet::Clear()
{
    if (_loftedOutputs.empty() && _numLoftedNodeRefs == 0) {
        return;
    }

    TRACE_FUNCTION();

    _loftedOutputs.clear();

    WorkParallelForN(_numLoftedNodeRefs, 
        [&loftedNodeRefs = _loftedNodeRefs] (size_t begin, size_t end) {
            for (size_t i = begin; i != end; ++i) {
                loftedNodeRefs[i].store(0, std::memory_order_relaxed);
            }
        });
}

void
Ef_LoftedOutputSet::Resize(const VdfNetwork &network)
{
    // This array is over-allocated to accommodate the maximum network 
    // capacity.
    const size_t newSize = network.GetNodeCapacity();
    if (newSize > _numLoftedNodeRefs) {
        auto * const newReferences = new std::atomic<uint32_t>[newSize];

        // Copy all the existing entries into the new array.
        for (size_t i = 0; i < _numLoftedNodeRefs; ++i) {
            newReferences[i].store(
                _loftedNodeRefs[i].load(
                    std::memory_order_relaxed),
                std::memory_order_relaxed);        
        }

        // Initialize the tail values in the new array. 
        for (size_t i = _numLoftedNodeRefs; i < newSize; ++i) {
            newReferences[i].store(0, std::memory_order_relaxed);
        }

        _loftedNodeRefs.reset(newReferences);
    }
    _numLoftedNodeRefs = newSize;
}

void
Ef_LoftedOutputSet::CollectLoftedDependencies(
    const VdfOutputToMaskMap &deps,
    VdfMaskedOutputVector *processedRequest) const
{
    // Resize the new invalidation request to the maximum number of elements it
    // could possibly contain. We will populate the vector in parallel.
    const size_t maxNumRequest =
        std::min(_loftedOutputs.size(), deps.size());
    processedRequest->resize(maxNumRequest);

    // Keep track of how many entries in the new invalidation request have been
    // populated in parallel. We will use this to later trim the tail of the
    // vector.
    std::atomic<size_t> numRequest(0);

    // For each dependent output, determine if it has been lofted, and if so
    // add it to the invalidation request.
    const std::unique_ptr<std::atomic<uint32_t>[]> &loftedNodeRefs = _loftedNodeRefs;
    const size_t numLoftedNodeRefs = _numLoftedNodeRefs;
    const _LoftedOutputsMap &loftedOutputs = _loftedOutputs;

    WorkParallelForN(deps.bucket_count(),
        [&deps, &loftedNodeRefs, numLoftedNodeRefs, 
            &loftedOutputs, &numRequest, processedRequest]
        (size_t b, size_t e) {
            for (size_t i = b; i != e; ++i) {
                VdfOutputToMaskMap::const_local_iterator it = deps.begin(i);
                VdfOutputToMaskMap::const_local_iterator end = deps.end(i);
                for (; it != end; ++it) {
                    const VdfOutput *output = it->first;

                    // Is this output's node even referenced in the set of
                    // lofted outputs?
                    const VdfIndex idx = VdfNode::GetIndexFromId(
                        output->GetNode().GetId());
                    if (numLoftedNodeRefs <= idx || !loftedNodeRefs[idx]) {
                        continue;
                    }

                    // If this output has been lofted, add it to the
                    // invalidation request.
                    _LoftedOutputsMap::const_accessor accessor;
                    if (loftedOutputs.find(accessor, output->GetId())) {
                        (*processedRequest)[numRequest.fetch_add(1)] =
                            VdfMaskedOutput(
                                const_cast<VdfOutput*>(output),
                                it->second & accessor->second);
                    }
                }
            }
        });

    processedRequest->resize(numRequest.load(std::memory_order_relaxed));
}

PXR_NAMESPACE_CLOSE_SCOPE
