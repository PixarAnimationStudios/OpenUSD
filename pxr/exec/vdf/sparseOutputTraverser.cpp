//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/sparseOutputTraverser.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/mask.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/poolChainIndexer.h"

#include "pxr/base/tf/bits.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/trace/trace.h"

#include <iostream>
#include <set>
#include <tuple>

PXR_NAMESPACE_OPEN_SCOPE

// Compile Time Settings
#define _TRAVERSAL_TRACING          0
#define _DEBUG_CACHED_TRAVERSAL     0

/////////////////////////////////////////////////////////////////////////////

namespace {

// A type used to represent an output in a priority queue.
class VdfSparseOutputTraverser_PrioritizedOutput
{
public:
    VdfSparseOutputTraverser_PrioritizedOutput(
        const VdfOutput *output,
        int              parentCacheIndex,
        const VdfMask   &dependencyMask)
    :   _output(output), 
        _parentCacheIndices({parentCacheIndex}),
        _dependencyBits(dependencyMask.GetBits())
    {}

    // Returns the output.
    const VdfOutput *GetOutput() const {
        return _output;
    }

    // Returns the accumulated dependency bits.
    const VdfMask::Bits &GetDependencyBits() const {
        return _dependencyBits;
    }

    // Extends this prioritized output with \p dependencyMask and 
    // \p parentCacheIndex.
    void Extend(const VdfMask &dependencyMask, int parentCacheIndex) {
        _dependencyBits |= dependencyMask.GetBits();
        _parentCacheIndices.push_back(parentCacheIndex);
    }

    // Returns the parent cache indicies.
    const std::vector<int> &GetParentCacheIndices() const {
        return _parentCacheIndices;
    }

private:

    const VdfOutput  *_output;

    // The (accumulated) parent cache indicies.
    std::vector<int>  _parentCacheIndices;

    // The (accumulated) dependency mask.
    VdfMask::Bits _dependencyBits;
};

using _PrioritizedOutput = class VdfSparseOutputTraverser_PrioritizedOutput;

// A map from pool chain index to prioritized output, used to ensure that we
// process outputs in their order in the pool chain.
//
// Note that using a std::map<> gives us the _PrioritizedOutputs sorted by
// the pool chain index.
//
using _PrioritizedOutputMap = std::map<VdfPoolChainIndex, _PrioritizedOutput>;


// This struct is used as a stack frame during traversals.
struct VdfSparseOutputTraverser_TraversalStackFrame
{
    VdfSparseOutputTraverser_TraversalStackFrame(
        const VdfOutput *output_, const VdfMask &mask_,
        int parentCacheIndex_, const VdfInput *input_)
    : output(output_),
      mask(mask_),
      parentCacheIndex(parentCacheIndex_),
      incomingInput(input_) {}

    // The output to traverse from.
    const VdfOutput *output;

    // The dependency mask.
    VdfMask          mask;

    // The parent of this stack frame (used for cached traversal)
    int              parentCacheIndex;

    // The input through which we reached the output.  May be null.
    const VdfInput *incomingInput;
};

using _TraversalStackFrame = VdfSparseOutputTraverser_TraversalStackFrame;

// A vector of traversal stack frames, used as the stack during a traversal.
using _TraversalStack = std::vector<_TraversalStackFrame>;


// Type used to identify the masks that have already been visited for
// traversed connections.
using _VisitedConnections =
    TfHashMap<const VdfConnection *, VdfMask::Bits, TfHash>;

// This struct is used to represent the core state of a sparse traversal.
struct VdfSparseOutputTraverser_TraversalState
{
    _VisitedConnections visitedConnections;
    _PrioritizedOutputMap prioritizedOutputs;
    _TraversalStack stack;
};

using _TraversalState = VdfSparseOutputTraverser_TraversalState;

} // anonymous namespace


// Class used to keep transient state of a sparse traversal.
class VdfSparseOutputTraverser::_TraversalHelper
{
  public:
    
    _TraversalHelper(
        const VdfMaskedOutputVector &outputs,
        const OutputCallback &outputCallback,
        const VdfNodeCallback &nodeCallback);

    ~_TraversalHelper();

    // Performs a cached traversal for the given \p state.
    //
    // Extends \p cache as needed.
    void CachedTraversal(
        _TraversalState *state,
        _Cache *cache);

    // Traverses \p output, updating \p state with downstream outputs that 
    // depend on the elements indicated by \p mask.
    void TraverseOutput(
        _TraversalState *state,
        const VdfOutput &output,
        const VdfMask &mask,
        const std::vector<int> &parentCacheIndices,
        const VdfInput *incomingInput);

  private:

    // This method figures out what nodes need to be traversed from
    // \p output, traverses them calling _nodeCallback, and records 
    // them in \p cacheEntry if it's not \c null.
    void _TraverseOutputConnections(
        _TraversalState *state,
        const VdfOutput &output,
        const VdfMask  &mask,
        _CacheEntry *cacheEntry,
        int cacheIndex);

    // Returns the size of _nodeCallbackInvocations depending on the
    // network size and the presence of a node callback.
    static size_t _GetNodeCallbackInvocationsSize(
        const VdfMaskedOutputVector &outputs,
        const VdfNodeCallback       &nodeCallback);

    // Invokes the node callback (if one was provided) for the given node
    // (unless the callback already has been invoked for the node).
    //
    void _InvokeNodeCallback(const VdfNode &node) {
        if (_nodeCallback) {
            const VdfIndex nodeIndex =
                VdfNode::GetIndexFromId(node.GetId());

            if (!_nodeCallbackInvocations.IsSet(nodeIndex)) {
                _nodeCallbackInvocations.Set(nodeIndex);
                _nodeCallback(node);
            }
        }
    }

    // Traverses \p node from \p connection, updating \p state with
    // downstream outputs that depend on the elements indicated by \p mask.
    //
    void _TraverseNode(
        _TraversalState     *state,
        const VdfNode       &node,
        const VdfConnection *connection,
        const VdfMask       &mask,
        int                  parentCacheIndex);

  private:

    // The provided output callback.
    OutputCallback _outputCallback;

    // The provided node callback.
    VdfNodeCallback _nodeCallback;

    // Pointer to current cache line, if caching is active.
    _Cache *_currentCache;

    // The cache size before doing the cached traversal (only used for
    // debugging).
    int _originalCacheSize;

    // One bit for each node in the network indicating whether or not the
    // node callback has been invoked for that node yet (to avoid redundant
    // node callback invocations).
    TfBits _nodeCallbackInvocations;
};


// Cache entry used for storing cached traversal results.
struct VdfSparseOutputTraverser::_CacheEntry
{
    _CacheEntry(
        const VdfOutput *output_,
        bool cont_,
        const VdfMask &mask_,
        const VdfInput *incomingInput_)
        : output(output_)
        , cont(cont_)
        , mask(mask_)
        , incomingInput(incomingInput_)
    {}

    // The visited output.
    const VdfOutput *output;

    // The cont bool holds the result of the output callback at the time
    // of the traversal that cached this _CacheEntry.
    bool cont;

    // The indices in _Cache that hold the child _CacheEntry objects.
    std::vector<int> childIndices;

    // The mask for VdfOutput to invalidate.
    VdfMask mask;

    // The target nodes that should be visited from this output (uses
    // a set to avoid duplicates).
    std::set<const VdfNode *> targetNodes;

    // The VdfInput by which this output was reached.  May be null
    // if this is one of the first outputs to be traversed.
    const VdfInput *incomingInput;
};


/////////////////////////////////////////////////////////////////////////////


VdfSparseOutputTraverser::VdfSparseOutputTraverser(bool enableCaching) :
    _enableCaching(enableCaching),
    _cache(0)
{
}

void
VdfSparseOutputTraverser::InvalidateAll()
{
    // Note: We don't use TfReset here, because we expect the cache being
    //       repopulated quickly.
    if (!_cache.empty()) {
        _cache.clear();
    }
    if (!_cacheHistory.empty()) {
        _cacheHistory = _CacheHistory();
    }
}

#if _DEBUG_CACHED_TRAVERSAL
static bool _less(const VdfMaskedOutput &a, const VdfMaskedOutput &b)
{
    return a.GetOutput()->GetDebugName() < b.GetOutput()->GetDebugName();
}
#endif

void
VdfSparseOutputTraverser::Traverse(
    const VdfMaskedOutputVector &outputs,
    const OutputCallback        &outputCallback,
    const VdfNodeCallback       &nodeCallback)
{
    // Use a sparse traverser with caching turned off (there's no need to
    // do any caching because the traverser is discarded after this call).
    VdfSparseOutputTraverser(/* enableCaching */ false)
        .TraverseWithCaching(outputs, outputCallback, nodeCallback);
}

VdfSparseOutputTraverser::_Cache*
VdfSparseOutputTraverser::_GetOrCreateCacheEntry(
    const VdfMaskedOutputVector &sortedOutputs)
{
    // If caching has been disabled, this method should never be called.
    TF_VERIFY((MaxRequestsBeforeEviction != 0 && _enableCaching), 
              "Attempt to create cache entry with caching disabled.");

    // Find this request in the traversal cache and insert a new
    // cache entry if this is a unique new request.
    _TraversalCache::iterator it = _cache.find(sortedOutputs);
    if (it != _cache.end()) {
        return &(it->second);
    }

    // Insert a new cache entry for this key
    int wasInserted = false;
    std::tie(it, wasInserted) = _cache.insert({sortedOutputs, _Cache()});
    TF_VERIFY(wasInserted);

    _cacheHistory.push(it);

    // Enforce the eviction policy
    if (_cacheHistory.size() > MaxRequestsBeforeEviction && 
        MaxRequestsBeforeEviction > -1 && !_cacheHistory.empty()) {
        // Retrieve the cache entry to evict from the cache history
        const _TraversalCache::iterator evictIt = _cacheHistory.front();
        _cacheHistory.pop();
    
        // Before erasing the element, make sure that we do not delete
        // the latest cache entry. This should never happen!
        if (TF_VERIFY(evictIt != it, 
                      "Cache entry has been evicted before use.")) {
            // Erase the evicted cache element
            _cache.erase(evictIt);
        }
    }

    return &(it->second);
}

void
VdfSparseOutputTraverser::TraverseWithCaching(
    const VdfMaskedOutputVector &outputs,
    const OutputCallback        &outputCallback,
    const VdfNodeCallback       &nodeCallback)
{
    TfAutoMallocTag2 tag("Vdf", TF_FUNC_NAME());

    #if _DEBUG_CACHED_TRAVERSAL
        printf("> START size: %zu %d %d\n",
            outputs.size(),
            outputCallback ? 1 : 0,
            nodeCallback   ? 1 : 0);

        VdfMaskedOutputVector outputsCopy = outputs;
        std::sort(reqCopy.begin(), reqCopy.end(), _less);

        for(size_t i=0; i<reqCopy.size(); i++)
            printf("  %zu: %p %s %s\n",
                i,
                outputsCopy[i].GetOutput(),
                outputsCopy[i].GetOutput()->GetDebugName().c_str(),
                outputsCopy[i].GetMask().GetRLEString().c_str());
    #endif

    TRACE_FUNCTION();

    #if _TRAVERSAL_TRACING
        std::cout << std::endl
                  << "Starting sparse traversal with "
                  << outputs.size()
                  << " outputs"
                  << std::endl;
    #endif

    // There is no point in traversing or chaching empty requests
    if (outputs.empty()) {
        return;
    }

    const bool useCaching = _enableCaching && outputCallback && 
        MaxRequestsBeforeEviction != 0;

    VdfMaskedOutputVector sortedOutputs = outputs;
    if (outputs.size() > 1 && useCaching) {
        VdfSortAndUniqueMaskedOutputVector(&sortedOutputs);
    }

    _TraversalState state;
    _TraversalHelper helper(sortedOutputs, outputCallback, nodeCallback);

    // Push the inital outputs and masks onto the stack.
    for (const VdfMaskedOutput &maskedOutput : outputs) {
        const VdfOutput * const output = maskedOutput.GetOutput();
        const VdfMask &mask = maskedOutput.GetMask();

        //XXX: We find that sometimes there are requests that attempt to
        //     invalidate the same VdfMaskedOutput twice. We reject those here
        //     (instead of finding the cause (for now)) so that we can take
        //     advantage of caching (see below).
        if (!state.stack.empty() &&
            state.stack.back().output == output &&
            state.stack.back().mask   == mask) {
            continue;
        }

        state.stack.emplace_back(output, mask, -1, nullptr);
    }

    // Should we attempt to cache this traversal? Note that the current usage
    // pattern (with the greatest benefit) is during mungs when a single 
    // VdfMaskedOutput is invalidated repeatedly using an outputCallback only.
    //
    // Note that the call to _CachedTraversal() will extend/modify state->stack
    // as needed.
    //
    //XXX: Also consider caching the chain of "prioritized" pool output 
    //     traversals, which in practice are quite long, and are common
    //     across many traversals.  (Andru's first crack at this yielded
    //     unexpected slowdowns we didn't have time to fully explore.)
    if (useCaching) {
        helper.CachedTraversal(
            &state, 
            _GetOrCreateCacheEntry(sortedOutputs));
    }

    // Loop while we've got work to do.
    while (!state.stack.empty() || !state.prioritizedOutputs.empty()) {

        while (!state.stack.empty()) {

            // Get the next output to process.
            //
            // Since we're popping the frame off the stack, we have to be
            // careful to copy it by value, and not just get a reference.
            //
            const _TraversalStackFrame frame = state.stack.back();
            TF_AXIOM(frame.output);
            state.stack.pop_back();

            // Process the output.
            helper.TraverseOutput(
                &state, *frame.output, frame.mask,
                std::vector<int>(1, frame.parentCacheIndex),
                frame.incomingInput);
        }

        // The stack is empty; see if we have pool outputs to process.
        if (state.prioritizedOutputs.empty()) {
            break;
        }

        // Pull the top output from the priority queue.  This works, because
        // _PrioritizedOutputMap is a std::map and thus sorted.
        const _PrioritizedOutputMap::iterator topIter =
            state.prioritizedOutputs.begin();

        const _PrioritizedOutput &top = topIter->second;

#if _TRAVERSAL_TRACING
        std::cout << "  Invalidating pool output \""
                  << top.GetOutput()->GetDebugName()
                  << "\"" << std::endl;
#endif

        // Process the output.  Pass null as the incomingInput because
        // we don't currently track the set of inputs that were traversed
        // on every occasion this prioritized output was reached.

        // Currently we have no cases (specifically not SharingNode
        // invalidation accumulation) that require this information for pool
        // outputs.

        const VdfMask dependencyMask(top.GetDependencyBits());
        const size_t numParents = top.GetParentCacheIndices().size();

        helper.TraverseOutput(
            &state, *top.GetOutput(), dependencyMask,
            top.GetParentCacheIndices(), /* incomingInput */ nullptr);

        // Remove prioritzed output.  Note that the call to
        // helper.TraverseOutput() above may have inserted more prioritized
        // outputs.  However, topIter is still valid in that case.  We used
        // to remove topIter before the call to TraverseOutput(), but if
        // we would do so, we would need to copy all data by value which
        // we want to avoid.
        //
        // However, we still make sure that the state hasn't been modified
        // in the meantime.

        TF_VERIFY(dependencyMask.GetBits() == top.GetDependencyBits());
        TF_VERIFY(numParents == top.GetParentCacheIndices().size());

        state.prioritizedOutputs.erase(topIter);
    }
}

VdfSparseOutputTraverser::_TraversalHelper::_TraversalHelper(
    const VdfMaskedOutputVector &outputs,
    const OutputCallback        &outputCallback,
    const VdfNodeCallback       &nodeCallback)
:   _outputCallback(outputCallback),
    _nodeCallback(nodeCallback),
    _currentCache(nullptr),
    _originalCacheSize(0),
    _nodeCallbackInvocations(
        _GetNodeCallbackInvocationsSize(outputs, nodeCallback))
{
}

VdfSparseOutputTraverser::_TraversalHelper::~_TraversalHelper()
{
    #if _DEBUG_CACHED_TRAVERSAL
        if (_currentCache) {
            const _Cache &cache = *_currentCache;
            const _Cache::CacheEntries &cacheEntries = cache.cacheEntries;

            for (size_t i=_originalCacheSize; i<cacheEntries.size(); i++) {

                std::string children;
                TF_FOR_ALL(j, cacheEntries[i].childIndices)
                    children += TfStringPrintf(" %d", *j);
                children += " ";

                printf("  uncached inval: %zu cont:%d children:{%s} %s %s\n",
                    i,
                    cacheEntries[i].cont,
                    children.c_str(),
                    cacheEntries[i].output->GetDebugName().c_str(),
                    cacheEntries[i].mask.GetRLEString().c_str());
            }
        }
    #endif
}

/* static */
size_t
VdfSparseOutputTraverser::_TraversalHelper::_GetNodeCallbackInvocationsSize(
    const VdfMaskedOutputVector &outputs,
    const VdfNodeCallback       &nodeCallback)
{
    // Return the size of the network if we have a node callback (and
    // something to do), otherwise return 0.
    return (nodeCallback && !outputs.empty()) ?
        VdfGetMaskedOutputVectorNetwork(outputs)->GetNodeCapacity() : 0;
}

void
VdfSparseOutputTraverser::_TraversalHelper::CachedTraversal(
    _TraversalState  *state,
    _Cache           *cache)
{
    _currentCache = cache;

    // uncomment next line result in always cache misses
    //_currentCache->cacheEntries.clear();

    // If we have nothing, then there is nothing to play back...
    const int cacheSize = _currentCache->cacheEntries.size();
    if (cacheSize == 0) {
        return;
    }

#if _DEBUG_CACHED_TRAVERSAL
    // Remember the original cache size for printing out new
    // cache entries.
    _originalCacheSize = cacheSize;
#endif

    // Wipe stack clean, to be populated with missing bits and pieces...
    state->stack.clear();

    // Start with the working set including all the root 
    // node indices in the cache entry.
    TfBits workingSet(cacheSize);
    for (const int index : _currentCache->rootIndices) {
        workingSet.Set(index);
    }

    while (workingSet.IsAnySet()) {

        const size_t i = workingSet.GetFirstSet();
        workingSet.Clear(i);

        _CacheEntry &e = _currentCache->cacheEntries[i];

        const bool cont = _outputCallback(*e.output, e.mask, e.incomingInput);
        if (cont) {

            // If we have a node callback, invoke it for all target nodes.
            if (_nodeCallback)  {
                for (const VdfNode * const node : e.targetNodes) {
                    _InvokeNodeCallback(*node);
                }
            }

            // Add all children to the working set.
            for (const int index : e.childIndices) {
                workingSet.Set(index);
            }
        }

#if _DEBUG_CACHED_TRAVERSAL
        std::string children;
        TF_FOR_ALL(j, e.childIndices)
            children += TfStringPrintf(" %d", *j);
        children += " ";

        printf("  cached inval: %zu cont:%d h:%d children:{%s}"
               " %s %s\n",
               i,
               cont,
               e.cont,
               children.c_str(),
               e.output->GetDebugName().c_str(),
               e.mask.GetRLEString().c_str());
#endif

        if (cont == e.cont) {
            continue;
        }

        // If we had recorded a continue, but encounter a stop, mark this
        // branch as skip...
        if (cont) {

            // The cached entry is marked as not cont, but we need to
            // continue.
            e.cont = true;

            // Since not cached, continue to traverse from here, but
            // don't visit again the same node.
            _TraverseOutputConnections(state, *e.output, e.mask, &e, i);
        }
    }
}

void
VdfSparseOutputTraverser::_TraversalHelper::TraverseOutput(
    _TraversalState        *state,
    const VdfOutput        &output,
    const VdfMask          &mask,
    const std::vector<int> &parentCacheIndices,
    const VdfInput         *incomingInput)
{
    TF_VERIFY(parentCacheIndices.size() >= 1);

    int myCacheIndex = -1;

    // Call the output callback if any and record a new cache node if caching.
    if (_outputCallback) {

        // Note that incomingInput is sometimes null.
        const bool cont = _outputCallback(output, mask, incomingInput);

        if (_currentCache) {

            // myCacheIndex is the parent of all downstream nodes that will
            // be visited via _TraverseOutputConnections().
            myCacheIndex = _currentCache->cacheEntries.size();

            if (TF_DEV_BUILD) {
                for (const int i : parentCacheIndices) {
                    TF_VERIFY(myCacheIndex != i);
                }
            }

            // Add new cache index that is child of all parentCacheIndices at
            // index = myCacheIndex.
            _currentCache->cacheEntries.emplace_back(
                &output, cont, mask, incomingInput);

            // If this is a root node, mark it as such in the cache entry.
            if (parentCacheIndices.size() == 1 && 
                parentCacheIndices[0] == -1) {
                _currentCache->rootIndices.push_back(myCacheIndex);
            }

            // Inform parents about the new child.
            for (const int i : parentCacheIndices) {
                if (!TF_VERIFY(i >= -1 && i < myCacheIndex)) {
                    continue;
                }

                if (i >= 0) {
                    _currentCache->cacheEntries[i].childIndices.
                        push_back(myCacheIndex);
                }
            }
        }

        // If the output callback told us not to continue, stop traversing.
        //
        // For example: The VdfExecutorInterface uses this to stop traversing
        // when it encounters an already marked as invalid output.
        if (!cont) {
            return;
        }
    }

    // Traverse the nodes connected to this output, and if we have a 
    // cache entry that we just added above, record the traversal in it.
    _TraverseOutputConnections(
        state, output, mask,
        _currentCache ? &(_currentCache->cacheEntries.back()) : nullptr,
        myCacheIndex);
}

void 
VdfSparseOutputTraverser::_TraversalHelper::_TraverseOutputConnections(
    _TraversalState *state,
    const VdfOutput &output,
    const VdfMask   &mask,
    _CacheEntry     *cacheEntry,
    int              cacheIndex)
{
    #if _TRAVERSAL_TRACING
        std::cout << "  Traversing output \""
                  << output.GetDebugName()
                  << "\" with mask = "
                  << mask.GetRLEString()
                  << std::endl;
    #endif

    for (const VdfConnection * const connection : output.GetConnections()) {

        // See if we have already visited this connection.
        _VisitedConnections::iterator visitedIt =
            state->visitedConnections.find(connection);

        // Skip this connection if its accumulated traversal
        // mask contains the current dependency mask.
        if (visitedIt != state->visitedConnections.end() &&
            visitedIt->second.Contains(mask.GetBits())) {

            // At this point, we have detected a cycle.
            continue;
        }

        // If the mask on the connection is empty, we can skip this 
        // connection.
        if (connection->GetMask().IsAllZeros()) {
            continue;
        }

        // If the dependency mask doesn't overlap the mask for this
        // connection, we can skip the connection.
        if (mask.Overlaps(connection->GetMask())) {

            // Update the accumulated traversal mask.
            if (visitedIt != state->visitedConnections.end()) {
                visitedIt->second |= mask.GetBits();
            } else {
                state->visitedConnections.insert({connection, mask.GetBits()});
            }

            // Get the node on the other end of the connection.
            const VdfNode &targetNode = connection->GetTargetNode();

            // If we have a node callback, invoke it for the target node.
            _InvokeNodeCallback(targetNode);

            // Remember the target node if we're caching.
            if (cacheEntry) {
                cacheEntry->targetNodes.insert(&targetNode);
            }

            // Traverse the target node.
            _TraverseNode(state, targetNode, connection, mask, cacheIndex);
        }
    }
}

// XXX:speculation
// I think it would be faster if VdfSpeculationNodes were handled specially
// here.  As it currently stands, I think we can end up with inefficient
// traversals because speculation nodes take us back up to a higher point
// in the pool.  It'd be better if we finished all pool traversal before
// processing speculation nodes, because that will better vectorize the
// resulting traversal.
void
VdfSparseOutputTraverser::_TraversalHelper::_TraverseNode(
    _TraversalState     *state,
    const VdfNode       &node,
    const VdfConnection *connection,
    const VdfMask       &mask,
    int                  parentCacheIndex)
{
    if (!TF_AXIOM(connection)) {
        return;
    }

    #if _TRAVERSAL_TRACING
        std::cout << "  Traversing node \""
                  << node.GetDebugName()
                  << "\" from connection \""
                  << connection->GetDebugName()
                  << "\" with mask "
                  << mask.GetRLEString()
                  << std::endl;
    #endif

    VdfMaskedOutputVector dependencies;

    // Ask the node for the dependencies.
    node.ComputeOutputDependencyMasks(*connection, mask, &dependencies);
    
    // Loop over all the dependent outputs and the nodes connected to them.
    for (const VdfMaskedOutput &dependency : dependencies) {
        if (!TF_VERIFY(dependency.GetOutput())) {
            continue;
        }
        const VdfOutput &output = *(dependency.GetOutput());
        const VdfMask &dependencyMask = dependency.GetMask();

        // If it's not a pool output, push the output onto the stack for
        // immediate processing.
        if (!Vdf_IsPoolOutput(output)) {
            state->stack.emplace_back(
                &output, dependencyMask,
                parentCacheIndex, &connection->GetTargetInput());
            continue;
        }

        // Otherwise, accumulate the pool mask in the associatedOutputs map,
        // and don't traverse the output until we're done with everything on
        // the stack.

        // The output traverser processes nodes further up the pool chain
        // first by using pool chain indices as priorities.
        const VdfPoolChainIndex poolIndex = 
            node.GetNetwork().GetPoolChainIndex(output);

        const _PrioritizedOutputMap::iterator iter =
            state->prioritizedOutputs.find(poolIndex);
        if (iter != state->prioritizedOutputs.end()) {

            // Make sure that poolIndex is computed consistently (ie. there
            // is an unique, consistent index for each output).
            if (TF_VERIFY(iter->second.GetOutput() == &output)) {

                // Extend this prioritized output and make sure it referes to
                // the same output (since we use the pool chain index as id).
                iter->second.Extend(dependencyMask, parentCacheIndex);
            }

        } else {

            // Insert this pool output into the priority queue.
            state->prioritizedOutputs.emplace(
                poolIndex,
                _PrioritizedOutput(&output, parentCacheIndex, dependencyMask));
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
