//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/exec/vdf/scheduler.h"
#include "pxr/exec/vdf/testUtils.h"

#include "pxr/base/arch/timing.h"
#include "pxr/base/tf/pxrCLI11/CLI11.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/dispatcher.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>

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

static void _WriteStat(
    const char *const profileName,
    const double timeInSeconds,
    const int count,
    std::ofstream &statsFile)
{
    statsFile
        << '{'
        << "'profile':'" << profileName << "',"
        << "'metric':'time',"
        << "'value':" << timeInSeconds << ','
        << "'samples':" << count
        << "}\n";
}

static void
testCancellation(
    const unsigned numIterations,
    const unsigned numNodes,
    const unsigned poolSize,
    const bool worstCaseSpy)
{
    std::cout << "\nTesting scheduler cancellation.\n";       

    VdfTestUtils::Network network;
    VdfRequest request(_BuildNetwork(numNodes, poolSize, network));

    std::atomic_bool interruptionFlag = false;
    VdfScheduler scheduler(&interruptionFlag);

    // Do a dry run. The first run tends to be slow enough that if we use it as
    // the maxiumum delay before interruption, many runs will schedule to
    // completion.
    {
        VdfSchedule schedule;
        scheduler.CreateSchedule(
            request, &schedule, /* topologicallySort */ true);
    }

    // Measure how long it takes to schedule the second time and use this as the
    // maximum time we will delay before interrupting.
    uint64_t firstScheduleTime;
    {
        VdfSchedule schedule;

        const uint64_t start = ArchGetStartTickTime();

        scheduler.CreateSchedule(
            request, &schedule, /* topologicallySort */ true);

        firstScheduleTime = ArchGetStopTickTime() - start;
    }

    std::cout << "First schedule took "
              << ArchTicksToSeconds(firstScheduleTime)*1000
              << "ms\n";

    WorkDispatcher dispatcher;

    std::mt19937 rng(0);
    std::uniform_int_distribution<int64_t> randomDuration(
        0, ArchTicksToNanoseconds(firstScheduleTime));

    uint64_t totalDelay = 0;
    uint64_t maxDelay = 0;

    for (size_t i=0; i<numIterations;) {
        if (worstCaseSpy) {
            TraceCollector::GetInstance().SetEnabled(true);
        }

        interruptionFlag = false;
        bool completed = false;

        VdfSchedule schedule;
        dispatcher.Run([&request, &scheduler, &schedule, &completed]{
            completed = scheduler.CreateSchedule(
                request, &schedule, /* topologicallySort */ true);
        });

        std::this_thread::sleep_for(
            std::chrono::nanoseconds(randomDuration(rng)));

        const uint64_t start = ArchGetStartTickTime();

        {
            TRACE_SCOPE("Interrupt");
            interruptionFlag = true;
            dispatcher.Wait();
        }

        const uint64_t delay = ArchGetStopTickTime() - start;

        if (worstCaseSpy) {
            TraceCollector::GetInstance().SetEnabled(false);
        }

        // Make sure we get numIterations where we successfully interrupt
        // scheduling.
        if (completed) {
            continue;
        }
        else {
            ++i;
        }

        totalDelay += delay;

        if (delay > maxDelay) {
            maxDelay = delay;

            if (worstCaseSpy) {
                const TraceReporterPtr reporter =
                    TraceReporter::GetGlobalReporter();
                reporter->UpdateTraceTrees();

                std::cout << "Writing worst case trace data to maxDelay.spy\n";
                std::ofstream spyFile("maxDelay.spy");
                TF_AXIOM(spyFile);
                reporter->SerializeProcessedCollections(spyFile);
            }
        }

        if (worstCaseSpy) {
            TraceCollector::GetInstance().Clear();
            TraceReporter::GetGlobalReporter()->ClearTree();
        }
    }

    std::cout << "average interruption delay "
              << ArchTicksToSeconds(totalDelay)*1000/numIterations
              << "ms\n";
    std::cout << "maximum interruption delay "
              << ArchTicksToSeconds(maxDelay)*1000
              << "ms\n";

    std::cout << "Writing to perfstats.raw\n";
    std::ofstream statsFile("perfstats.raw");
    TF_AXIOM(statsFile);

    _WriteStat(
        "interruption_average_time", 
        ArchTicksToSeconds(totalDelay)/numIterations,
        numIterations,
        statsFile);
    _WriteStat(
        "interruption_maximum_time",
        ArchTicksToSeconds(maxDelay),
        /* count */ 1,
        statsFile);
}

int
main(int argc, char **argv) 
{
    unsigned numIterations = 1000;
    unsigned numNodes = 25000;
    unsigned poolSize = 1000;
    bool worstCaseSpy = false;

    // Set up arguments and their defaults
    CLI::App app(
        "Test the performance of scheduling interruption. Measures the delay "
        "between the time when we attempt to interrupt and when scheduling "
        "actually returns.");
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
        "--worstCaseSpy", worstCaseSpy,
        "Report worst-case interruption trace data in .spy format.");

    CLI11_PARSE(app, argc, argv);

    std::cout << "Running with " << numIterations << " iterations, "
              << numNodes << " nodes, and pool size " << poolSize << ".\n";
    WorkSetMaximumConcurrencyLimit();

    testCancellation(numIterations, numNodes, poolSize, worstCaseSpy);
}
