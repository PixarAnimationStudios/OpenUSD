//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/executionTypeRegistry.h"
#include "pxr/exec/vdf/isolatedSubnetwork.h"
#include "pxr/exec/vdf/readIterator.h"
#include "pxr/exec/vdf/readWriteIterator.h"
#include "pxr/exec/vdf/sparseInputTraverser.h"
#include "pxr/exec/vdf/sparseOutputTraverser.h"
#include "pxr/exec/vdf/testUtils.h"

#include "pxr/base/gf/vec2d.h"
#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/staticTokens.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (axis)
    ((pool, ".pool"))

    (input)
    (input1)
    (input2)
    (output)
    (output1)
    (output2)
);

TF_REGISTRY_FUNCTION(VdfExecutionTypeRegistry)
{
    VdfExecutionTypeRegistry::Define(GfVec3d(0));
}

static void
TranslatePoints(const VdfContext &context) 
{
    const GfVec3d &axis = context.GetInputValue<GfVec3d>(_tokens->axis);
    VdfReadWriteIterator<GfVec3d> iter(context, _tokens->pool);
    for ( ; !iter.IsAtEnd(); ++iter) {
        *iter += axis;
    }
}

VdfNode *
BuildTestNetwork(VdfTestUtils::Network &graph,
                  VdfNode **pointsNode = NULL)
{
    // We're going to build a network like this:
    //                            
    //           points (2 points)
    //             |              
    //             | [10]   axis1
    //             |       /
    //            translate1
    //             |
    //             | [01]  axis2
    //             |      /
    //            translate2
    //             |
    //
    //              

    graph.AddInputVector<GfVec3d>("points", 2);    
    graph["points"]
        .SetValue(0, GfVec3d(1, 0, 0))
        .SetValue(1, GfVec3d(0, 1, 0));

    graph.AddInputVector<GfVec3d>("axis1");
    graph["axis1"]
        .SetValue(0, GfVec3d(1, 0, 0));

    graph.AddInputVector<GfVec3d>("axis2");
    graph["axis2"]
        .SetValue(0, GfVec3d(0, 1, 0));

    graph.AddInputVector<GfVec3d>("disconnectedOutput");
    graph["disconnectedOutput"]
        .SetValue(0, GfVec3d(0, 1, 0));

    VdfMask point1Mask(2);
    VdfMask point2Mask(2);
    point1Mask.SetIndex(0);
    point2Mask.SetIndex(1);

    VdfMask oneOneMask = VdfMask::AllOnes(1);
    VdfMask twoOnesMask = VdfMask::AllOnes(2);

    VdfTestUtils::CallbackNodeType translateNodeType(&TranslatePoints);
    translateNodeType
        .ReadWrite<GfVec3d>(_tokens->pool, _tokens->pool)
        .Read<GfVec3d>(_tokens->axis);

    graph.Add("translate1", translateNodeType);
    graph.Add("translate2", translateNodeType);

    graph["points"]     >> graph["translate1"].In(_tokens->pool, twoOnesMask);
    graph["translate1"].GetVdfNode()->GetOutput()->SetAffectsMask(point1Mask);
    graph["axis1"]  >> graph["translate1"].In(_tokens->axis, oneOneMask);

    graph["translate1"] >> graph["translate2"].In(_tokens->pool, twoOnesMask);
    graph["translate2"].GetVdfNode()->GetOutput()->SetAffectsMask(point2Mask);
    graph["axis2"]       >> graph["translate2"].In(_tokens->axis, oneOneMask);

    // Add connection to be disconnected on the source output side.
    graph["disconnectedOutput"] >> graph["translate2"].In(_tokens->axis, oneOneMask);

    if (pointsNode) {
        *pointsNode = graph["points"];
    }

    return graph["translate2"];
}

static void
DoNothingNodeCallback(const VdfContext &context) 
{
    // do nothing
}

static VdfMask::Bits
CommonNodeComputeInputDependencyMaskCallback(
    const VdfMaskedOutput &maskedOutput,
    const VdfConnection &inputConnection)
{
    // output1 depends on input1, but not on input2
    // output2 depends on input2, but not on input1

    VdfMask::Bits bits(1);

    if ((maskedOutput.GetOutput()->GetName() == _tokens->output1 &&
        inputConnection.GetTargetInput().GetName() == _tokens->input1) ||
        (maskedOutput.GetOutput()->GetName() == _tokens->output2 &&
        inputConnection.GetTargetInput().GetName() == _tokens->input2)) {

        bits.SetAll();
    }

    return bits;
}

VdfNode *
BuildTestNetworkWithInputDependencyCallback(VdfTestUtils::Network &graph)
{
    // We're going to build a network like this:
    //                            
    //           input1        input2
    //                |        /
    //                 |      /
    //                  |    /
    //                commonNode
    // (depends on      /    |     (depends on
    //  input1 only)   /      |     input2 only)
    //                /        |
    //           nodeA         nodeB
    //               |         /
    //                |       /
    //                 |     /
    //               outputNode
    //                   |
    //
    //

    graph.AddInputVector<GfVec2d>("input1");
    graph["input1"]
        .SetValue(0, GfVec2d(1, 0));

    graph.AddInputVector<GfVec2d>("input2");
    graph["input2"]
        .SetValue(0, GfVec2d(0, 1));

    // This creates a node with a custom input dependency mask, making output1
    // dependent on input1, but not on input2. output2 on the other hand is
    // dependent on input2, but not on input1.
    // However, When traversing this network via both outputs on commonNode, 
    // we should uncover subnetworks connected to input1 as well as input2!
    VdfTestUtils::CallbackNodeType commonNodeType(&DoNothingNodeCallback);
    commonNodeType
        .Read<GfVec2d>(_tokens->input1)
        .Read<GfVec2d>(_tokens->input2)
        .Out<GfVec2d>(_tokens->output1)
        .Out<GfVec2d>(_tokens->output2)
        .ComputeInputDependencyMaskCallback(
            &CommonNodeComputeInputDependencyMaskCallback);

    graph.Add("commonNode", commonNodeType);

    VdfTestUtils::CallbackNodeType 
        passThroughNodeType(&DoNothingNodeCallback);
    passThroughNodeType
        .ReadWrite<GfVec2d>(_tokens->input, _tokens->output);

    graph.Add("nodeA", passThroughNodeType);
    graph.Add("nodeB", passThroughNodeType);

    VdfTestUtils::CallbackNodeType
        outputNodeType(&DoNothingNodeCallback);
    outputNodeType
        .Read<GfVec2d>(_tokens->input1)
        .Read<GfVec2d>(_tokens->input2)
        .Out<GfVec2d>(_tokens->output);

    graph.Add("outputNode", outputNodeType);

    graph["input1"] >> graph["commonNode"]
        .In(_tokens->input1, VdfMask::AllOnes(1));
    graph["input2"] >> graph["commonNode"]
        .In(_tokens->input2, VdfMask::AllOnes(1));

    graph["commonNode"].Output(_tokens->output1) >> graph["nodeA"]
        .In(_tokens->input, VdfMask::AllOnes(1));
    graph["commonNode"].Output(_tokens->output2) >> graph["nodeB"]
        .In(_tokens->input, VdfMask::AllOnes(1));

    graph["nodeA"] >> graph["outputNode"]
        .In(_tokens->input1, VdfMask::AllOnes(1));
    graph["nodeB"] >> graph["outputNode"]
        .In(_tokens->input2, VdfMask::AllOnes(1));

    return graph["outputNode"];
}

// ---------------------------------------------------------------------------

static void
nodeCallback(const VdfNode &node)
{
    std::cout << "node:   " << node.GetDebugName() << std::endl;
}

static bool
outputCallback(const VdfOutput &output, const VdfMask &mask,
               const VdfInput* prevInput)
{
    std::cout << "output: " << output.GetDebugName() << " with mask "
              << mask << " reached via input: "
              << (prevInput ? prevInput->GetDebugName().c_str()
              : "NULL") << std::endl;
    return true;
}                      

static bool
testOutputTraversal()
{
    std::cout << std::endl << std::endl
              << "Testing traversal in input-to-output direction..."
              << std::endl;
    VdfTestUtils::Network graph;

    VdfNode *node;
    BuildTestNetwork(graph, &node);

    VdfMask point1Mask(2);
    VdfMask point2Mask(2);
    point1Mask.SetIndex(0);
    point2Mask.SetIndex(1);

    {
        std::cout << std::endl
                  << "Traversing with mask " << point1Mask << std::endl;
        VdfMaskedOutputVector request(1, VdfMaskedOutput( node->GetOutput(), point1Mask ) );
        VdfSparseOutputTraverser::Traverse(request, outputCallback, nodeCallback);
    }

    {
        std::cout << std::endl
                  << "Traversing with mask " << point2Mask << std::endl;
        VdfMaskedOutputVector request(1, VdfMaskedOutput( node->GetOutput(), point2Mask ) );
        VdfSparseOutputTraverser::Traverse(request, outputCallback, nodeCallback);
    }

    return true;
}

// ---------------------------------------------------------------------------

static bool
nodePathCallback(const VdfNode            &node,
                 const VdfObjectPtrVector &path)
{
    std::cout << std::endl;
    std::cout << "node: " << node.GetDebugName() << std::endl;
    std::cout << "path: " << std::endl;
    TF_REVERSE_FOR_ALL(i, path) {

        const VdfConnection *connection = i->GetIfConnection();
        TF_VERIFY(connection);

        std::cout << "    " << connection->GetDebugName() << std::endl;
    }

    return true;
}

static bool
nodeCallbackForInputTraversal(const VdfNode &node)
{
    std::cout << "node:   " << node.GetDebugName() << std::endl;
    return true;
}

static bool
testInputTraversal()
{
    std::cout << std::endl << std::endl
              << "Testing traversal in output-to-input direction..."
              << std::endl;
    VdfTestUtils::Network graph;

    VdfNode *node = BuildTestNetwork(graph);

    VdfMask point1Mask(2);
    VdfMask point2Mask(2);
    point1Mask.SetIndex(0);
    point2Mask.SetIndex(1);

    {
        std::cout << std::endl
                  << "Traversing with mask " << point1Mask << std::endl;
        VdfMaskedOutputVector request(1, VdfMaskedOutput( node->GetOutput(), point1Mask ) );
        VdfSparseInputTraverser::TraverseWithPath(
            request, nodePathCallback, NULL);
    }

    {
        std::cout << std::endl
                  << "Traversing with mask " << point2Mask << std::endl;
        VdfMaskedOutputVector request(1, VdfMaskedOutput( node->GetOutput(), point2Mask ) );
        VdfSparseInputTraverser::TraverseWithPath(
            request, nodePathCallback, NULL);
    }

    {
        std::cout << std::endl
                  << "Traversing with CallbackMode set to "
                  << "CallbackModeTerminalNodes and with mask "<< point1Mask
                  << std::endl;
        VdfMaskedOutputVector request(1, VdfMaskedOutput( node->GetOutput(), point1Mask ) );
        VdfSparseInputTraverser::Traverse(
            request, nodeCallbackForInputTraversal,
            VdfSparseInputTraverser::CallbackModeTerminalNodes);
    }

    {
        std::cout << std::endl
                  << "Traversing with CallbackMode set to "
                  << "CallbackModeTerminalNodes and with mask "<< point2Mask
                  << std::endl;
        VdfMaskedOutputVector request(1, VdfMaskedOutput( node->GetOutput(), point2Mask ) );
        VdfSparseInputTraverser::Traverse(
            request, nodeCallbackForInputTraversal,
            VdfSparseInputTraverser::CallbackModeTerminalNodes);
    }

    return true;
}

static bool
testInputTraversalWithInputDependencyCallback()
{
    std::cout << std::endl << std::endl
              << "Testing traversal in output-to-input direction, "
              << "with input depdenceny callback..."
              << std::endl;

    VdfTestUtils::Network graph;

    VdfNode *node = BuildTestNetworkWithInputDependencyCallback(graph);
    VdfMask outputMask = VdfMask::AllOnes(1);

    {
        std::cout << std::endl
                  << "Traversing with mask " << outputMask << std::endl;
        VdfMaskedOutputVector request(1, VdfMaskedOutput( node->GetOutput(), outputMask ) );
        VdfSparseInputTraverser::TraverseWithPath(
            request, nodePathCallback, NULL);
    }

    {
        std::cout << std::endl
                  << "Traversing with CallbackMode set to "
                  << "CallbackModeTerminalNodes and with mask "<< outputMask
                  << std::endl;
        VdfMaskedOutputVector request(1, VdfMaskedOutput( node->GetOutput(), outputMask ) );
        VdfSparseInputTraverser::Traverse(
            request, nodeCallbackForInputTraversal,
            VdfSparseInputTraverser::CallbackModeTerminalNodes);
    }

    return true;
}

// ---------------------------------------------------------------------------

int 
main(int argc, char **argv) 
{
    if (!testOutputTraversal()) {
        return -1;
    }

    if (!testInputTraversal()) {
        return -1;
    }

    if (!testInputTraversalWithInputDependencyCallback()) { 
        return -1;
    }
    
    return 0;
}
