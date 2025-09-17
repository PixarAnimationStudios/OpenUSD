//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/ef/leafNodeCache.h"

#include "pxr/exec/ef/leafNode.h"

#include "pxr/base/tf/mallocTag.h"
#include "pxr/base/tf/stl.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"

#include <tbb/combinable.h>
#include <tbb/concurrent_unordered_set.h>

PXR_NAMESPACE_OPEN_SCOPE

// Predicate used for the dependency cache.
// This predicate tracks leaf node dependencies. We always cache at outputs
// directly above the leaf nodes, because those are the outputs that will
// appear in requests.
static bool
EfLeafNodeCache_DependencyPredicate(
    const VdfNode &node,
    VdfOutputToMaskMap *outputDeps,
    std::vector<const VdfNode *> *nodeDeps)
{
    // Is this node a leaf node?
    if (EfLeafNode::IsALeafNode(node)) {
        // Grab the single connection and store the output directly above
        // it in the dependencies map.
        const VdfConnection *c =
            node.GetInput(EfLeafTokens->in)->GetConnections()[0];
        (*outputDeps)[&c->GetSourceOutput()].SetOrAppend(c->GetMask());

        // Establish a node dependency.
        nodeDeps->push_back(&node);

        // We're done here.
        return false;
    }

    // Continue the dependency traversal.
    return true;
}

EfLeafNodeCache::EfLeafNodeCache() :
    _version(0),
    _cachesAreInvalid(false),
    _dependencyCache(&EfLeafNodeCache_DependencyPredicate)
{
}

const VdfOutputToMaskMap &
EfLeafNodeCache::FindOutputs(
    const VdfMaskedOutputVector &outputs,
    bool updateIncrementally)
{
    // Make sure the caches are cleared if they are invalid.
    _ClearCachesIfInvalid();

    if (const _SparseCacheEntry *e = TfMapLookupPtr(_sparseCache, outputs)) {
        return e->outputs;
    }

    return _dependencyCache.FindOutputs(outputs, updateIncrementally);
}

const std::vector<const VdfNode *> &
EfLeafNodeCache::FindNodes(
    const VdfMaskedOutputVector &outputs,
    bool updateIncrementally)
{
    // Make sure the caches are cleared if they are invalid.
    _ClearCachesIfInvalid();

    if (const _SparseCacheEntry *e = TfMapLookupPtr(_sparseCache, outputs)) {
        return e->nodes;
    }

    return _dependencyCache.FindNodes(outputs, updateIncrementally);
}

TfBits
EfLeafNodeCache::_CombineLeafNodes(
    const TfBits &outputsMask,
    const std::vector<TfBits> &leafNodes) const
{
    TRACE_FUNCTION();

    tbb::combinable<TfBits> threadSets([this](){
        return TfBits(_indexer.GetCapacity());
    });

    WorkParallelForN(
        outputsMask.GetSize(),
        [&threadSets, &outputsMask, &leafNodes](size_t b, size_t e){
            TfBits *set = &threadSets.local();
            for (size_t i = b; i != e; ++i) {
                if (outputsMask.IsSet(i)) {
                    *set |= leafNodes[i];
                }
            }
        });

    TfBits result(_indexer.GetCapacity());
    threadSets.combine_each([&result](const TfBits &set){
        result |= set;
    });

    return result;
}

const std::vector<const VdfNode *> &
EfLeafNodeCache::FindNodes(
    const VdfMaskedOutputVector &outputs,
    const TfBits &outputsMask)
{
    // Bail out if the outputs vector is empty.
    if (outputs.empty() || outputsMask.GetNumSet() == 0) {
        static std::vector<const VdfNode *> empty;
        return empty;
    }

    TRACE_FUNCTION();
    TfAutoMallocTag2 tag("Ef", "EfLeafNodeCache::FindNodes");

    // Make sure the caches are cleared if they are invalid.
    _ClearCachesIfInvalid();

    // Lookup the cached traversal, if any.
    _VectorizedCacheEntry *vectorized =
        TfMapLookupPtr(_vectorizedCache, outputs);

    // If there is no cached traversal, we have to do the traversal now. This
    // is the slow path.
    if (!vectorized) {
        vectorized = _PopulateVectorizedEntry(outputs);
    }

    // For all the outputs selected with the outputsMask, lookup the combined
    // leaf node set.
    TfBits *combinedLeafNodes = &vectorized->combinedLeafNodes[outputsMask];

    // If there is no combined leaf node set, we need to combine it now and then
    // cache the result.
    if (combinedLeafNodes->IsEmpty()) {
        *combinedLeafNodes =
            _CombineLeafNodes(outputsMask, vectorized->leafNodes);
    }

    // Build the key into the sparse cache.
    VdfMaskedOutputVector sparseKey;
    sparseKey.reserve(outputsMask.GetNumSet());
    for (size_t idx : outputsMask.GetAllSetView()) {
        sparseKey.push_back(outputs[idx]);
    }

    // Lookup the entry in the sparse cache, if any.
    _SparseCacheEntry *sparse = TfMapLookupPtr(_sparseCache, sparseKey);

    // If there is no entry in the sparse cache, create a new one.
    if (!sparse) {
        sparse = _PopulateSparseEntry(sparseKey, *combinedLeafNodes);
    }

    // Return the vector of nodes from the sparse cache.
    return sparse->nodes;
}

void
EfLeafNodeCache::Clear()
{
    // Increment the edit version.
    ++_version;

    // Caches are now cleared, but in a valid state.
    _cachesAreInvalid.store(false, std::memory_order_relaxed);

    // Clear all internal state.
    _dependencyCache.Invalidate();
    _indexer.Invalidate();
    _vectorizedCache.clear();
    _sparseCache.clear();
    _traverser.Invalidate();
}

void
EfLeafNodeCache::WillDeleteConnection(const VdfConnection &connection)
{
    // If the caches are already invalid there is no need to write to the
    // atomics again.
    if (!_cachesAreInvalid.load(std::memory_order_relaxed)) {
        // Increment the edit version.
        ++_version;

        // Internal state related to vectorized and sparse caches is invalid.
        _cachesAreInvalid.store(true, std::memory_order_relaxed);
    }

    // Propagate changes to the dependency cache and leaf node indexer.
    _dependencyCache.WillDeleteConnection(connection);
    _indexer.DidDisconnect(connection);
}

void
EfLeafNodeCache::DidConnect(const VdfConnection &connection)
{
    // If the caches are already invalid there is no need to write to the
    // atomics again.
    if (!_cachesAreInvalid.load(std::memory_order_relaxed)) {
        // Increment the edit version.
        ++_version;

        // Internal state related to vectorized and sparse caches is invalid.
        _cachesAreInvalid.store(true, std::memory_order_relaxed);
    }

    // Propagate changes to the dependency cache and leaf node indexer.
    _dependencyCache.DidConnect(connection);
    _indexer.DidConnect(connection);
}

void
EfLeafNodeCache::_ClearCachesIfInvalid()
{
    if (!_cachesAreInvalid.load(std::memory_order_relaxed)) {
        return;
    }

    TRACE_FUNCTION();

    _cachesAreInvalid.store(false, std::memory_order_relaxed);

    if (!_vectorizedCache.empty()) {
        _vectorizedCache.clear();
        _sparseCache.clear();
        _traverser.Invalidate();
    }
}

EfLeafNodeCache::_VectorizedCacheEntry *
EfLeafNodeCache::_PopulateVectorizedEntry(const VdfMaskedOutputVector &outputs)
{
    TRACE_FUNCTION();

    // Make sure to populate the cache with the result of this traversal.
    _VectorizedCacheEntry *entry = &_vectorizedCache[outputs];

    // Populate the output and node sets.
    entry->leafNodes.resize(outputs.size(), TfBits(_indexer.GetCapacity()));

    // Do the traversal.
    const Ef_LeafNodeIndexer &indexer = _indexer;
    _traverser.Traverse(
        outputs,
        [entry, &indexer](const VdfNode &node, size_t index){
            // If the visited node is a leaf node, record the leaf node
            // dependency for the relevant output in the request, as
            // denoted by the index.
            if (EfLeafNode::IsALeafNode(node)) {
                const Ef_LeafNodeIndexer::Index leafIndex =
                    indexer.GetIndex(node);
                TF_DEV_AXIOM(leafIndex != Ef_LeafNodeIndexer::InvalidIndex);
                entry->leafNodes[index].Set(leafIndex);
            }
            return true;
        });

    // Return the new entry.
    return entry;
}

EfLeafNodeCache::_SparseCacheEntry * 
EfLeafNodeCache::_PopulateSparseEntry(
    const VdfMaskedOutputVector &outputs,
    const TfBits &leafNodes)
{
    TRACE_FUNCTION();

    // Create a new entry in the sparse cache.
    _SparseCacheEntry *entry = &_sparseCache[outputs];

    // Reserve storage for the leaf nodes and outputs containers.
    const size_t numLeafNodes = leafNodes.GetNumSet();
    entry->nodes.reserve(numLeafNodes);
    entry->outputs.reserve(numLeafNodes);

    // For each leaf node, insert the node into the nodes container, and the
    // connected leaf output into the outputs container.
    for (size_t idx : leafNodes.GetAllSetView()) {
        entry->nodes.push_back(_indexer.GetNode(idx));
        entry->outputs[_indexer.GetSourceOutput(idx)].SetOrAppend(
            *_indexer.GetSourceMask(idx));
    }

    return entry;
}

PXR_NAMESPACE_CLOSE_SCOPE
