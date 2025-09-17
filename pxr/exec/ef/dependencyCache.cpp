//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/ef/dependencyCache.h"

#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"
#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/nodeSet.h"
#include "pxr/exec/vdf/sparseOutputTraverser.h"

#include <tbb/enumerable_thread_specific.h>

PXR_NAMESPACE_OPEN_SCOPE

EfDependencyCache::EfDependencyCache(
    PredicateFunction predicate) :
    _predicate(predicate)
{
}

EfDependencyCache::~EfDependencyCache() = default;

const VdfOutputToMaskMap &
EfDependencyCache::FindOutputs(
    const VdfMaskedOutputVector &outputs,
    bool updateIncrementally) const
{
    return _Find(outputs, updateIncrementally).outputDeps;
}

const std::vector<const VdfNode *> &
EfDependencyCache::FindNodes(
    const VdfMaskedOutputVector &outputs,
    bool updateIncrementally) const
{
    return _Find(outputs, updateIncrementally).nodeDeps;
}

void
EfDependencyCache::Invalidate()
{
    if (_cache.empty()) {
        return;
    }

    TRACE_FUNCTION();

    _cache.clear();
}

void
EfDependencyCache::WillDeleteConnection(
    const VdfConnection &connection)
{
    if (_cache.empty()) {
        return;
    }

    TRACE_FUNCTION();

    // Invalidate all traversals which followed this connection by
    // traversing the source, as well as the target nodes.
    for (_Cache::iterator it = _cache.begin(); it != _cache.end(); ++it) {
        _Entry &entry = it->second;
        if (!entry.IsValid()) {
            continue;
        }

        // If the traversal contains both the source and target nodes, we always
        // have to entirely invalidate the entry, since we can't incrementally
        // update in response to deleting connections. Otherwise, the connection
        // to be deleted isn't in the cached traversal and therefore we can
        // ignore it.
        if (entry.ContainsNode(connection.GetSourceNode()) &&
            entry.ContainsNode(connection.GetTargetNode())) {
            entry.Invalidate();
        }
    }
}

void
EfDependencyCache::DidConnect(const VdfConnection &connection)
{
    if (_cache.empty())
        return;

    TRACE_FUNCTION();

    // Record the new connection with each traversal, which includes
    // the source node.
    //
    // We will also have to check whether the source output was included
    // in the traversal, and whether the connection mask overlaps the
    // traversed mask at said output, but we can delay this relatively
    // expensive check until later.
    //
    // Entries which are not incrementally updated will be dropped here.
    for (_Cache::iterator it = _cache.begin(); it != _cache.end(); ++it) {
        _Entry &entry = it->second;
        if (!entry.IsValid()) {
            continue;
        }

        // If the traversal doesn't contain the source node, then the addition
        // of this connection can't affect the cached traversal.
        if (!entry.ContainsNode(connection.GetSourceNode())) {
            continue;
        }

        // If this entry is not being incrementally updated, invalidated it
        // entirely.
        if (!entry.updateIncrementally) {
            entry.Invalidate();
            continue;
        }

        // Otherwise, we can incrementally update the cached traversal for the
        // added connection. We do that update lazily, so here we just record
        // information that identifies the new connection.
        if (connection.GetMask().IsAnySet()) {
            entry.newConnections.emplace_back(
                connection.GetSourceNode().GetId(),
                connection.GetSourceOutput().GetName(),
                connection.GetTargetNode().GetId(),
                connection.GetTargetInput().GetName());
        }
    }
}


const EfDependencyCache::_Entry &
EfDependencyCache::_Find(
    const VdfMaskedOutputVector &outputs,
    bool updateIncrementally) const
{
    TRACE_FUNCTION();

    // Look up the entry in the cache and return the cached result, if any.
    VdfMaskedOutputVector sortedOutputs(outputs);
    VdfSortAndUniqueMaskedOutputVector(&sortedOutputs);

    const _Cache::iterator it = _cache.find(sortedOutputs);
    if (it != _cache.end()) {
        _Entry &entry = it->second;

        // If the entry is invalid, remove it from the map.
        if (!entry.IsValid()) {
            _cache.erase(it);
        }

        // Otherwise, return the cached entry if there aren't any new
        // connections to process. (If we have new connections, we need to
        // incrementally update the cached traversal.)
        else if (it->second.newConnections.empty()) {
            return it->second;
        }
    }

    // Cache miss: We need to populate the cache for the given outputs.
    return _PopulateCache(sortedOutputs, updateIncrementally);
}

const EfDependencyCache::_Entry &
EfDependencyCache::_PopulateCache(
    const VdfMaskedOutputVector& sortedOutputs, 
    bool updateIncrementally) const
{
    TRACE_FUNCTION();
    TfAutoMallocTag2 tag("Vdf", "EfDependencyCache::_PopulateCache");

    // Insert a new entry into the cache map.
    const auto [iterator, emplaced] = _cache.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(sortedOutputs),
        std::forward_as_tuple(updateIncrementally));
    _Entry &entry = iterator->second;

    // If the outputs is empty, bail out and return the empty set of
    // dependencies.
    if (sortedOutputs.empty()) {
        return entry;
    }

    // Make sure the bitset of referenced nodes, and the vector of number of
    // outputs per node is sufficiently large.
    const VdfNetwork *network = VdfGetMaskedOutputVectorNetwork(sortedOutputs);
    const size_t nodeCapacity = network->GetNodeCapacity();
    if (entry.nodeRefs.GetSize() < nodeCapacity) {
        entry.nodeRefs.ResizeKeepContent(nodeCapacity);

        // We only need to track the number of outputs per node for
        // incrementally updated traversals.
        if (entry.updateIncrementally) {
            entry.nodeNumOutputs.resize(nodeCapacity, 0);
        }
    }

    // Can we extend the existing traversal by building a partial outputs?
    // We can partially traverse any new connections, but only if the traversal
    // is not fully invalid (as determined above).
    if (!entry.newConnections.empty()) {
        _TraversePartially(network, &entry);
    }

    // Start a full re-traversal if necessary.
    else {
        _Traverse(sortedOutputs, &entry);
    }

    // Return the newly populated entry.
    return entry;
}

void
EfDependencyCache::_Traverse(
    const VdfMaskedOutputVector &outputs,
    _Entry *entry) const
{
    // Bail out if there is nothing to do.
    if (outputs.empty()) {
        return;
    }

    TRACE_FUNCTION();

    // Include the nodes from the outputs in the bitset of referenced nodes,
    // and initialize the number of outputs on these nodes.
    TF_FOR_ALL (it, outputs) {
        const VdfNode &node = it->GetOutput()->GetNode();
        const VdfIndex nodeIndex = VdfNode::GetIndexFromId(node.GetId());
        entry->nodeRefs.Set(nodeIndex);

        // We only need the track the number of outputs per node for
        // incrementally updated traversals.
        if (entry->updateIncrementally) {
            entry->nodeNumOutputs[nodeIndex] = node.GetNumOutputs();
        }
    }       

    // Fill the entry with outputs and masks accumulated in the traversal.
    // Note, that we use the output callback for incrementally updated 
    // traversals only.
    VdfSparseOutputTraverser::Traverse(
        outputs,
        entry->updateIncrementally
            ? std::bind(
                &EfDependencyCache::_OutputCallback,
                std::placeholders::_1,
                std::placeholders::_2, 
                std::placeholders::_3, 
                entry)
            : VdfSparseOutputTraverser::OutputCallback(),
        std::bind(
            &EfDependencyCache::_NodeCallback,
            std::placeholders::_1, 
            _predicate, 
            entry));

    // Make sure the vector of node dependencies is sorted and unique.
    std::sort(entry->nodeDeps.begin(), entry->nodeDeps.end());
    entry->nodeDeps.erase(
        std::unique(entry->nodeDeps.begin(), entry->nodeDeps.end()),
        entry->nodeDeps.end());
}

void
EfDependencyCache::_TraversePartially(
    const VdfNetwork *network,
    _Entry *entry) const
{
    TRACE_FUNCTION();

    // Make sure that this is an incrementally updated traversal.
    TF_VERIFY(
        entry->updateIncrementally &&
        entry->nodeNumOutputs.size() == entry->nodeRefs.GetSize());

    // The outputs for the new dependencies may contain duplicate outputs!
    VdfMaskedOutputVector dependencies;

    // Gather the dependencies for the partial traversal across the
    // new connections.
    VdfNodeSet extendedNodes, skipTraversal;
    for (const _Entry::_Connection &connection : entry->newConnections) {
        const VdfConnection *const c = connection.GetConnection(network);
        if (!c) {
            continue;
        }

        // Gather the dependencies across the new connection, if the source
        // output is included in the original traversal.
        const bool sourceOutputVisited = 
            _GatherDependenciesForNewConnection(entry, *c, &dependencies);
        // If the node at the source side of the connection has been extended
        // with new outputs, we must re-gather all the dependencies of that
        // node. The new outputs may become part of this traversal, even
        // though they were not before.
        if (!sourceOutputVisited) {
            const VdfNode &sourceNode = c->GetSourceNode(); 
            const VdfIndex sourceNodeIndex =
                VdfNode::GetIndexFromId(sourceNode.GetId());
            const size_t numOutputs = entry->nodeNumOutputs[sourceNodeIndex];
            if (sourceNode.GetNumOutputs() != numOutputs) {
                // When the new output has no dependencies (ie. a sharing node
                // output that doesn't depend on any interface inputs), we don't
                // need to update anything. 
                if (!extendedNodes.Contains(sourceNode)) {
                    const VdfMaskedOutput &sourceMaskedOutput =
                        c->GetSourceMaskedOutput();
                    if (sourceNode.ComputeInputDependencyMasks(
                        sourceMaskedOutput, false /* skipAssoc. */).empty()) {
                        skipTraversal.Insert(sourceNode);
                        continue;
                    }
                    extendedNodes.Insert(sourceNode);
                    skipTraversal.Remove(sourceNode);
                }
            }
        }
    }
    // Reset entry's nodeNumOutputs[] for all nodes that were extended but
    // didn't add any input dependencies.  Usually, this is an empty node set.
    if (!skipTraversal.IsEmpty()) {
        for (const size_t nodeIndex : skipTraversal) {
            const VdfNode *node = network->GetNode(nodeIndex);
            entry->nodeNumOutputs[nodeIndex] = node->GetNumOutputs();
        }
    }
    // Gather dependencies for all extended nodes. We do this in one vectorized
    // go because we may end up adding tons of new connections on the same
    // extended node(s). Computing  dependencies one-by-one would be very
    // expensive (e.g. sharing nodes).
    for (const size_t nodeIndex : extendedNodes) {
        const VdfNode *node = network->GetNode(nodeIndex);
        _GatherDependenciesForExtendedNode(*entry, *node, &dependencies);
    }

    // The partial outputs from which to start this new traversal in
    // order to extend the existing one.
    VdfMaskedOutputVector sortedoutputs;

    // The outputs with dependencies may contain duplicate outputs, so make
    // sure to filter those in order to speed up the actual traversal.
    if (dependencies.size() > 1) {
        TRACE_FUNCTION_SCOPE("sorting outputs");

        // Gather all unique outputs and accumulate their masks.
        TfHashMap<VdfOutput *, VdfMask, TfHash> uniqueOutputs;
        TF_FOR_ALL (it, dependencies) {
            uniqueOutputs[it->GetOutput()].SetOrAppend(it->GetMask());
        }

        // Add all the unique outputs and accumulated masks to the outputs
        // for the partial traversal.
        TF_FOR_ALL (it, uniqueOutputs) {
            sortedoutputs.push_back(VdfMaskedOutput(it->first, it->second));
        }
    }

    // Extend the existing traversal with new dependencies picked up
    // through the sortedoutputs. If the partial outputs remains empty,
    // use the gathered dependencies instead.
    _Traverse(sortedoutputs.empty() ? dependencies : sortedoutputs, entry);

    // Clear the vector of newly added connections. We do this after the
    // traversal, since the callbacks will use the state of the newConnections
    // vector as an indication of whether the traversal is extending an
    // existing one.
    entry->newConnections.clear();
}

bool
EfDependencyCache::_GatherDependenciesForNewConnection(
    _Entry *entry,
    const VdfConnection &connection,
    VdfMaskedOutputVector *dependencies) const
{
    // Find the traversed mask, if the output was included in the
    // existing traversal.
    VdfOutputToMaskMap::const_iterator ref =
        entry->outputRefs.find(&connection.GetSourceOutput());

    // If the output is not included in the existing traversal, bail out.
    if (ref == entry->outputRefs.end()) {
        return false;
    }

    // If the source output was included in the existing traversal, make
    // sure that the mask of the new connection overlaps with the
    // traversed mask at the source output. If the cached mask is empty, it
    // means there was no connection on that output the last time we traversed
    // it, so we weren't able to infer a mask; in that case, we always treat the
    // output as if its mask was an all-ones mask the same size as the
    // connection's mask.
    if (ref->second.IsEmpty() || ref->second.Overlaps(connection.GetMask())) {

        // The node target by the connection is part of the traversal, but
        // the traversal outputs will be built for the target node's outputs.
        // Hence, we must include the target node by invoking the predicate.
        _predicate(
            connection.GetTargetNode(), &entry->outputDeps, &entry->nodeDeps);

        // Collect all the outputs dependent on the traversed subset of
        // the connection and source output masks, and include those
        // outputs in the partial outputs. If the cached mask is empty, ignore
        // it and only use the new mask we got from the connection.
        const VdfMask &inputMask = ref->second.IsEmpty()
            ? connection.GetMask() : ref->second & connection.GetMask();
        connection.GetTargetNode().ComputeOutputDependencyMasks(
            connection, inputMask, dependencies);
    }

    // Return true to indicate that the source output is included in the
    // existing traversal.
    return true;
}

void
EfDependencyCache::_GatherDependenciesForExtendedNode(
    const _Entry &entry,
    const VdfNode &node,
    VdfMaskedOutputVector *dependencies) const
{
    TRACE_FUNCTION();

    // Maintain a thread-local vector of discovered dependencies. The
    // thread-locals will later be combined into the result vector.
    tbb::enumerable_thread_specific<VdfMaskedOutputVector> threadDeps;

    // For each one of the connections, on each one of the inputs, get the
    // source output and restart the traversal from there. First, iterate over
    // all the inputs on the node in parallel...
    const VdfNode::InputMapIterator inputs = node.GetInputsIterator();
    WorkParallelForEach(
        inputs.begin(), inputs.end(),
        [&entry, &node, &threadDeps](const std::pair<TfToken, VdfInput *> &i){
            // Iterate over all the connections on this input in parallel.
            const VdfConnectionVector &connections = i.second->GetConnections();
            WorkParallelForN(
                connections.size(),
                [&entry, &node, &threadDeps, &connections](size_t f, size_t l){
                    // Get the thread-local dependencies once per task.
                    VdfMaskedOutputVector &localDeps = threadDeps.local();

                    // For every connection handled by this task...
                    for (size_t i = f; i != l; ++i) {
                        // Get the source output from the input connection.
                        const VdfConnection *connection = connections[i];
                        const VdfOutput &output = connection->GetSourceOutput();

                        // If the source node on the input connection isn't part
                        // of the existing traversal, we can bail out right
                        // away.
                        if (!entry.ContainsNode(output.GetNode())) {
                            continue;
                        }

                        // Determine if the source output is part of the
                        // existing traversal.
                        VdfOutputToMaskMap::const_iterator ref =
                            entry.outputRefs.find(&output);

                        // If the source output is part of the existing
                        // traversal, re-gather all the output dependencies on
                        // the current node.
                        if (ref != entry.outputRefs.end()) {
                            const VdfMask &mask =
                                ref->second.IsEmpty()
                                ? connection->GetMask()
                                : connection->GetMask() & ref->second;
                            node.ComputeOutputDependencyMasks(
                                *connection, mask, &localDeps);
                        }
                    }
            });
    });

    // Combine all thread-local dependencies into the result vector.
    threadDeps.combine_each([dependencies](const VdfMaskedOutputVector &d){
        dependencies->insert(dependencies->end(), d.begin(), d.end());
    });
}

bool
EfDependencyCache::_NodeCallback(
    const VdfNode &node,
    PredicateFunction predicate,
    _Entry *entry)
{
    const VdfIndex nodeIndex = VdfNode::GetIndexFromId(node.GetId());

    // Insert this node into the set of referenced nodes
    entry->nodeRefs.Set(nodeIndex);

    // Record the current number of outputs on this node, if the entry is
    // being incrementally updated.
    if (entry->updateIncrementally) {
        entry->nodeNumOutputs[nodeIndex] = node.GetNumOutputs();
    }

    // Call the dependency predicate.
    return predicate(node, &entry->outputDeps, &entry->nodeDeps);
}

bool
EfDependencyCache::_OutputCallback(
    const VdfOutput &output,
    const VdfMask &mask,
    const VdfInput *input,
    _Entry *entry)
{
    const bool hasConnections = !output.GetConnections().empty();

    // Insert this output as having been visited.
    //
    // If this output has no connections on it, we store an empty mask here.
    // Mask sizes are often inferred from the sizes of the masks stored on
    // connections. When there are no connections present, we can't reliably
    // determine the size of the input-to-output mask for a node.
    //
    // So here, we exercise a bit of distrust and don't cache a mask at all for
    // outputs that have nothing attached to them; instead, we cache an empty
    // mask to signify that we've seen this output but we don't actually know
    // the mask on the output that we've reached. This means we are still able
    // to provide correct answers to reachability queries that are interested
    // only in which outputs are reachable. It also means we are able to provide
    // correct handling of incremental updates: if we blindly trust the
    // traversal mask, we would end up caching 1x1 masks for outputs that would
    // produce more than 1 output when the output has no connections attached,
    // and if we incrementally update the cache when something connects up to
    // that output with a larger mask, we end up with disagreeing mask sizes in
    // the cache and we start failing axioms when we check for mask containment.
    const auto [iterator, emplaced] =
        entry->outputRefs.emplace(&output, hasConnections ? mask : VdfMask());

    // Has this output been visited before?
    if (!emplaced && hasConnections) {
        VdfMask &cachedMask = iterator->second;

        // If this is a partial traversal, bail out if this output has
        // already been visited with the given mask.
        const bool isPartialTraversal = !entry->newConnections.empty();
        if (isPartialTraversal) {
            // If the old mask is empty, it means the last time we visited this
            // node nothing was connected to its outputs.
            if (!cachedMask.IsEmpty() && cachedMask.Contains(mask)) {
                return false;
            }
        }

        if (cachedMask.IsEmpty()) {
            // If the existing mask is empty, it means the last time we visited
            // this node nothing was connected to its outputs; we replace that
            // empty mask with the mask on the connection that got us here.
            cachedMask = mask;
        } else {
            // Otherwise, we append the specified mask to the traversal mask.
            cachedMask |= mask;
        }
    } else if (!emplaced) {
        // An entry already existed, but we have no connections now. Clear out
        // the cached mask, since we can't reliably determine what the output
        // mask should be without connections.
        iterator->second = VdfMask();
    }

    // Continue the traversal.
    return true;
}

VdfConnection *
EfDependencyCache::_Entry::_Connection::GetConnection(
    const VdfNetwork *const network) const
{
    TRACE_FUNCTION();

    const VdfNode *const sourceNode =
        network->GetNodeById(sourceNodeId);
    if (!sourceNode) {
        return nullptr;
    }
    const VdfNode *const targetNode =
        network->GetNodeById(targetNodeId);
    if (!targetNode) {
        return nullptr;
    }
    const VdfOutput *const output =
        sourceNode->GetOptionalOutput(outputName);
    if (!output) {
        return nullptr;
    }
    const VdfInput *const input =
        targetNode->GetInput(inputName);
    if (!input) {
        return nullptr;
    }

    // Attempt to find the connection, starting from whichever end has the
    // fewest connections.
    if (input->GetConnections().size() < output->GetConnections().size()) {
        for (VdfConnection *const connection : input->GetConnections()) {
            if (&connection->GetSourceOutput() == output) {
                return connection;
            }
        }
    } else {
        for (VdfConnection *const connection : output->GetConnections()) {
            if (&connection->GetTargetInput() == input) {
                return connection;
            }
        }
    }
    return nullptr;
}

PXR_NAMESPACE_CLOSE_SCOPE
