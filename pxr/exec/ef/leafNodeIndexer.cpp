//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/ef/leafNodeIndexer.h"

#include "pxr/exec/ef/leafNode.h"
#include "pxr/exec/vdf/connection.h"

#include "pxr/base/trace/trace.h"

#include <iterator>

PXR_NAMESPACE_OPEN_SCOPE

void
Ef_LeafNodeIndexer::Invalidate()
{
    TRACE_FUNCTION();

    _indices.clear();
    _nodes.clear();
    _freeList.clear();
}

void
Ef_LeafNodeIndexer::DidDisconnect(const VdfConnection &connection)
{
    // Bail out if the connection does not target a leaf node.
    const VdfNode &leafNode = connection.GetTargetNode();
    if (!EfLeafNode::IsALeafNode(leafNode)) {
        return;
    }

    TRACE_FUNCTION();
    TfAutoMallocTag2 tag("Ef", "Ef_LeafNodeIndexer::DidDisconnect");

    // Find the index of the targeted node.
    const VdfIndex nodeIndex = VdfNode::GetIndexFromId(leafNode.GetId());
    const auto indexIt = _indices.find(nodeIndex);
    TF_AXIOM(indexIt != _indices.end());

    // Find the leaf node index using the node index.
    const Index index = indexIt->second;
    TF_AXIOM(index != InvalidIndex);

    // The index is now unassigned.
    indexIt->second = InvalidIndex;

    // Push the index onto the free list.
    _freeList.push(index);
}

void
Ef_LeafNodeIndexer::DidConnect(const VdfConnection &connection)
{
    // Bail out if the connection does not target a leaf node.
    const VdfNode &leafNode = connection.GetTargetNode();
    if (!EfLeafNode::IsALeafNode(leafNode)) {
        return;
    }

    TRACE_FUNCTION();
    TfAutoMallocTag2 tag("Ef", "Ef_LeafNodeIndexer::DidConnect");
    
    // Find the index of the targeted node.
    const VdfIndex nodeIndex = VdfNode::GetIndexFromId(leafNode.GetId());

    // It's possible for the map to already contain an entry for this node
    // index, if the same node is being re-connected, or a new leaf node
    // aliasing a previous node index is being connected.
    // 
    // Connecting the same leaf node to multiple source outputs is not
    // supported, so we don't expect to race on emplacement.
    const auto [indexIt, emplaced] = _indices.emplace(
        std::piecewise_construct,
            std::forward_as_tuple(nodeIndex),
            std::forward_as_tuple(InvalidIndex));

    // Get the output and mask of the connected source output.
    const VdfOutput *output = &connection.GetSourceOutput();
    const VdfMask *mask = &connection.GetMask();

    // Get the leaf node index for this node. It should be unassigned at this
    // point.
    Index index = indexIt->second;
    TF_AXIOM(index == InvalidIndex);

    // If there is an entry on the free list, let's re-use that one.
    if (_freeList.try_pop(index)) {
        _nodes[index] = _LeafNode{&leafNode, output, mask};
        indexIt->second = index;
    }

    // If the free list is empty, let's append the new leaf node data entry to
    // the vector.
    else {
        const auto nodeIt = _nodes.push_back({&leafNode, output, mask});
        indexIt->second = std::distance(_nodes.begin(), nodeIt);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

