//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/ef/dependencyCache.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/threadLimits.h"
#include "pxr/exec/vdf/testUtils.h"

#include <algorithm>
#include <iostream>
#include <random>

PXR_NAMESPACE_USING_DIRECTIVE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (input)
    (output)
);

// Build a network of nodes with "mover" nodes connected in a chain, one
// "source" node for each mover that provides an input, and one "sink" node for
// each mover that pulls on its output.
static void
_BuildNetwork(
    VdfTestUtils::Network &graph,
    const size_t numNodes,
    std::vector<VdfNode*> *sourceNodes = nullptr,
    std::vector<VdfNode*> *moverNodes = nullptr,
    std::vector<VdfNode*> *sinkNodes = nullptr,
    std::vector<VdfConnection*> *sourceConnections = nullptr,
    std::vector<VdfConnection*> *sinkConnections = nullptr)
{
    VdfTestUtils::CallbackNodeType moverType(+[](const VdfContext&){});
    moverType
        .Read<int>(_tokens->input)
        .Out<int>(_tokens->output);

    VdfTestUtils::CallbackNodeType sourceType(+[](const VdfContext&){});
    sourceType
        .Out<int>(_tokens->output);

    VdfTestUtils::CallbackNodeType sinkType(+[](const VdfContext&){});
    sinkType
        .Read<int>(_tokens->input)
        .Out<int>(_tokens->output);

    const VdfMask oneOneMask = VdfMask::AllOnes(1);

    for (size_t i=0; i<numNodes; ++i) {
        const std::string sourceName = TfStringPrintf("source%zu", i);
        graph.Add(sourceName, sourceType);
        if (sourceNodes) {
            sourceNodes->push_back(graph[sourceName].GetVdfNode());
        }

        const std::string moverName = TfStringPrintf("mover%zu", i);
        graph.Add(moverName, moverType);
        if (moverNodes) {
            moverNodes->push_back(graph[moverName].GetVdfNode());
        }

        const std::string sinkName = TfStringPrintf("sink%zu", i);
        graph.Add(sinkName, sinkType);
        if (sinkNodes) {
            sinkNodes->push_back(graph[sinkName].GetVdfNode());
        }

        // Connect the source node to the mover node.
        graph[sourceName] >> graph[moverName].In(_tokens->input, oneOneMask);

        if (sourceConnections) {
            VdfConnection *const connection =
                graph.GetConnection(
                    TfStringPrintf(
                        "%s:output -> %s:input",
                        sourceName.c_str(), moverName.c_str()));
            TF_AXIOM(connection);
            sourceConnections->push_back(connection);
        }

        // Connect the movers in a chain.
        if (i > 0) {
            graph[TfStringPrintf("mover%zu", i-1)] >>
            graph[moverName].In(_tokens->input, oneOneMask);
        }

        // Connect the mover node to the sink node.
        graph[moverName] >> graph[sinkName].In(_tokens->input, oneOneMask);

        if (sinkConnections) {
            VdfConnection *const connection =
                graph.GetConnection(
                    TfStringPrintf(
                        "%s:output -> %s:input",
                        moverName.c_str(), sinkName.c_str()));
            TF_AXIOM(connection);
            sinkConnections->push_back(connection);
        }
    }
}

#define ASSERT_EQ(expr, expected)                                       \
    [&] {                                                               \
        auto&& expr_ = expr;                                            \
        if (expr_ != expected) {                                        \
            TF_FATAL_ERROR(                                             \
                "Expected " TF_PP_STRINGIZE(expr) " == '%s'; got '%s'", \
                TfStringify(expected).c_str(),                          \
                TfStringify(expr_).c_str());                            \
        }                                                               \
     }()

static void
_VerifyExpectedNodes(
    const std::vector<const VdfNode *> &resultNodes,
    const std::vector<std::string> &expectedNodes)
{
    ASSERT_EQ(resultNodes.size(), expectedNodes.size());
    for (size_t i=0; i<expectedNodes.size(); ++i) {
        bool found = false;
        for (size_t j=0; j<resultNodes.size(); ++j) {
            if (resultNodes[j]->GetDebugName() == expectedNodes[i]) {
                found = true;
                break;
            }
        }

        if (!found) {
            TF_FATAL_ERROR(
                "Failed to find expected node %s\n",
                expectedNodes[i].c_str());
        }
    }
}

static void
_VerifyExpectedOutputs(
    const VdfOutputToMaskMap &resultOutputsMap,
    const std::vector<std::string> &expectedOutputs)
{
    ASSERT_EQ(resultOutputsMap.size(), expectedOutputs.size());
    for (size_t j=0; j<expectedOutputs.size(); ++j) {
        bool found = false;
        for (const auto& [output, mask] : resultOutputsMap) {
            if (output->GetDebugName() == expectedOutputs[j]) {
                found = true;
                break;
            }
        }

        if (!found) {
            TF_FATAL_ERROR(
                "Failed to find expected output %s\n",
                expectedOutputs[j].c_str());
        }
    }
}

static bool
_FindSinkNodes(
    const VdfNode &node,
    VdfOutputToMaskMap *outputDeps,
    std::vector<const VdfNode *> *nodeDeps)
{
    const bool foundSink =
        node.GetDebugName().find("sink") != std::string::npos;
    if (foundSink) {
        nodeDeps->push_back(&node);

        const VdfInput *const input = node.GetInput(_tokens->input);
        if (TF_VERIFY(input)) {
            const VdfConnection *const connection =
                input->GetConnections().front();
            outputDeps->emplace(
                &connection->GetSourceOutput(),
                connection->GetMask());
        }
        return false;
    }
    return true;
}

static void
testBasic(const bool updateIncrementally)
{
    std::cout << "\nTesting basic dependency cache functionality.\n";       
    std::cout << "updateIncrementally = " << updateIncrementally << "\n";

    // Create the network
    VdfTestUtils::Network graph;
    std::vector<VdfNode*> sourceNodes, moverNodes, sinkNodes;
    _BuildNetwork(graph, 4, &sourceNodes, &moverNodes, &sinkNodes);
    VdfNetwork &network = graph.GetNetwork();

    EfDependencyCache cache(&_FindSinkNodes);

    static const VdfMask oneOneMask = VdfMask::AllOnes(1);

    // These expected results are used for the initial and final state of the
    // network.
    static const std::vector<std::vector<std::string>> expectedNodes(
        {{"VdfTestUtils::DependencyCallbackNode sink0",
          "VdfTestUtils::DependencyCallbackNode sink1",
          "VdfTestUtils::DependencyCallbackNode sink2",
          "VdfTestUtils::DependencyCallbackNode sink3"},
         {"VdfTestUtils::DependencyCallbackNode sink1",
          "VdfTestUtils::DependencyCallbackNode sink2",
          "VdfTestUtils::DependencyCallbackNode sink3"},
         {"VdfTestUtils::DependencyCallbackNode sink2",
          "VdfTestUtils::DependencyCallbackNode sink3"},
         {"VdfTestUtils::DependencyCallbackNode sink3"}});
    static const std::vector<std::vector<std::string>>
        expectedOutputs(
            {{{"VdfTestUtils::DependencyCallbackNode mover0[output]"},
              {"VdfTestUtils::DependencyCallbackNode mover1[output]"},
              {"VdfTestUtils::DependencyCallbackNode mover2[output]"},
              {"VdfTestUtils::DependencyCallbackNode mover3[output]"}},
             {{"VdfTestUtils::DependencyCallbackNode mover1[output]"},
              {"VdfTestUtils::DependencyCallbackNode mover2[output]"},
              {"VdfTestUtils::DependencyCallbackNode mover3[output]"}},
             {{"VdfTestUtils::DependencyCallbackNode mover2[output]"},
              {"VdfTestUtils::DependencyCallbackNode mover3[output]"}},
             {{"VdfTestUtils::DependencyCallbackNode mover3[output]"}}});

    TF_AXIOM(sourceNodes.size() == expectedNodes.size());

    {
        std::cout << "Test network traversals\n";

        for (size_t i=0; i<sourceNodes.size(); ++i) {
            VdfNode *const node = sourceNodes[i];
            VdfMaskedOutputVector outputs;
            outputs.emplace_back(node->GetOutput(), oneOneMask);

            const std::vector<const VdfNode *> &resultNodes =
                cache.FindNodes(outputs, updateIncrementally);
            const VdfOutputToMaskMap &resultOutputsMap =
                cache.FindOutputs(outputs, updateIncrementally);

            _VerifyExpectedNodes(resultNodes, expectedNodes[i]);
            _VerifyExpectedOutputs(resultOutputsMap, expectedOutputs[i]);
        }
    }

    const std::string connectionToDeleteAndReAdd("mover0:output -> sink0:input");

    {
        std::cout << "Test network traversals after deleting a connection.\n";

        VdfConnection *const connection =
            graph.GetConnection(connectionToDeleteAndReAdd);
        TF_AXIOM(connection);
        cache.WillDeleteConnection(*connection);
        network.Disconnect(connection);

        static const std::vector<std::vector<std::string>> expectedNodes(
            {{"VdfTestUtils::DependencyCallbackNode sink1",
              "VdfTestUtils::DependencyCallbackNode sink2",
              "VdfTestUtils::DependencyCallbackNode sink3"},
             {"VdfTestUtils::DependencyCallbackNode sink1",
              "VdfTestUtils::DependencyCallbackNode sink2",
              "VdfTestUtils::DependencyCallbackNode sink3"},
             {"VdfTestUtils::DependencyCallbackNode sink2",
              "VdfTestUtils::DependencyCallbackNode sink3"},
             {"VdfTestUtils::DependencyCallbackNode sink3"}});
        static const std::vector<std::vector<std::string>>
            expectedOutputs(
                {{{"VdfTestUtils::DependencyCallbackNode mover1[output]"},
                  {"VdfTestUtils::DependencyCallbackNode mover2[output]"},
                  {"VdfTestUtils::DependencyCallbackNode mover3[output]"}},
                 {{"VdfTestUtils::DependencyCallbackNode mover1[output]"},
                  {"VdfTestUtils::DependencyCallbackNode mover2[output]"},
                  {"VdfTestUtils::DependencyCallbackNode mover3[output]"}},
                 {{"VdfTestUtils::DependencyCallbackNode mover2[output]"},
                  {"VdfTestUtils::DependencyCallbackNode mover3[output]"}},
                 {{"VdfTestUtils::DependencyCallbackNode mover3[output]"}}});

        for (size_t i=0; i<sourceNodes.size(); ++i) {
            VdfNode *const node = sourceNodes[i];
            const VdfMaskedOutputVector outputs(
                1, {node->GetOutput(), oneOneMask});

            const std::vector<const VdfNode *> &resultNodes =
                cache.FindNodes(outputs, updateIncrementally);
            const VdfOutputToMaskMap &resultOutputsMap =
                cache.FindOutputs(outputs, updateIncrementally);

            _VerifyExpectedNodes(resultNodes, expectedNodes[i]);
            _VerifyExpectedOutputs(resultOutputsMap, expectedOutputs[i]);
        }
    }

    {
        std::cout << "Test network traversals after creating a connection.\n";

        // Re-create the connection we deleted
        graph["mover0"] >> graph["sink0"].In(_tokens->input, oneOneMask);
        VdfConnection *const connection =
            graph.GetConnection(connectionToDeleteAndReAdd);
        TF_AXIOM(connection);
        cache.DidConnect(*connection);

        for (size_t i=0; i<sourceNodes.size(); ++i) {
            VdfNode *const node = sourceNodes[i];
            VdfMaskedOutputVector outputs;
            outputs.emplace_back(node->GetOutput(), oneOneMask);

            const std::vector<const VdfNode *> &resultNodes =
                cache.FindNodes(outputs, updateIncrementally);
            const VdfOutputToMaskMap &resultOutputsMap =
                cache.FindOutputs(outputs, updateIncrementally);

            _VerifyExpectedNodes(resultNodes, expectedNodes[i]);
            _VerifyExpectedOutputs(resultOutputsMap, expectedOutputs[i]);
        }
    }
}

// Performs multiple queries on the given cache, using a fixed number of
// randomply chosen outputs in each request. Verfies that the query results
// match those from a freshly computed cache.
static void
_QueryCache(
    const bool updateIncrementally,
    const size_t numQueries,
    const size_t numNodes,
    const std::vector<VdfNode*> &sourceNodes,
    EfDependencyCache *const cache)
{
    TRACE_FUNCTION();

    size_t numNodesFound = 0, numOutputsFound = 0;

    for (size_t i=0; i<numQueries; ++i) {
        std::mt19937 rng(i);
        std::uniform_int_distribution<size_t> randomNode(0, numNodes-1);

        VdfMaskedOutputVector outputs;
        static constexpr size_t requestSize = 10;
        for (size_t j=0; j<requestSize; ++j) {
            const size_t sourceI = randomNode(rng);
                
            VdfNode *const node = sourceNodes[sourceI];
            outputs.emplace_back(node->GetOutput(), VdfMask::AllOnes(1));
        }

        const std::vector<const VdfNode *> &resultNodes =
            cache->FindNodes(outputs, updateIncrementally);
        const VdfOutputToMaskMap &resultOutputsMap =
            cache->FindOutputs(outputs, updateIncrementally);

        // Re-compute the query results using a fresh cache.
        EfDependencyCache referenceCache(&_FindSinkNodes);
        const std::vector<const VdfNode *> &referenceNodes =
            referenceCache.FindNodes(outputs, /* updateIncrementally */ false);
        const VdfOutputToMaskMap &referenceOutputsMap =
            referenceCache.FindOutputs(outputs, /* updateIncrementally */ false);

        // Make sure the traversal query results match.
        std::set<const VdfNode *> resultNodeSet(
            resultNodes.begin(), resultNodes.end());
        std::set<const VdfNode *> referenceNodeSet(
            referenceNodes.begin(), referenceNodes.end());
        TF_AXIOM(resultNodes == referenceNodes);
        TF_AXIOM(resultOutputsMap == referenceOutputsMap);

        numNodesFound += resultNodeSet.size();
        numOutputsFound += resultOutputsMap.size();
    }

    // As a hedge against a test case that leaves us with empty query results,
    // make sure *some* traversals found some non-empty results.
    TF_AXIOM(numNodesFound > 0);
    TF_AXIOM(numOutputsFound > 0);
}

static void
testThreadingDeleteConnections(
    const bool updateIncrementally)
{
    std::cout << "\nTesting deletion of connections using "
              << WorkGetConcurrencyLimit() << " threads.\n";
    std::cout << "updateIncrementally = " << updateIncrementally << "\n";

    static constexpr size_t numNodes = 1000;

    VdfTestUtils::Network graph;
    std::vector<VdfNode*> sourceNodes;
    std::vector<VdfConnection*> sourceConnections(numNodes);
    std::vector<VdfConnection*> sinkConnections(numNodes);
    {
        TRACE_SCOPE("Build network");

        _BuildNetwork(
            graph,
            numNodes,
            &sourceNodes,
            /* moverNodes */ nullptr,
            /* sinkNodes */ nullptr,
            &sourceConnections,
            &sinkConnections);
    }
    VdfNetwork &network = graph.GetNetwork();

    EfDependencyCache cache(&_FindSinkNodes);
    static constexpr size_t numQueries = 100;
    {
        TRACE_SCOPE("Query network");

        _QueryCache(
            updateIncrementally, numQueries, numNodes, sourceNodes, &cache);
    }

    {
        TRACE_SCOPE("Delete connections");

        std::mt19937 rng(0);

        // We carefully dole out connections to be deleted such the resulting
        // VdfNetwork edits are thread safe. By concurrently deleting unique
        // source connections (each of which connects to a unique source node
        // and mover node) and sink connections (each of which is similarly
        // unique), we never concurrently delete connections that share a common
        // input or output.
        
        std::vector<size_t> sourceConnectionIndices(sourceConnections.size());
        std::iota(
            sourceConnectionIndices.begin(), sourceConnectionIndices.end(), 0);
        std::shuffle(
            sourceConnectionIndices.begin(), sourceConnectionIndices.end(),
            rng);

        std::vector<size_t> sinkConnectionIndices(sinkConnections.size());
        std::iota(
            sinkConnectionIndices.begin(), sinkConnectionIndices.end(), 0);
        std::shuffle(
            sinkConnectionIndices.begin(), sinkConnectionIndices.end(),
            rng);

        WorkParallelForN(
            numNodes,
            [&cache, &network,
             &sourceConnections, &sinkConnections,
             &sourceConnectionIndices, &sinkConnectionIndices]
            (size_t b, size_t e) {
                std::mt19937 rng(0);
                rng.discard(b);

                // 0 : delete source-to-mover connection
                // 1 : delete mover-to-sink connection
                std::uniform_int_distribution<size_t> randomOperation(0, 1);

                for (size_t i = b; i != e; ++i) {
                    VdfConnection *const connection =
                        randomOperation(rng) == 0
                        ? sourceConnections[sourceConnectionIndices[i]]
                        : sinkConnections[sinkConnectionIndices[i]];
                    if (connection) {
                        cache.WillDeleteConnection(*connection);
                        network.Disconnect(connection);
                    }
                }
            });
    }

    {
        TRACE_SCOPE("Query network after deleting connections");

        EfDependencyCache cache(&_FindSinkNodes);
        _QueryCache(
            updateIncrementally, numQueries, numNodes, sourceNodes, &cache);
    }
}

static void
testThreadingCreateConnections(
    const bool updateIncrementally)
{
    std::cout << "\nTesting creation of connections using "
              << WorkGetConcurrencyLimit() << " threads.\n";
    std::cout << "updateIncrementally = " << updateIncrementally << "\n";

    static constexpr size_t numNodes = 1000;

    VdfTestUtils::Network graph;
    std::vector<VdfNode*> sourceNodes, moverNodes, sinkNodes;
    {
        TRACE_SCOPE("Build network");

        _BuildNetwork(graph, numNodes, &sourceNodes, &moverNodes, &sinkNodes);
    }
    VdfNetwork &network = graph.GetNetwork();

    static constexpr size_t numQueries = 100;
    EfDependencyCache cache(&_FindSinkNodes);
    {
        TRACE_SCOPE("Query network");

        _QueryCache(
            updateIncrementally, numQueries, numNodes, sourceNodes, &cache);
    }

    {
        TRACE_SCOPE("Create connections");

        // We are carefully to create create connections such that the resulting
        // VdfNetwork edits are thread safe. In particular, we avoid concurrent
        // creation of connections that share a common input.

        std::mt19937 rng(0);

        std::vector<size_t> moverIndices(numNodes);
        std::iota(moverIndices.begin(), moverIndices.end(), 0);
        std::shuffle(moverIndices.begin(), moverIndices.end(), rng);

        std::vector<size_t> sinkIndices(numNodes);
        std::iota(sinkIndices.begin(), sinkIndices.end(), 0);
        std::shuffle(sinkIndices.begin(), sinkIndices.end(), rng);

        WorkParallelForN(
            numNodes,
            [&cache, &network,
             &sourceNodes, &moverNodes, &sinkNodes,
             &moverIndices, &sinkIndices]
            (size_t b, size_t e) {
                std::mt19937 rng(0);
                rng.discard(b);

                // 0 : create source-to-mover connection
                // 1 : create mover-to-sink connection
                std::uniform_int_distribution<size_t> randomOperation(0, 1);

                std::uniform_int_distribution<size_t> randomNode(0, numNodes-1);

                for (size_t i = b; i != e; ++i) {
                    VdfNode *fromNode = nullptr;
                    VdfNode *toNode = nullptr;

                    if (randomOperation(rng) == 0) {
                        fromNode = sourceNodes[randomNode(rng)];
                        toNode = moverNodes[moverIndices[i]];
                    } else {
                        fromNode = moverNodes[randomNode(rng)];
                        toNode = sinkNodes[sinkIndices[i]];
                    }

                    VdfConnection *const connection = network.Connect(
                        fromNode->GetOutput(),
                        toNode, _tokens->input,
                        VdfMask::AllOnes(1));
                    TF_AXIOM(connection);
                    cache.DidConnect(*connection);
                }
            });
    }

    {
        TRACE_SCOPE("Query network after creating connections");

        EfDependencyCache cache(&_FindSinkNodes);
        _QueryCache(
            updateIncrementally, numQueries, numNodes, sourceNodes, &cache);
    }
}

int
main(int argc, char **argv) 
{
    testBasic(/* updateIncrementally */ false);
    testBasic(/* updateIncrementally */ true);

    TraceCollector::GetInstance().SetEnabled(true);
    {
        // Make sure the threading test cases work single threaded.
        WorkSetConcurrencyLimit(1);

        TRACE_SCOPE("Single threaded");

        testThreadingDeleteConnections(/* updateIncrementally */ false);
        testThreadingDeleteConnections(/* updateIncrementally */ true);
        testThreadingCreateConnections(/* updateIncrementally */ false);
        testThreadingCreateConnections(/* updateIncrementally */ true);
    }
    {
        WorkSetMaximumConcurrencyLimit();

        TRACE_SCOPE("Maximum concurrency");

        testThreadingDeleteConnections(/* updateIncrementally */ false);
        testThreadingDeleteConnections(/* updateIncrementally */ true);
        testThreadingCreateConnections(/* updateIncrementally */ false);
        testThreadingCreateConnections(/* updateIncrementally */ true);
    }
    TraceReporter::GetGlobalReporter()->Report(std::cout);

    return 0;
}
