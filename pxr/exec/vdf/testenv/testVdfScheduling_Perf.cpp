//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/exec/vdf/scheduler.h"
#include "pxr/exec/vdf/testUtils.h"

#include "pxr/base/tf/pxrCLI11/CLI11.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/base/trace/trace.h"

#include <iostream>
#include <random>

PXR_NAMESPACE_USING_DIRECTIVE
using namespace pxr_CLI;

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    (amount)
    (point)
    (output)
    (pool)
);

// Builds a network of nodes with "mover" nodes connected in a pool chain, each
// of which affects a random set of entries in the pool, and one "source" node
// for each mover that provides an input to it. Movers also take a 'point' input
// from a mover at a random point earlier in the chain.
//
// Returns a set of masked outputs to request, one for each entry of the pool,
// from the output of the last mover in the chain.
//
static VdfMaskedOutputVector
_BuildNetwork(
    const size_t numNodes,
    const size_t poolSize,
    VdfTestUtils::Network &graph)
{
    // A "mover" node that takes a scalar input from an 'amount' attribute, a
    // 'point' input that reads a point from the point pool, and an assocaited
    // 'pool' read/write input/output.
    VdfTestUtils::CallbackNodeType moverType(+[](const VdfContext&){});
    moverType
        .Read<int>(_tokens->amount)
        .Read<int>(_tokens->point)
        .ReadWrite<int>(_tokens->pool, _tokens->pool);

    // A node with no inputs that provides a source value on its one output.
    VdfTestUtils::CallbackNodeType sourceType(+[](const VdfContext&){});
    sourceType
        .Out<int>(_tokens->output);

    // Masks

    const VdfMask oneOneMask = VdfMask::AllOnes(1);

    std::mt19937 rng(0);
    std::uniform_int_distribution<size_t> poolSizeDist(0, poolSize-1);
    std::uniform_int_distribution<size_t> runLengthDist(1, poolSize/20);

    // Make a pool mask with random contiguous string of bits set.
    const auto _MakeRandomPoolMask =
        [&rng, &poolSizeDist, &runLengthDist, poolSize]
    {
        VdfMask poolMask(poolSize);
        const size_t start = poolSizeDist(rng);
        const size_t end = start + runLengthDist(rng);
        for (size_t i=start; i<end && i<poolSize; ++i) {
            poolMask.SetIndex(i);
        }
        return poolMask;
    };

    // Make a pool mask with a random bit set.
    const auto _MakePoolMaskWithRandomBitSet =
        [&rng, &poolSizeDist, poolSize]
    {
        VdfMask poolMask(poolSize);
        poolMask.SetIndex(poolSizeDist(rng));
        return poolMask;
    };

    // Build the network.
    for (size_t i=0; i<numNodes; ++i) {
        const std::string sourceName = TfStringPrintf("source%zu", i);
        graph.Add(sourceName, sourceType);

        const std::string moverName = TfStringPrintf("mover%zu", i);
        graph.Add(moverName, moverType);
        graph[moverName].GetOutput()->SetAffectsMask(_MakeRandomPoolMask());

        // Connect the source node to the mover node.
        graph[sourceName] >> graph[moverName].In(_tokens->amount, oneOneMask);

        if (i > 0) {
            std::uniform_int_distribution<size_t> earlierMoverDist(0, i-1);
            
            graph[TfStringPrintf("mover%zu", earlierMoverDist(rng))] >>
                graph[moverName].In(
                    _tokens->point, _MakePoolMaskWithRandomBitSet());

            // Connect the movers in a chain.
            graph[TfStringPrintf("mover%zu", i-1)] >>
                graph[moverName].In(_tokens->pool, VdfMask::AllOnes(poolSize));
        }
    }

    // Add each pool output entry individually to the returned set of outputs to
    // request.

    VdfMaskedOutputVector outputs;
    
    VdfTestUtils::Node &lastMover =
        graph[TfStringPrintf("mover%zu", numNodes-1)];

    for (size_t i=0; i<poolSize; ++i) {
        VdfMask mask(poolSize);
        mask.SetIndex(i);

        outputs.emplace_back(lastMover.GetOutput(), mask);
    }        

    return outputs;
}

// RAII class for collecting and reporting trace information.
class _PerformanceTracker
{
public:
    _PerformanceTracker(const bool outputAsSpy, const bool outputAsTrace)
        : _outputAsSpy(outputAsSpy)
        , _outputAsTrace(outputAsTrace) 
    {
        TraceCollector::GetInstance().SetEnabled(true);
    }

    ~_PerformanceTracker() {
        TraceCollector::GetInstance().SetEnabled(false);
        const TraceReporterPtr reporter = TraceReporter::GetGlobalReporter();
        reporter->UpdateTraceTrees();
        _WriteStats(*reporter);
        if (_outputAsSpy) {
            _WriteSpy(*reporter);
        }
        if (_outputAsTrace) {
            _WriteTrace(*reporter);
        }
    }

private:

    // Gets a trace node descenant of \p rootNode.
    //
    // Arguments following \p rootNode describe a path of descendants from the
    // root node. Each descendant is a string for the node name.
    // 
    // If prefixed by a '*', then the descendant matches the first child that
    // ends with the node name. This is useful to select nodes added by
    // TRACE_FUNCTION_SCOPE, because they may or may not be prefixed by the
    // pxr namespace.
    //
    // If ending with '?', then the requested descendant is optional. If an
    // optional descendant is not found, this function returns nullptr. If a
    // non-optional descendant is not found, this reports a TF_FATAL_ERROR.
    //
    template <class... Tail>
    static TraceAggregateNodePtr _GetTraceNode(
        const TraceAggregateNodePtr &rootNode,
        const std::string &childNameExpr,
        Tail &&...tail) {

        TF_AXIOM(childNameExpr.size() > 0);
        const bool isSuffix = childNameExpr.front() == '*';
        const bool isOptional = childNameExpr.back() == '?';
        const std::string childName = TfStringTrim(childNameExpr, "*?");

        TraceAggregateNodePtr childNode;
        if (isSuffix) {
            // Get the child node by finding the first child with the given
            // suffix.
            for (const TraceAggregateNodePtr &child : rootNode->GetChildren()) {
                if (TfStringEndsWith(child->GetKey().GetString(), childName)) {
                    childNode = child;
                    break;
                }
            }
        }
        else {
            // Get the child node by exact name.
            childNode = rootNode->GetChild(childName);
        }

        // If not found, the child must be optional.
        if (!childNode && !isOptional) {
            TF_FATAL_ERROR(
                "Expected trace node '%s' not found", childNameExpr.c_str());
        }
        
        // If optional and not found, return null now. Otherwise, find the
        // remaining children.
        if (isOptional && !childNode) {
            return nullptr;
        }
        return _GetTraceNode(childNode, std::forward<Tail>(tail)...);
    }

    static TraceAggregateNodePtr _GetTraceNode(
        const TraceAggregateNodePtr &rootNode) {
        return rootNode;
    }

    // Writes a node time in seconds to the stats file.
    static void _WriteStat(
        const TraceAggregateNodePtr &node,
        const char *const profileName,
        std::ofstream *const statsFile = nullptr) {
        
        const int count = node ? node->GetCount() : 0;
        const double timeInSeconds = node
            ? (ArchTicksToSeconds(node->GetInclusiveTime()) / count)
            : 0.0;

        // Always write the metric to stdout.
        std::cout << profileName << ": " << timeInSeconds << " seconds\n";

        // Write to the stats file, if provided.
        if (statsFile) {
            *statsFile
                << '{'
                << "'profile':'" << profileName << "',"
                << "'metric':'time',"
                << "'value':" << timeInSeconds << ','
                << "'samples':" << count
                << "}\n";
        }
    }

    static void _WriteStats(TraceReporter &reporter) {
        std::cout << "Writing to perfstats.raw\n";
        std::ofstream statsFile("perfstats.raw");
        TF_AXIOM(statsFile);

        const TraceAggregateNodePtr rootNode = reporter.GetAggregateTreeRoot();
        TF_AXIOM(rootNode);

        _WriteStat(
            _GetTraceNode(
                rootNode, "Main Thread", "*VdfScheduler::CreateSchedule"),
            "scheduling_time",
            &statsFile);
    }

    static void _WriteTrace(TraceReporter &reporter) {
        std::cout << "Writing trace data to scheduling_time.trace\n";
        std::ofstream traceFile("scheduling_time.trace");
        TF_AXIOM(traceFile);
        reporter.Report(traceFile);
    }

    static void _WriteSpy(TraceReporter &reporter) {
        std::cout << "Writing trace data to scheduling_time.spy\n";
        std::ofstream spyFile("scheduling_time.spy");
        TF_AXIOM(spyFile);
        reporter.SerializeProcessedCollections(spyFile);
    }

private:
    bool _outputAsSpy = false;
    bool _outputAsTrace = false;
};

int 
main(int argc, char **argv) 
{
    unsigned numIterations = 100;
    unsigned numNodes = 25000;
    unsigned poolSize = 1000;
    bool outputAsTrace = true;
    bool outputAsSpy = false;

    // Set up arguments and their defaults
    CLI::App app(
        "Measure the performance of scheduling a procedurally-generated "
        "network over multiple iterations.");

    app.add_option(
        "--numIterations", numIterations,
        "The number of times to schedule with interruption.");
    app.add_option(
        "--numNodes", numNodes,
        "The number of mover nodes in the generated network.");
    app.add_option(
        "--poolSize", poolSize,
        "The number of 'points' in the pool of values affected by 'movers'.");
    app.add_flag(
        "--trace", outputAsTrace,
        "Write trace data to test.trace");
    app.add_flag(
        "--spy", outputAsSpy,
        "Write trace data to test.spy");

    CLI11_PARSE(app, argc, argv);

    std::cout << "Running with " << numIterations << " iterations, "
              << numNodes << " nodes, and pool size " << poolSize << ".\n";
    WorkSetMaximumConcurrencyLimit();

    VdfTestUtils::Network network;
    VdfRequest request(_BuildNetwork(numNodes, poolSize, network));

    // Do a dry run, since the first run tends to be slow.
    {
        VdfSchedule schedule;
        VdfScheduler::Schedule(
            request, &schedule, /* topologicallySort */ false);
    }

    {
        _PerformanceTracker performanceTracker(outputAsSpy, outputAsTrace);

        for (size_t i=0; i<numIterations; ++i) {
            VdfSchedule schedule;
            VdfScheduler::Schedule(
                request, &schedule, /* topologicallySort */ false);
        }
    }
}
