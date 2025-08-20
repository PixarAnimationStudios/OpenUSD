//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/networkUtil.h"

#include "pxr/exec/vdf/sparseInputTraverser.h"
#include "pxr/exec/vdf/sparseOutputTraverser.h"

#include "pxr/base/trace/trace.h"

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

static bool
Vdf_NetworkUtil_IsSpeculationNode(
    bool *result, const VdfNode &node)
{
    if (node.IsSpeculationNode()) {
        *result = true;

        // stop traversal
        return false;
    }

    // continue traversal
    return true; 
}

bool VdfIsSpeculating(const VdfMaskedOutput &maskedOutput)
{
    TRACE_FUNCTION();

    bool isSpeculating = false;

    VdfSparseInputTraverser::Traverse(
        VdfMaskedOutputVector(1, maskedOutput),
        std::bind(
            &Vdf_NetworkUtil_IsSpeculationNode,
            &isSpeculating, std::placeholders::_1));

    return isSpeculating;
}

bool
VdfIsTopologicalSourceNode(
    const VdfNode &startNode,
    const VdfNode &nodeToFind,
    bool *foundSpecNode)
{
    TRACE_FUNCTION();

    // Set to true once nodeToFind has been found.
    bool foundNode = false;

    // The traversal callback.
    auto callback =
        [&nodeToFind, &foundSpecNode, &foundNode](const VdfNode &node) {
            bool continueTraversal = true;

            // We found the node we are looking for!
            if (&node == &nodeToFind) {
                foundNode = true;
            }

            // If this node is a speculation node, record that we encountered
            // one, and then stop the traversal along this path. We will still
            // continue traversing along other paths.
            if (node.IsSpeculationNode()) {
                if (foundSpecNode) {
                    *foundSpecNode = true;
                }
                continueTraversal = false;
            }

            // Continue the traversal along this path as long as the node has
            // not been found.
            return continueTraversal && !foundNode;
        };

    // Do the traversal.
    VdfTraverseTopologicalSourceNodes(startNode, callback);

    // Return true if the node has been found.
    return foundNode;
}

const VdfOutput *
VdfGetAssociatedSourceOutput(const VdfOutput &output)
{
    const VdfInput *assocInput = output.GetAssociatedInput();

    if (assocInput && (assocInput->GetConnections().size() == 1)) {
        return &(*assocInput)[0].GetSourceOutput();
    }

    return NULL;
}

void VdfEmptyNodeCallback(const VdfNode &)
{

}

PXR_NAMESPACE_CLOSE_SCOPE
