//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_NETWORK_UTIL_H
#define PXR_EXEC_VDF_NETWORK_UTIL_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/input.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/types.h"

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

class VdfMaskedOutput;
class VdfOutput;

/// \name Traversal
///
/// Used by clients to traverse networks, e.g., to perform invalidation.
///
/// @{


/// Returns true, if maskedOutput is computed via speculation node.
///
/// This does an upward traversal of the network and maybe slow.
///
VDF_API
bool VdfIsSpeculating(const VdfMaskedOutput &maskedOutput);

/// Searches for \p nodeToFind via topological (ie. not sparse) input
/// connections starting at \p startNode.  Won't traverse over speculation
/// nodes.
///
/// If \p foundSpecNode is non-null it will be populated when a speculation node
/// has been found while searching for \p nodeToFind.
///
VDF_API
bool VdfIsTopologicalSourceNode(
    const VdfNode &startNode,
    const VdfNode &nodeToFind,
    bool *foundSpecNode = nullptr);

/// Returns the output that is the source of the associated input of
/// \p output, if any and NULL otherwise.  Note that if \p output does not
/// have an associated input, NULL is returned.
///
VDF_API
const VdfOutput *VdfGetAssociatedSourceOutput(const VdfOutput &output);

/// Empty callback node, does nothing.
///
VDF_API
void VdfEmptyNodeCallback(const VdfNode &);

/// Traverses nodes starting at \p startNode and moving along its inputs.
/// Calls \p callback on visited nodes.
///
/// Nodes are guaranteed to be called at most once, though there is no 
/// guarantee on the order of traversal.
///
template <class Callable>
void VdfTraverseTopologicalSourceNodes(
    const VdfNode &startNode, 
    Callable &&callback)
{
    // Keep track of the nodes we have already visited.
    TfBits visited(startNode.GetNetwork().GetNodeCapacity());

    // Maintain a stack of nodes to traverse.
    std::vector<const VdfNode *> stack(1, &startNode);

    // Keep traversing as long as there are entries on the stack.
    while (!stack.empty()) {

        // The stack top is the node we are currently looking at.
        const VdfNode *top = stack.back();
        stack.pop_back();

        // Only consider this node, if it hasn't already been visited.
        const VdfIndex idx = VdfNode::GetIndexFromId(top->GetId());
        if (visited.IsSet(idx)) {
            continue;
        }
        visited.Set(idx);

        // Invoke the callback for the node, and push the input dependencies
        // on the stack, as long as the callback returns true.
        if (std::forward<Callable>(callback)(*top)) {
            for (const std::pair<TfToken, VdfInput *> &i :
                    top->GetInputsIterator()) {
                for (const VdfConnection *c : i.second->GetConnections()) {
                    stack.push_back(&c->GetSourceNode());
                }
            }
        }
    }
}

/// @}

PXR_NAMESPACE_CLOSE_SCOPE

#endif


