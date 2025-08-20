//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/context.h"
#include "pxr/exec/vdf/network.h"
#include "pxr/exec/vdf/node.h"
#include "pxr/exec/vdf/poolChainIndexer.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/threadLimits.h"

#include <cstdint>
#include <iostream>
#include <random>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (in1)
    (in2)
    (in3)
    (out)
    ((pool, ".pool"))
);

static constexpr size_t poolSize = 2;

namespace {

class TestRNode : public VdfNode {
public:
    TestRNode(VdfNetwork *n) :
        VdfNode(n,
            VdfInputSpecs()
                .ReadConnector<int>(_tokens->in1)
                .ReadConnector<int>(_tokens->in2)
                .ReadConnector<int>(_tokens->in3),
            VdfOutputSpecs()
                .Connector<int>(_tokens->out))
    {}

    void Compute(const VdfContext &) const {}
};

class TestRWNode : public VdfNode {
public:
    TestRWNode(VdfNetwork *n) :
        VdfNode(n,
            VdfInputSpecs()
                .ReadWriteConnector<int>(_tokens->pool, _tokens->pool)
                .ReadConnector<int>(_tokens->in1)
                .ReadConnector<int>(_tokens->in2)
                .ReadConnector<int>(_tokens->in3),
            VdfOutputSpecs()
                .Connector<int>(_tokens->pool))
    {
        GetOutput()->SetAffectsMask(VdfMask::AllOnes(poolSize));
        TF_AXIOM(Vdf_IsPoolOutput(*GetOutput()));
    }

    void Compute(const VdfContext &) const {}
};

}

int
main(int, char **)
{
    WorkSetMaximumConcurrencyLimit();

    // While this is a correctness test, we dump profiling information to help
    // investigate other performance regressions.
    TraceCollector::GetInstance().SetEnabled(true);

    VdfNetwork network;

    // Test concurrently adding nodes
    constexpr size_t numNodesFirstPass = 50000;
    {
        TRACE_SCOPE("Create nodes");

        WorkParallelForN(numNodesFirstPass, [&network](size_t b, size_t e) {
            std::mt19937 rng(b);
            std::uniform_int_distribution<size_t> randomNodeType(0, 1);

            for (size_t i = b; i != e; ++i) {
                VdfNode * const node = randomNodeType(rng)
                    ? static_cast<VdfNode*>(new TestRNode(&network))
                    : static_cast<VdfNode*>(new TestRWNode(&network));
                TF_AXIOM(node);

                node->SetDebugName("Round 1 Node");
            }
        });

        TF_AXIOM(network.GetNodeCapacity() == numNodesFirstPass);
        TF_AXIOM(network.GetOutputCapacity() == numNodesFirstPass);
    }

    // Test adding more nodes and making connections to the nodes created in
    // the previous pass.
    constexpr size_t numNodesSecondPass = 50000;
    constexpr size_t numConnections = 10;
    {
        TRACE_SCOPE("Create and connect nodes");

        WorkParallelForN(
            numNodesSecondPass,
            [&network, numNodesFirstPass, numNodesSecondPass, numConnections]
            (size_t b, size_t e) {
            std::mt19937 rng(b);
            std::uniform_int_distribution<size_t> randomNodeType(0, 1);
            std::uniform_int_distribution<size_t> randomSourceNode(
                0, numNodesFirstPass - 1);
            std::uniform_int_distribution<int> randomInput(0, 2);

            for (size_t i = b; i != e; ++i) {
                VdfNode * const targetNode = randomNodeType(rng)
                    ? static_cast<VdfNode*>(new TestRNode(&network))
                    : static_cast<VdfNode*>(new TestRWNode(&network));
                TF_AXIOM(targetNode);

                targetNode->SetDebugName("Round 2 Node");

                const VdfInputSpecs &inputSpecs = targetNode->GetInputSpecs();

                // If there is a r/w connector, make sure we only connect to
                // it once, since it won't support more than one connection and
                // will generate a coding error if we connect more than once.
                if (targetNode->GetOutput()->GetAssociatedInput()) {
                    VdfNode *sourceNode = network.GetNode(
                        randomSourceNode(rng));
                    TF_AXIOM(sourceNode);

                    const VdfConnection * const connection = network.Connect(
                        sourceNode->GetOutput(),
                        targetNode, _tokens->pool,
                        VdfMask::AllOnes(poolSize));
                    TF_AXIOM(connection);
                }

                // Connect a bunch of times to random source nodes and read
                // connectors on the recently created node.
                for (size_t ci = 0; ci < numConnections; ++ci) {
                    VdfNode *sourceNode = network.GetNode(
                        randomSourceNode(rng));
                    TF_AXIOM(sourceNode);

                    // Select a random input on the target node. If the first
                    // one is a r/w input, offset the index by one to only
                    // connect to the read inputs. We alreay made a r/w
                    // connection above.
                    int inputIndex = randomInput(rng);
                    if (targetNode->GetOutput()->GetAssociatedInput()) {
                        ++inputIndex;
                    }

                    const VdfConnection * const connection = network.Connect(
                        sourceNode->GetOutput(),
                        targetNode,
                        inputSpecs.GetInputSpec(inputIndex)->GetName(),
                        VdfMask::AllOnes(1));
                    TF_AXIOM(connection);
                }
            }
        });

        TF_AXIOM(network.GetNodeCapacity() ==
            numNodesFirstPass + numNodesSecondPass);
        TF_AXIOM(network.GetOutputCapacity() ==
            numNodesFirstPass + numNodesSecondPass);
    }

    // Perform basic validation of the network we just created
    {
        TRACE_SCOPE("Validate network");

        size_t numRWConnections = 0;
        size_t numInputConnections = 0;
        size_t numOutputConnections = 0;
        for (size_t i = 0; i < network.GetNodeCapacity(); ++i) {
            const VdfNode *node = network.GetNode(i);
            TF_AXIOM(node);

            // We expect to have made one r/w connection for every r/w node
            // created in the second pass.
            if (i >= numNodesFirstPass && node->IsA<TestRWNode>()) {
                ++numRWConnections;
            }

            // Validate all connections on all inputs in the network.
            for (const auto &[name, input] : node->GetInputsIterator()) {
                for (const VdfConnection *c : input->GetConnections()) {
                    TF_AXIOM(c);
                    ++numInputConnections;

                    // Expect connections to span from first-pass nodes to
                    // second-pass nodes.
                    TF_AXIOM(
                        VdfNode::GetIndexFromId(c->GetSourceNode().GetId()) <
                        numNodesFirstPass);
                    TF_AXIOM(
                        VdfNode::GetIndexFromId(c->GetTargetNode().GetId()) >=
                        numNodesFirstPass);
                }
            }

            // Validate all connections on all outputs in the network.
            for (const auto &[name, output] : node->GetOutputsIterator()) {
                for (const VdfConnection *c : output->GetConnections()) {
                    TF_AXIOM(c);
                    ++numOutputConnections;

                    // Expect connections to span from first-pass nodes to
                    // second-pass nodes.
                    TF_AXIOM(
                        VdfNode::GetIndexFromId(c->GetSourceNode().GetId()) <
                        numNodesFirstPass);
                    TF_AXIOM(
                        VdfNode::GetIndexFromId(c->GetTargetNode().GetId()) >=
                        numNodesFirstPass);
                }
            }     
        }

        const size_t numExpectedConnections =
            numConnections * numNodesSecondPass + numRWConnections;
        TF_AXIOM(numInputConnections == numExpectedConnections);
        TF_AXIOM(numOutputConnections == numExpectedConnections);
    }

    network.DumpStats(std::cout);
    TraceReporter::GetGlobalReporter()->Report(std::cout);

    return 0;
}
