//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/executorInvalidator.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/executorInterface.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/nodeProcessInvalidationInterface.h"
#include "pxr/exec/vdf/output.h"

#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/dispatcher.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/withScopedParallelism.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

void
VdfExecutorInvalidator::Invalidate(const VdfMaskedOutputVector &request)
{
    // Bail out if the request is empty. In that case, there is nothing to
    // invalidate.
    if (request.empty()) {
        return;
    }

    // If everything in the request is already invalid, we do not need to do
    // any more invalidation.
    if (_IsInvalid(request)) {
        return;
    }

    // Make sure the request is sorted, so that we can use it as a key for cache
    // lookup.
    // TODO: We could potentially improve performance by requiring the request
    //       to be in a data structure that is already guaranteed to be sorted
    //       (e.g. VdfRequest.)
    VdfMaskedOutputVector sortedRequest(request);
    VdfSortAndUniqueMaskedOutputVector(&sortedRequest);

    // Attempt to replay a previously recorded traversal. If this fails, start
    // a new traversal.
    _ReplayCache *replayCache = _GetReplayCache(sortedRequest);
    if (!_Replay(*replayCache)) {
        _Traverse(sortedRequest, replayCache);
    }
}

void
VdfExecutorInvalidator::Reset()
{
    _replayLRU.Clear();

    // We can key the dependency map off of the output index, and store the
    // output version with the keyed value. Doing so would allow us to carry
    // cached results across topological changes, and enable us to no longer
    // clear the map, here.
    _dependencyMap.clear();
}

bool
VdfExecutorInvalidator::_IsInvalid(const VdfMaskedOutputVector &request) const
{
    // Most commonly the request will only contain a single entry, so check for
    // validity of that single item here.
    if (request.size() == 1) {
        const VdfMaskedOutput &mo = request.front();
        return _executor->_IsOutputInvalid(
            mo.GetOutput()->GetId(), mo.GetMask());
    }

    TRACE_FUNCTION();

    const VdfExecutorInterface &executor = *_executor;
    std::atomic<bool> isValid(false);

    // Iterate over all the entries in the request and determine whether the
    // entries are invalid. If any one of the entries is valid, we can bail out
    // immediately.
    WorkParallelForN(
        request.size(),
        [&request, &executor, &isValid](size_t b, size_t e){
            for (size_t i = b; i != e; ++i) {
                // If we have already determined that any one entry is valid,
                // we can bail out immediately.
                if (isValid.load(std::memory_order_relaxed)) {
                    break;
                }

                // Determine if the entry is invalid.
                const VdfMaskedOutput &mo = request[i];
                const bool isEntryInvalid = executor._IsOutputInvalid(
                    mo.GetOutput()->GetId(), mo.GetMask());

                // If the entry is valid, set the flag and bail out.
                if (!isEntryInvalid) {
                    isValid.store(true, std::memory_order_relaxed);
                    break;
                }
            }
        });

    // If we have determined that any one entry in the request is valid, the
    // specified request is not all invalid.
    return !isValid.load(std::memory_order_relaxed);
}

VdfExecutorInvalidator::_ReplayCache *
VdfExecutorInvalidator::_GetReplayCache(const VdfMaskedOutputVector &outputs)
{
    TRACE_FUNCTION();

    // Find the replay cache in the LRU cache, and return the entry if its from
    // a cache hit.
    _ReplayCache *cache = nullptr;
    if (_replayLRU.Lookup(outputs, &cache)) {
        return cache;
    }
    
    // If the cache lookup resulted in a cache miss, make sure the fields of the
    // new (or re-used) entry are all cleared.
    cache->entries.clear();
    cache->inputs.clear();

    return cache;
}

uint32_t *
VdfExecutorInvalidator::_Visit(
    const VdfMaskedOutput &maskedOutput,
    uint32_t nextIndex)
{
    // Get the index of the visited output.
    const VdfIndex idx = VdfOutput::GetIndexFromId(
        maskedOutput.GetOutput()->GetId());

    // Make sure the array of visited outputs is appropriately sized.
    if (idx >= _visited.size()) {
        const size_t newSize = idx + 1;
        const size_t newCapacity = newSize + (newSize / 2);
        _visited.resize(newCapacity, _Visited(_timestamp));
    }

    // Get the visited entry.
    _Visited *visited = &_visited[idx];

    // If the output has not been visited during this round of invalidation,
    // record it as visited.
    const VdfMask &mask = maskedOutput.GetMask();
    if (visited->mask.IsEmpty() || visited->timestamp != _timestamp) {
        visited->timestamp = _timestamp;
        visited->index = nextIndex;
        visited->mask = mask;
    }

    // If the output has already been visited during this round of invalidation,
    // but the mask has not been visited, record the mask as visited.
    else if (!visited->mask.Contains(mask)) {
        visited->mask = _maskMemoizer.Append(visited->mask, mask);
    }

    // The output has already been visited.
    else {
        return nullptr;
    }

    // Visit the output.
    return &visited->index;
}

void
VdfExecutorInvalidator::_Traverse(
    const VdfMaskedOutputVector &request,
    _ReplayCache *replayCache)
{
    TRACE_FUNCTION();
    TfAutoMallocTag2 tag("Vdf", "VdfExecutorInvalidator::_Traverse");

    // This is a new round of invalidation, so increment the timestamp.
    ++_timestamp;

    // Clear the replay cache. We will record this new traversal for replaying.
    replayCache->entries.clear();

    // Keep track of all the inputs that need to be processed after the
    // traversal.
    VdfNodeToInputPtrVectorMap inputs;

    // Maintain a stack for the traversal, and a priority queue for the pool
    // outputs.
    _OutputStack stack(request);
    _PoolQueue queue;

    // Traverse while there is work to do.
    while (!stack.empty() || !queue.empty()) {
        // Process everything on the stack, until we can no longer make
        // progress.
        while (!stack.empty()) {
            VdfMaskedOutput top = stack.back();
            stack.pop_back();

            // Figure out whether this output should be visited.
            const uint32_t nextVisitIdx = replayCache->entries.size();
            if (uint32_t *visitIdx = _Visit(top, nextVisitIdx)) {
                // Visit the output and invalidate it.
                const bool invalidated = 
                    _TraverseOutput(top, &stack, &queue, &inputs);

                // If this is the first time this output has been visited,
                // record the invalidation for the replay cache.
                const VdfMask &topMask = top.GetMask();
                if (*visitIdx >= nextVisitIdx) {
                    replayCache->entries.emplace_back(top.GetOutput());
                    replayCache->entries.back().masks[invalidated] = topMask;
                } 

                // If this output has been visited before, augment the existing
                // entry in the replay cache. Multiple entries for the same
                // output can lead to race conditions when later replaying the
                // traversal in parallel!
                else {
                    VdfMask *replayMask =
                        &replayCache->entries[*visitIdx].masks[invalidated];
                    *replayMask = replayMask->IsEmpty()
                        ? topMask
                        : _maskMemoizer.Append(*replayMask, topMask);
                }
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

    // Now, process all relevant inputs encountered during the traversal.
    VdfNodeProcessInvalidationInterface::ProcessInvalidation(_executor, inputs);

    // Store the processed inputs in the replay cache.
    replayCache->inputs = std::move(inputs);
}

bool
VdfExecutorInvalidator::_TraverseOutput(
    const VdfMaskedOutput &maskedOutput,
    _OutputStack *stack,
    _PoolQueue *queue,
    VdfNodeToInputPtrVectorMap *inputs)
{
    // Invalidate the output. Stop the traversal if _InvalidateOutput returns
    // false, i.e. the output is already invalid for the given mask.
    const VdfOutput &output = *maskedOutput.GetOutput();
    const VdfMask &mask = maskedOutput.GetMask();
    if (!_executor->_InvalidateOutput(output, mask)) {
        return false;
    }

    // Retrieve the dependencies for this output and mask.
    const _Dependencies &dependencies = _GetDependencies(maskedOutput);

    // Populate the stack with all the dependent non-pool outputs.
    stack->insert(
        stack->end(),
        dependencies.outputs.begin(), dependencies.outputs.end());

    // Populate the queue with all the dependent pool outputs.
    for (const _PoolDependency &poolDependency : dependencies.poolOutputs) {
        std::pair<_PoolQueue::iterator, bool> result =
            queue->insert(
                {poolDependency.poolChainIndex, poolDependency.maskedOutput});

        // If there is already an entry for the dependent pool output, we need
        // to append the traversal mask to the queued output.
        if (!result.second) {
            VdfMaskedOutput *queuedOutput = &result.first->second;
            queuedOutput->SetMask(_maskMemoizer.Append(
                queuedOutput->GetMask(),
                poolDependency.maskedOutput.GetMask()));
        }
    }

    // Populate the list of inputs to process after the traversal, if any.
    // The list is bucketed by node. Chances are that most of the inputs here
    // will be on the same node, so let's remember the node we are currently
    // populating inputs for, such that we can avoid repeated hash map lookups.
    const VdfNode *currentNode = nullptr;
    VdfInputPtrVector *currentInputs = nullptr;
    for (const VdfInput *input : dependencies.inputs) {
        const VdfNode *node = &input->GetNode();
        if (currentNode != node) {
            currentNode = node;
            currentInputs = &(*inputs)[node];
        }
        currentInputs->push_back(input);
    }

    return true;
}

bool
VdfExecutorInvalidator::_Replay(const _ReplayCache &replayCache)
{
    // If there is no replay cache there is nothing to replay.
    const std::vector<_ReplayEntry> &entries = replayCache.entries;
    if (entries.empty()) {
        return false;
    }

    VdfExecutorInterface *executor = _executor;
    std::atomic<bool> replayable(true);

    // First, let's figure out if we can even replay this cached invalidation
    // traversal. We can only do so if every entry that has previously been
    // invalidated is now valid again. Otherwise the traversal path could be
    // different and that means we have to start a full-fledged traversal.
    {
        TRACE_FUNCTION_SCOPE("validating cache");

        WorkParallelForN(
            entries.size(),
            [executor, &entries, &replayable] (size_t b, size_t e) {
                for (size_t i = b; i != e; ++i) {
                    // If we have already figured out that we can't replay,
                    // there is no point in continuing on.
                    if (!replayable.load(std::memory_order_relaxed)) {
                        break;
                    }

                    // Check if the output has been invalidated in the cached
                    // traversal, and if it is currently valid again. If that's
                    // not the case we can't replay the cached traversal.
                    const VdfOutput *output = entries[i].output;
                    const VdfMask *masks = entries[i].masks;

                    // We expect the output to already be invalid for the mask
                    // with which the invalidation previously returned false.
                    const VdfId outputId = output->GetId();
                    if (!masks[0].IsEmpty() &&
                            !executor->_IsOutputInvalid(
                                outputId, masks[0])) {
                        replayable.store(false, std::memory_order_relaxed);
                        break;
                    }

                    // We expect the output to still be valid for the mask
                    // with which the invalidation previously returned true.
                    if (!masks[1].IsEmpty() &&
                            executor->_IsOutputInvalid(
                                outputId, masks[1])) {
                        replayable.store(false, std::memory_order_relaxed);
                        break;
                    }
                }
            });
    }

    // If we have determined that the traversal cannot be replayed, we need to
    // bail out.
    if (!replayable.load(std::memory_order_relaxed)) {
        return false;
    }

    // Second, let's replay the actual traversal and invalidate all the outputs
    // that were previously invalidated. At this point we no longer need to
    // worry about topology or traversal path and can simply put the output
    // buffers back into the invalid state as quickly as possible.
    {
        TRACE_FUNCTION_SCOPE("invalidating outputs");

        WorkWithScopedParallelism([&]() {
            WorkDispatcher dispatcher;
            // Invalidate all the outputs in parallel.
            dispatcher.Run([executor, &entries]() {
                WorkParallelForN(
                    entries.size(),
                    [executor, &entries] (size_t b, size_t e) {
                        for (size_t i = b; i != e; ++i) {
                            // Invalidate the output with both
                            // masks. Note that the overwhelming
                            // majority of outputs will either have the
                            // first, or second mask set, but not both.
                            const VdfOutput *output = entries[i].output;
                            const VdfMask *masks = entries[i].masks;

                            if (!masks[0].IsEmpty()) {
                                executor->
                                    _InvalidateOutput(*output, masks[0]);
                            }

                            if (!masks[1].IsEmpty()) {
                                executor->
                                    _InvalidateOutput(*output, masks[1]);
                            }
                        }
                    });
                });
        
            // Process invalidation for all the recorded inputs.
            const VdfNodeToInputPtrVectorMap &inputs = replayCache.inputs;
            if (!inputs.empty()) {
                dispatcher.Run([executor, &inputs](){
                        VdfNodeProcessInvalidationInterface::
                            ProcessInvalidation(executor, inputs);
                    });
            }
        });
    }

    // Success!
    return true;
}

const VdfExecutorInvalidator::_Dependencies &
VdfExecutorInvalidator::_GetDependencies(
    const VdfMaskedOutput &maskedOutput)
{
    // Find and return the cached dependencies, if any.
    _DependencyMap::iterator cachedEntryIt = _dependencyMap.find(maskedOutput);
    if (cachedEntryIt != _dependencyMap.end()) {
        return cachedEntryIt->second;
    }

    // Create a new entry if there are no cached dependencies, and compute the
    // dependencies once.
    cachedEntryIt = _dependencyMap.insert(
        {maskedOutput, _Dependencies()}).first;
    _ComputeDependencies(maskedOutput, &cachedEntryIt->second);

    // Return the newly computed dependencies.
    return cachedEntryIt->second;
}

void
VdfExecutorInvalidator::_ComputeDependencies(
    const VdfMaskedOutput &maskedOutput,
    _Dependencies *dependencies)
{
    TRACE_FUNCTION();

    const VdfOutput &output = *maskedOutput.GetOutput();
    const VdfMask &mask = maskedOutput.GetMask();

    // Look at all the outgoing connections on this output.
    VdfMaskedOutputVector nodeDependencies;
    for (const VdfConnection *connection : output.GetConnections()) {
        // Skip all connections where the connection mask is not overlapping
        // with the traversal mask. 
        const VdfMask &connectionMask = connection->GetMask();
        if (connectionMask.IsAllZeros() || !connectionMask.Overlaps(mask)) {
            continue;
        }

        // If the targeted node is one that must be processed after the
        // traversal, let's record the targeted input as a dependency.
        const VdfNode &node = connection->GetTargetNode();
        if (node.IsA<VdfNodeProcessInvalidationInterface>()) {
            dependencies->inputs.push_back(&connection->GetTargetInput());
        }

        // Compute the masked output dependencies given the connection and
        // traversal mask incoming on the targeted node.
        node.ComputeOutputDependencyMasks(*connection, mask, &nodeDependencies);

        // Iterate over all of the dependent masked outputs.
        for (const VdfMaskedOutput &dependency : nodeDependencies) {
            // If the dependent output is a pool output, retrieve it's pool
            // chain index (i.e. the priority in the priority queue), and insert
            // the index and output into the list of pool dependencies.
            if (Vdf_IsPoolOutput(*dependency.GetOutput())) {
                const VdfPoolChainIndex pci =
                    node.GetNetwork().GetPoolChainIndex(
                        *dependency.GetOutput());
                dependencies->poolOutputs.push_back({pci, dependency});
            }

            // If the dependent output is not a pool output, insert it into the
            // list of dependent non-pool outputs.
            else {
                dependencies->outputs.push_back(dependency);
            }
        }

        // Clear the list of dependent outputs for the next iteration of
        // the loop.
        nodeDependencies.clear();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
