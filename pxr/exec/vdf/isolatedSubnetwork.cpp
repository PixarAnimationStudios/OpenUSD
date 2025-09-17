//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/isolatedSubnetwork.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/network.h"

#include "pxr/base/trace/trace.h"

#include <stack>

PXR_NAMESPACE_OPEN_SCOPE

std::unique_ptr<VdfIsolatedSubnetwork>
VdfIsolatedSubnetwork::IsolateBranch(
    VdfConnection *const connection,
    EditFilter canDelete)
{
    if (!connection) {
        TF_CODING_ERROR("Null connection");
        return nullptr;
    }

    VdfIsolatedSubnetwork *const isolated = new VdfIsolatedSubnetwork(
        &connection->GetTargetNode().GetNetwork());

    if (!isolated->AddIsolatedBranch(connection, canDelete)) {
        delete isolated;
        return nullptr;
    }

    isolated->RemoveIsolatedObjectsFromNetwork();

    return std::unique_ptr<VdfIsolatedSubnetwork>(isolated);
}

std::unique_ptr<VdfIsolatedSubnetwork>
VdfIsolatedSubnetwork::IsolateBranch(
    VdfNode *const node,
    EditFilter canDelete)
{
    if (!node) {
        TF_CODING_ERROR("Null node");
        return nullptr;
    }
    if (node->HasOutputConnections()) {
        TF_CODING_ERROR("Root node has output connections.");
        return nullptr;
    }

    // If we can't delete the initial node, we bail early.
    if (!canDelete(node)) {
        return nullptr;
    }

    VdfIsolatedSubnetwork *const isolated =
        new VdfIsolatedSubnetwork(&node->GetNetwork());

    if (!isolated->AddIsolatedBranch(node, canDelete)) {
        delete isolated;
        return nullptr;
    }
    
    isolated->RemoveIsolatedObjectsFromNetwork();

    return std::unique_ptr<VdfIsolatedSubnetwork>(isolated);
}

std::unique_ptr<VdfIsolatedSubnetwork>
VdfIsolatedSubnetwork::New(VdfNetwork *const network)
{
    if (!network) {
        TF_CODING_ERROR("Null network");
        return nullptr;
    }

    return std::unique_ptr<VdfIsolatedSubnetwork>(
        new VdfIsolatedSubnetwork(network));
}

bool
VdfIsolatedSubnetwork::AddIsolatedBranch(
    VdfConnection *const connection,
    EditFilter canDelete)
{
    if (!connection) {
        TF_CODING_ERROR("Null connection");
        return false;
    }
    if (&connection->GetTargetNode().GetNetwork() != _network) {
        TF_CODING_ERROR(
            "Attempt to call AddIsolatedBranch with a connection from a "
            "different network.");
        return false;
    }
    if (_removedIsolatedObjects) {
        TF_CODING_ERROR(
            "Attempt to call AddIsolatedBranch after calling "
            "RemoveIsolatedObjectsFromNetwork");
        return false;
    }

    // Collect all nodes/connections that are reachable from the input side
    // of the connection.
    _TraverseBranch(connection, canDelete);

    return true;
}

bool
VdfIsolatedSubnetwork::AddIsolatedBranch(
    VdfNode *const node,
    EditFilter canDelete)
{
    if (!node) {
        TF_CODING_ERROR("Null node");
        return false;
    }
    if (&node->GetNetwork() != _network) {
        TF_CODING_ERROR(
            "Attempt to call AddIsolatedBranch with a node from a different "
            "network.");
        return false;
    }
    if (_removedIsolatedObjects) {
        TF_CODING_ERROR(
            "Attempt to call AddIsolatedBranch after calling "
            "RemoveIsolatedObjectsFromNetwork");
        return false;
    }

    // If we can't delete the initial node, we bail early.
    if (node->HasOutputConnections() || !canDelete(node)) {
        return false;
    }

    // Collect all nodes/connections reachable from node.
    // Traverse up all input connections.
    for (VdfConnection *const c :  node->GetInputConnections()) {
        _TraverseBranch(c, canDelete);
    }
    
    _nodes.push_back(node);

    return true;
}
    
VdfIsolatedSubnetwork::VdfIsolatedSubnetwork(VdfNetwork *network)
: _network(network)
{
    TF_VERIFY(_network);
}

VdfIsolatedSubnetwork::~VdfIsolatedSubnetwork()
{
    TRACE_FUNCTION();

    // Make sure isolated objects get removed from the network before we delete
    // them.
    if (!_removedIsolatedObjects) {
        RemoveIsolatedObjectsFromNetwork();
    }

    for (VdfConnection *const c : _connections) {
        _network->_DeleteConnection(c);
    }

    for (VdfNode *const n : _nodes) {
        _network->_DeleteNode(n);
    }
}

bool
VdfIsolatedSubnetwork::_CanTraverse(
    const VdfNode &sourceNode,
    EditFilter canDelete)
{
    if (!canDelete(&sourceNode)) {
        return false;
    }

    // Find or emplace an entry in the map where we store the number of
    // remaining unisolated output connections for each visited node.
    const auto [it, emplaced] =
        _unisolatedOutputConnections.try_emplace(
            VdfNode::GetIndexFromId(sourceNode.GetId()));
    int& count = it.value();
    if (emplaced) {
        for (const auto &[_, output] : sourceNode.GetOutputsIterator()) {
            count += output->GetNumConnections();
        }
    }

    // Decrement to account for the output connection we just traversed to get
    // to this node. If the new count is zero, the node is isolated and we can
    // continue traversing.
    --count;
    TF_VERIFY(count >= 0);
    return count == 0;
}

void
VdfIsolatedSubnetwork::_TraverseBranch(
    VdfConnection *const connection,
    EditFilter canDelete)
{
    TRACE_FUNCTION();

    std::stack<VdfConnection*> stack;
    stack.push(connection);

    while (!stack.empty()) {
        VdfConnection *const currentConnection = stack.top();
        stack.pop();

        // Mark this connection as visited.
        const bool connectionInserted =
            _connections.insert(currentConnection).second;
        if (!connectionInserted) {
            continue;
        }
    
        VdfNode &sourceNode = currentConnection->GetSourceNode();
        if (!_CanTraverse(sourceNode, canDelete)) {
            continue;
        }

        // Once _CanTraverse returns true to indicate that a node is isolated,
        // we will never re-visit that node again.
        _nodes.push_back(&sourceNode);

        // Push the connections onto the stack in reverse order, so that
        // on the next iteration, the first connection is the top of the
        // stack and gets picked up first.
        const VdfConnectionVector inputConnections =
            sourceNode.GetInputConnections();
        auto rit = inputConnections.crbegin();
        const auto rend = inputConnections.crend();
        for (; rit != rend; ++rit) {
            stack.push(*rit);
        }
    }
}

void
VdfIsolatedSubnetwork::RemoveIsolatedObjectsFromNetwork()
{
    TRACE_FUNCTION();

    // First, remove all connections. This happens before any nodes are removed
    // to match the order in which the VdfNetwork sends out deletion notices.
    for (VdfConnection *const c : _connections) {
        _network->_RemoveConnection(c);
    }

    // Remove all nodes from the network so that the network is in a consistent
    // state. Note that the nodes are not deleted.
    for (VdfNode *const n : _nodes) {
        _network->_RemoveNode(n);
    }

    _removedIsolatedObjects = true;
}

PXR_NAMESPACE_CLOSE_SCOPE
