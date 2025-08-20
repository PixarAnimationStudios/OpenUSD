//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EF_LEAF_NODE_INDEXER_H
#define PXR_EXEC_EF_LEAF_NODE_INDEXER_H

///\file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/types.h"

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_vector.h>

PXR_NAMESPACE_OPEN_SCOPE

class VdfConnection;
class VdfMask;
class VdfOutput;

/// The leaf node indexer tracks leaf nodes added and removed from the network,
/// and associates each leaf node with a unique index.
/// 
/// The indexer also maintains a list of the source outputs, each individual
/// leaf node is connected to. The size of the index space is relative to the
/// number of leaf nodes, rather than all nodes in the network.
///
class Ef_LeafNodeIndexer
{
public:
    /// Data type of the index.
    ///
    using Index = uint32_t;

    /// Sentinel for an invalid index.
    ///
    static constexpr Index InvalidIndex = Index(-1);

    /// Returns the capacity of the indexer, i.e. the high water mark of
    /// tracked leaf nodes.
    ///
    size_t GetCapacity() const {
        return _nodes.size();
    }

    /// Returns an index for a given leaf \p node. Returns InvalidIndex if no
    /// such index exists.
    ///
    Index GetIndex(const VdfNode &node) const;

    /// Returns the node for a given \p index. Returns \c nullptr if no such
    /// node exists.
    ///
    const VdfNode *GetNode(Index index) const {
        return index < _nodes.size() ? _nodes[index].node : nullptr;
    }

    /// Returns the output a given leaf node \p index is sourcing data from.
    /// Returns \c nullptr if no such output exists.
    ///
    const VdfOutput *GetSourceOutput(Index index) const {
        return index < _nodes.size() ? _nodes[index].output : nullptr;
    }

    /// Returns the mask at the output a given leaf node \p index is sourcing
    /// data from. Returns \c nullptr if no such mask exists.
    ///
    const VdfMask *GetSourceMask(Index index) const {
        return index < _nodes.size() ? _nodes[index].mask : nullptr;
    }

    /// Invalidate the entire cache.
    ///
    void Invalidate();

    /// Call this to notify the cache of connections that have been deleted.
    ///
    /// \note It is safe to call DidDisconnect() and DidConnect() concurrently.
    ///
    void DidDisconnect(const VdfConnection &connection);

    /// Call this to notify the cache of newly added connections.
    /// 
    /// \note It is safe to call DidDisconnect() and DidConnect() concurrently.
    ///
    void DidConnect(const VdfConnection &connection);

private:
    // The data tracked for each leaf node.
    struct _LeafNode {
        const VdfNode *node;
        const VdfOutput *output;
        const VdfMask *mask;
    };

    // Map from VdfNode index to leaf node index. If a given node does not have
    // an index, InvalidIndex will be stored at the corresponding location.
    tbb::concurrent_unordered_map<VdfIndex, Index> _indices;

    // The tightly packed vector of leaf node data. The vector is indexed with
    // the leaf node index.
    tbb::concurrent_vector<_LeafNode> _nodes;

    // Free list of leaf node data. New indices are assigned by pulling from
    // this list first.
    tbb::concurrent_queue<Index> _freeList;
};

inline
Ef_LeafNodeIndexer::Index
Ef_LeafNodeIndexer::GetIndex(const VdfNode &node) const
{
    const auto it = _indices.find(VdfNode::GetIndexFromId(node.GetId()));
    return it != _indices.end() ? it->second : InvalidIndex;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
