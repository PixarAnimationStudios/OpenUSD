//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/ef/leafNode.h"
#include "pxr/exec/ef/leafNodeCache.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/threadLimits.h"
#include "pxr/exec/vdf/executionTypeRegistry.h"
#include "pxr/exec/vdf/network.h"

#include <iostream>
#include <random>
#include <set>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (input)
    (output)
);

TF_REGISTRY_FUNCTION(VdfExecutionTypeRegistry)
{
    VdfExecutionTypeRegistry::Define<int>(0);
}

namespace {

class _Node : public VdfNode
{
public:
    _Node(VdfNetwork *network) :
        VdfNode(network,
            VdfInputSpecs().ReadConnector(TfType::Find<int>(), _tokens->input),
            VdfOutputSpecs().Connector(TfType::Find<int>(), _tokens->output))
    {}

    void Compute(const VdfContext &) const override {}    
};

// The network monitor is used to keep the EfLeafNodeCache updated, mirroring
// how the cache is expected to be used in the system.
class _LeafNodeMonitor : public VdfNetwork::EditMonitor
{
public:
    EfLeafNodeCache *Get() {
        return &_leafNodeCache;
    }

    void WillClear() override {
        _leafNodeCache.Clear();
    }

    void DidConnect(const VdfConnection *connection) override {
        _leafNodeCache.DidConnect(*connection);
    }

    void DidAddNode(const VdfNode *node) override {}

    void WillDelete(const VdfConnection *connection) override {
        _leafNodeCache.WillDeleteConnection(*connection);
    }

    void WillDelete(const VdfNode *node) override {}

private:
    EfLeafNodeCache _leafNodeCache;
};

}

static void
_BuildNetworkAndConnect(
    VdfNetwork *const network,
    std::vector<VdfNode*> *const rootNodes,
    std::vector<EfLeafNode*> *const leafNodes)
{
    TRACE_FUNCTION();

    TF_VERIFY(rootNodes && leafNodes && rootNodes->size() == leafNodes->size());

    WorkParallelForN(
        rootNodes->size(),
        [network, rootNodes, leafNodes]
        (size_t b, size_t e) {
            for (size_t i = b; i != e; ++i) {
                VdfNode *const root = new _Node(network);
                VdfNode *const middle = new _Node(network);
                network->Connect(
                    root->GetOutput(), middle,
                    _tokens->input, VdfMask::AllOnes(1));

                EfLeafNode *const leaf = new EfLeafNode(
                    network, TfType::Find<int>());
                network->Connect(
                    middle->GetOutput(), leaf,
                    EfLeafTokens->in, VdfMask::AllOnes(1));

                (*rootNodes)[i] = root;
                (*leafNodes)[i] = leaf;
            }
        });
}

static size_t
_DisconnectSomeLeafNodes(
    VdfNetwork *const network,
    const std::vector<EfLeafNode*> &leafNodes)
{
    TRACE_FUNCTION();

    std::atomic<size_t> numDisconnected(0);

    WorkParallelForN(
        leafNodes.size(),
        [network, &leafNodes, &numDisconnected]
        (size_t b, size_t e) {
            std::mt19937 rng(0);
            rng.discard(b);
            std::bernoulli_distribution doDisconnect;

            for (size_t i = b; i != e; ++i) {
                if (doDisconnect(rng)) {
                    VdfConnection &connection =
                        leafNodes[i]->GetInput(
                            EfLeafTokens->in)->GetNonConstConnection(0);
                    network->Disconnect(&connection);
                    ++numDisconnected;
                }
            }
        });

    return numDisconnected.load();
}

static void
_ReconnectDanglingLeafNodes(
    VdfNetwork *const network,
    const std::vector<VdfNode *> &rootNodes,
    const std::vector<EfLeafNode*> &leafNodes)
{
    TRACE_FUNCTION();

    WorkParallelForN(
        leafNodes.size(),
        [network, &rootNodes, &leafNodes]
        (size_t b, size_t e) {
            std::mt19937 rng(0);
            rng.discard(b);
            std::uniform_int_distribution<size_t> randomNode(
                0, rootNodes.size() - 1);

            for (size_t i = b; i != e; ++i) {
                if (leafNodes[i]->HasInputConnections()) {
                    continue;
                }

                const VdfNode *const rootNode = rootNodes[randomNode(rng)];
                VdfConnection *const connection =
                    rootNode->GetOutput()->GetConnections()[0];
                VdfNode *const middleNode = &connection->GetTargetNode();

                network->Connect(
                    middleNode->GetOutput(), leafNodes[i],
                    EfLeafTokens->in, VdfMask::AllOnes(1));
            }
        });
}

static void
testLeafNodeNetworkEdits(const size_t numNodes)
{
    _LeafNodeMonitor leafNodeMonitor;

    VdfNetwork network;
    network.RegisterEditMonitor(&leafNodeMonitor);

    std::vector<VdfNode*> rootNodes;
    std::vector<EfLeafNode*> leafNodes;
    rootNodes.resize(numNodes);
    leafNodes.resize(numNodes);

    TF_VERIFY(leafNodeMonitor.Get()->GetVersion() == 0);

    _BuildNetworkAndConnect(&network, &rootNodes, &leafNodes);
    TF_VERIFY(leafNodeMonitor.Get()->GetVersion() != 0);

    // Build a "request" of outputs to use for querying the cache
    VdfMaskedOutputVector rootOutputs;
    rootOutputs.reserve(numNodes);
    for (size_t i = 0; i < numNodes; ++i) {
        rootOutputs.push_back(VdfMaskedOutput(
            rootNodes[i]->GetOutput(), VdfMask::AllOnes(1)));
    }

    // Find all the connected leaf nodes, and verify that every newly created
    // leaf node appears in this set
    {
        const std::vector<const VdfNode *> &result =
            leafNodeMonitor.Get()->FindNodes(rootOutputs, true);
        TF_VERIFY(result.size() == numNodes);

        std::set<const VdfNode *> resultSet(result.begin(), result.end());
        TF_VERIFY(resultSet.size() == numNodes);
        for(size_t i = 0; i < numNodes; ++i) {
            TF_VERIFY(resultSet.count(leafNodes[i]));
        }
    }

    // Find all the source outputs, and verify that every source output
    // connected to a newly created leaf node appears in this set
    {
        const VdfOutputToMaskMap &result =
            leafNodeMonitor.Get()->FindOutputs(rootOutputs, true);
        TF_VERIFY(result.size() == numNodes);

        for(size_t i = 0; i < numNodes; ++i) {
            const VdfConnection &connection =
                (*leafNodes[i]->GetInput(EfLeafTokens->in))[0];
            const VdfOutputToMaskMap::const_iterator it =
                result.find(&connection.GetSourceOutput());
            TF_VERIFY(it != result.end() && it->second == VdfMask::AllOnes(1));
        }
    }

    // Randomly disconnect some leaf nodes
    size_t version = leafNodeMonitor.Get()->GetVersion();
    const size_t numDisconnected = _DisconnectSomeLeafNodes(&network, leafNodes);
    TF_VERIFY(leafNodeMonitor.Get()->GetVersion() != version);

    // Find all the connected leaf nodes
    {
        const std::vector<const VdfNode *> &result =
            leafNodeMonitor.Get()->FindNodes(rootOutputs, true);
        TF_VERIFY(result.size() == numNodes - numDisconnected);
    }

    // Find all the source outputs
    {
        const VdfOutputToMaskMap &result =
            leafNodeMonitor.Get()->FindOutputs(rootOutputs, true);
        TF_VERIFY(result.size() == numNodes - numDisconnected);
    }

    // Reconnect dangling leaf nodes to other random nodes
    version = leafNodeMonitor.Get()->GetVersion();
    _ReconnectDanglingLeafNodes(&network, rootNodes, leafNodes);
    TF_VERIFY(leafNodeMonitor.Get()->GetVersion() != version);

    // Find all the connected leaf nodes
    {
        const std::vector<const VdfNode *> &result =
            leafNodeMonitor.Get()->FindNodes(rootOutputs, true);
        TF_VERIFY(result.size() == numNodes);
    }

    network.Clear();
    network.UnregisterEditMonitor(&leafNodeMonitor);
}

int
main(int argc, char **argv) 
{
    const size_t numNodes = 100000;

    TraceCollector::GetInstance().SetEnabled(true);
    {
        // Make sure the threading test cases work single threaded.
        WorkSetConcurrencyLimit(1);

        TRACE_SCOPE("Single threaded");

        testLeafNodeNetworkEdits(numNodes);
    }
    {
        WorkSetMaximumConcurrencyLimit();

        TRACE_SCOPE("Maximum concurrency");

        testLeafNodeNetworkEdits(numNodes);
    }
    TraceReporter::GetGlobalReporter()->Report(std::cout);

    return 0;
}
