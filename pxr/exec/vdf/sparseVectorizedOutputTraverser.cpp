//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/sparseVectorizedOutputTraverser.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/output.h"

#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"

PXR_NAMESPACE_OPEN_SCOPE

void
VdfSparseVectorizedOutputTraverser::Traverse(
    const VdfMaskedOutputVector &outputs,
    const NodeCallback &callback)
{
    if (outputs.empty()) {
        return;
    }

    TRACE_FUNCTION();

    // Start a traversal for each output in the traversal request. The
    // traversals will happen in parallel.
    WorkParallelForN(
        outputs.size(),
        [this, &outputs, &callback](size_t b, size_t e){
            for (size_t i = b; i != e; ++i) {
                const VdfMaskedOutput &maskedOutput = outputs[i];
                _Traverse(i, maskedOutput, callback);
            }
        });
}

void
VdfSparseVectorizedOutputTraverser::Invalidate()
{
    _dependencyMap.clear();
}

bool
VdfSparseVectorizedOutputTraverser::_Visit(
    const _OutputAndMask &outputAndMask,
    _VisitedOutputs *visitedOutputs)
{
    // Get the output and mask.
    const VdfOutput *output = outputAndMask.output;
    const VdfMask *mask = outputAndMask.mask;

    // Attempt to insert the output and mask into the map.
    std::pair<_VisitedOutputs::iterator, bool> result =
        visitedOutputs->insert({output, mask});

    // If the output was successfully inserted, this is the first time we are
    // visiting it.
    if (result.second) {
        return true;
    }

    // If the output has already been visited during this traversal, check if
    // we are visiting again with a subset of the visited mask. If the mask is
    // not a subset, we will visit this output again.
    // Note that we do not combine the visited masks in order to avoid expensive
    // ref-count traffic in the mask registry. We may end up visiting outputs
    // more than once.
    else if (result.first->second->Contains(*mask)) {
        return false;
    }

    // Visit the output.
    return true;
}

void
VdfSparseVectorizedOutputTraverser::_Traverse(
    size_t index,
    const VdfMaskedOutput &maskedOutput,
    const NodeCallback &callback)
{
    TRACE_FUNCTION();

    // Keep track of which outputs have been visited.
    _VisitedOutputs visitedOutputs;

    // Maintain a stack for the traversal, and a priority queue for the pool
    // outputs.
    _OutputStack stack(1, {maskedOutput.GetOutput(), &maskedOutput.GetMask()});
    _PoolQueue queue;

    // Traverse while there is work to do.
    while (!stack.empty() || !queue.empty()) {
        // Process everything on the stack, until we can no longer make
        // progress.
        while (!stack.empty()) {
            _OutputAndMask top = stack.back();
            stack.pop_back();

            // Visit the output, if it has not already been visited.
            if (_Visit(top, &visitedOutputs)) {
                _TraverseOutput(index, top, callback, &stack, &queue);
            }
        }

        // Then pick up the pool output with the highest priority. This is the
        // output highest up in the pool chain (i.e. closest to the
        // copy-to-pool nodes.)
        if (!queue.empty()) {
            _PoolQueue::iterator top = queue.begin();
            stack.push_back(top->second);
            queue.erase(top);
        }
    }
}

void
VdfSparseVectorizedOutputTraverser::_TraverseOutput(
    size_t index,
    const _OutputAndMask &outputAndMask,
    const NodeCallback &callback,
    _OutputStack *stack,
    _PoolQueue *queue)
{
    // If this is a pool output with a single output connection, we can
    // potentially take a shortcut that does not require us to do a cache
    // lookup.
    if (_TakePoolShortcut(outputAndMask, queue)) {
        return;
    }

    // Retrieve the dependencies for this output and mask.
    const _Dependencies &dependencies = _GetDependencies(outputAndMask);

    // Populate the stack with all the dependent non-pool outputs.
    for (const VdfMaskedOutput &dependency : dependencies.outputs) {
        stack->push_back({dependency.GetOutput(), &dependency.GetMask()});
    }

    // Populate the queue with all the dependent pool outputs.
    for (const _PoolDependency &poolDependency : dependencies.poolOutputs) {
        const VdfMaskedOutput &mo = poolDependency.maskedOutput;
        _QueuePoolOutput(
            poolDependency.poolChainIndex,
            {mo.GetOutput(), &mo.GetMask()},
            queue);
    }

    // Call the node callback
    if (callback) {
        for (const VdfNode *node : dependencies.nodes) {
            callback(*node, index);
        }
    }
}

void
VdfSparseVectorizedOutputTraverser::_QueuePoolOutput(
    const VdfPoolChainIndex &poolChainIndex,
    const _OutputAndMask &outputAndMask,
    _PoolQueue *queue)
{
    std::pair<_PoolQueue::iterator, bool> result = 
        queue->insert({poolChainIndex, outputAndMask});

    // If there is already an entry for the dependent pool output, we need
    // to append the traversal mask to the queued output.
    if (!result.second) {
        _OutputAndMask *queuedOutput = &result.first->second;
        queuedOutput->mask =
            &_maskMemoizer.Append(*queuedOutput->mask, *outputAndMask.mask);
    }
}

bool
VdfSparseVectorizedOutputTraverser::_TakePoolShortcut(
    const _OutputAndMask &outputAndMask,
    _PoolQueue *queue)
{
    // We can take a shortcut through the pool if this is a pool output with
    // only a single connection.
    const VdfOutput *output = outputAndMask.output;
    if (!Vdf_IsPoolOutput(*output) || output->GetConnections().size() != 1) {
        return false;
    }

    // If the connected input has an associated output that is another pool
    // output we can continue the traversal at that next pool output. The
    // dependency here is so trivial that we don't need to do a lookup in the
    // cache.
    const VdfConnection *connection = output->GetConnections().front();
    const VdfOutput *associatedOutput =
        connection->GetTargetInput().GetAssociatedOutput();
    if (!associatedOutput || !Vdf_IsPoolOutput(*associatedOutput)) {
        return false;
    }

    // Get the pool chain index of the next pool output.
    const VdfNetwork &network = connection->GetTargetNode().GetNetwork();
    const VdfPoolChainIndex pci = network.GetPoolChainIndex(*associatedOutput);
    
    // Queue up the next pool output.
    _QueuePoolOutput(
        pci,
        {const_cast<VdfOutput *>(associatedOutput), outputAndMask.mask},
        queue);

    return true;
}

const VdfSparseVectorizedOutputTraverser::_Dependencies &
VdfSparseVectorizedOutputTraverser::_GetDependencies(
    const _OutputAndMask &outputAndMask)
{
    // Find and return the cached dependencies, if any.
    VdfMaskedOutput mo(outputAndMask.output, *outputAndMask.mask);
    _DependencyMap::const_iterator it = _dependencyMap.find(mo);
    if (it != _dependencyMap.end()) {
        return it->second;
    }

    // Create a new entry if there are no cached dependencies, and compute the
    // dependencies once.
    _Dependencies dependencies;
    _ComputeDependencies(outputAndMask, &dependencies);
    it = _dependencyMap.emplace(std::move(mo), std::move(dependencies)).first;

    // Return the newly computed dependencies.
    return it->second;
}

void
VdfSparseVectorizedOutputTraverser::_ComputeDependencies(
    const _OutputAndMask &outputAndMask,
    _Dependencies *dependencies)
{
    const VdfOutput &output = *outputAndMask.output;
    const VdfMask &mask = *outputAndMask.mask;

    // Look at all the outgoing connections on this output.
    VdfMaskedOutputVector nodeDependencies;
    for (const VdfConnection *connection : output.GetConnections()) {
        // Skip all connections where the connection mask is not overlapping
        // with the traversal mask. 
        const VdfMask &connectionMask = connection->GetMask();
        if (connectionMask.IsAllZeros() || !connectionMask.Overlaps(mask)) {
            continue;
        }

        // If this node has no output connections (terminal node), keep track
        // of it so that we can later invoke the callback.
        const VdfNode &node = connection->GetTargetNode();
        if (!node.HasOutputConnections()) {
            dependencies->nodes.push_back(&node);
        }

        // Compute the masked output dependencies given the connection and
        // traversal mask incoming on the targeted node.
        node.ComputeOutputDependencyMasks(*connection, mask, &nodeDependencies);

        // Iterate over all of the dependent masked outputs.
        for (VdfMaskedOutput &dependency : nodeDependencies) {
            // If the dependent output is a pool output, retrieve it's pool
            // chain index (i.e. the priority in the priority queue), and insert
            // the index and output into the list of pool dependencies.
            if (Vdf_IsPoolOutput(*dependency.GetOutput())) {
                const VdfPoolChainIndex pci =
                    node.GetNetwork().GetPoolChainIndex(
                        *dependency.GetOutput());
                dependencies->poolOutputs.push_back(
                    {pci, std::move(dependency)});
            }

            // If the dependent output is not a pool output, insert it into the
            // list of dependent non-pool outputs.
            else {
                dependencies->outputs.push_back(std::move(dependency));
            }
        }

        // Clear the list of dependent outputs for the next iteration of
        // the loop.
        nodeDependencies.clear();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
