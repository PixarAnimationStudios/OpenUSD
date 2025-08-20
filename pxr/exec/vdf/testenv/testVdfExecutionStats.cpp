//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/testUtils.h"
#include "pxr/exec/vdf/executionStats.h"

#include <iostream>
#include <thread>

PXR_NAMESPACE_USING_DIRECTIVE

void 
Log(VdfTestUtils::ExecutionStats& stats, VdfId id)
{
    for (int i = 0; i < 100; ++i) {
        stats.Log(VdfExecutionStats::NodeEvaluateEvent, id, i);
    }
}

static int
TestSimpleLogging()
{
    std::cout << "TestSimpleLogging ...";
    VdfTestUtils::ExecutionStats stats;

    for (size_t i = 0; i < 100; ++i) {
        stats.Log(VdfExecutionStats::NodeEvaluateEvent, 12, i);
    }

    VdfTestUtils::ExecutionStatsProcessor processor;
    stats.GetProcessedStats(&processor);

    if (processor.events.size() != 1) {
        std::cerr << "Actual : " << processor.events.size() << std::endl;
        return 1;
    }

    if (processor.events.begin()->second.size() != 100) {
        std::cerr << "Actual : "
            << processor.events.begin()->second.size() << std::endl;
        return 2;
    }

    for (size_t i = 0; i < 100; ++i) {
        const VdfExecutionStats::Event& e = processor.events.begin()->second[i];
        if (e.event != VdfExecutionStats::NodeEvaluateEvent ||
            e.nodeId != 12 ||
            e.data != i) {
            return 3;
        }
    }

    return 0;
}

static int
TestSimpleMultiThreadedLogging()
{
    std::cout << "TestSimpleMultiThreadedLogging ...";
    VdfTestUtils::ExecutionStats stats;

    std::thread a(Log, std::ref(stats), 12),
                b(Log, std::ref(stats), 13), 
                c(Log, std::ref(stats), 14);

    a.join();
    b.join();
    c.join();

    VdfTestUtils::ExecutionStatsProcessor processor;
    stats.GetProcessedStats(&processor);

    if (processor.events.size() == 0) {
        std::cerr << "Actual : " << processor.events.size() << std::endl;
        return 4;
    }

    size_t lastSeen[3] = {0, 0, 0};

    using ThreadToEvents =
        VdfTestUtils::ExecutionStatsProcessor::ThreadToEvents;
    for (const ThreadToEvents::value_type &threadAndEvents : processor.events) {
        if (threadAndEvents.second.size() == 0) {
            return 5;
        }

        for (const VdfExecutionStats::Event &e : threadAndEvents.second) {
            if (e.event != VdfExecutionStats::NodeEvaluateEvent ||
                e.data != lastSeen[e.nodeId - 12]) {
                std::cerr << "Event : " << e.event << std::endl;
                std::cerr << "Node : " << e.nodeId << std::endl;
                std::cerr << "Data : " << e.data << std::endl;
                std::cerr << "Last : " << lastSeen[e.nodeId - 12] << std::endl;
                return 6; 
            }
            ++lastSeen[e.nodeId - 12];
        }
    }

    for (size_t i = 0 ; i < 3; ++i) {
        if (lastSeen[i] != 100) {
            std::cerr << "i : " << i << std::endl;
            std::cerr << "lastSeen : " << lastSeen[i] << std::endl;
            return 100;
        }
    }

    return 0;
}

static int
TestAddingSubStat()
{
    std::cout << "TestAddingSubStat ...";

    VdfTestUtils::ExecutionStats stats;
    stats.AddSubStat(20);
    stats.AddSubStat(12);

    std::thread a(Log, std::ref(stats), 12),
            b(Log, std::ref(stats), 13), 
            c(Log, std::ref(stats), 14);

    a.join();
    b.join();
    c.join();

    VdfTestUtils::ExecutionStatsProcessor processor; 
    stats.GetProcessedStats(&processor);

    size_t lastSeen[3] = {0, 0, 0};

    if (processor.events.size() != 3) {
        std::cerr << "Actual : " << processor.events.size() << std::endl;
        return 7;
    }

    using ThreadToEvents =
        VdfTestUtils::ExecutionStatsProcessor::ThreadToEvents;
    for (const ThreadToEvents::value_type &threadAndEvents : processor.events) {
        if (threadAndEvents.second.size() == 0) {
            return 8;
        }

        for (const VdfExecutionStats::Event &e : threadAndEvents.second) {

            if (e.event != VdfExecutionStats::NodeEvaluateEvent ||
                e.data != lastSeen[e.nodeId - 12]) {
                return 9; 
            }

            ++lastSeen[e.nodeId - 12];
        }
    }

    if (processor.subStats.size() != 2) {
        return 10;
    }

    for (size_t i = 0; i < 2; ++i) {
        if (processor.subStats[i]->events.size() != 0) {
            return 11;
        }
    }

    for (size_t i = 0; i < 3; ++i) {
        if (lastSeen[i] != 100) {
            return 101;
        }
    }

    return 0;
}

static int
TestMultiStats()
{
    std::cout << "TestMultiStats ...";

    VdfTestUtils::ExecutionStats stats_A, stats_B;
    stats_A.AddSubStat(10);
    stats_A.AddSubStat(15);

    std::thread 
            a(Log, std::ref(stats_A), 12),
            b(Log, std::ref(stats_A), 13), 
            c(Log, std::ref(stats_B), 14),
            d(Log, std::ref(stats_B), 15);

    a.join();
    b.join();
    c.join();
    d.join();

    VdfTestUtils::ExecutionStatsProcessor proc_A, proc_B;
    stats_A.GetProcessedStats(&proc_A);
    stats_B.GetProcessedStats(&proc_B);

    using ThreadToEvents =
        VdfTestUtils::ExecutionStatsProcessor::ThreadToEvents;

    size_t lastSeen[4] = {0, 0, 0, 0};

    // Sub stats A

    if (proc_A.events.size() != 2) {
        std::cerr << "Actual : " << proc_A.events.size() << std::endl;
        return 12;
    }

    for (const ThreadToEvents::value_type &threadAndEvents : proc_A.events) {
        if (threadAndEvents.second.size() == 0) {
            return 13;
        }

        for (const VdfExecutionStats::Event &e : threadAndEvents.second) {
            if (e.event != VdfExecutionStats::NodeEvaluateEvent ||
                e.data != lastSeen[e.nodeId - 12]) {
                return 14; 
            }
            ++lastSeen[e.nodeId - 12];
        }

    }

    if (proc_A.subStats.size() != 2) {
        return 15;
    }

    for (size_t i = 0; i < 2; ++i) {
        if (proc_A.subStats[i]->events.size() != 0) {
            return 16;
        }
    }

    // Sub stats B

    if (proc_B.events.size() != 2) {
        std::cerr << "Actual : " << proc_B.events.size() << std::endl;
        return 17;
    }

    for (const ThreadToEvents::value_type &threadAndEvents : proc_B.events) {
        if (threadAndEvents.second.size() != 100) {
            return 18;
        }

        for (const VdfExecutionStats::Event &e : threadAndEvents.second) {
            if (e.event != VdfExecutionStats::NodeEvaluateEvent ||
                e.data != lastSeen[e.nodeId - 12]) {
                return 19; 
            }
            ++lastSeen[e.nodeId - 12];
        }
    }

    if (proc_B.subStats.size() != 0) {
        return 20;
    }

    for (size_t i = 0; i < 4; ++i) {
        if (lastSeen[i] != 100) {
            return 102;
        }
    }

    return 0;
}

typedef int(*TestFunction)(void);
static const TestFunction tests[] = {
    TestSimpleLogging,
    TestSimpleMultiThreadedLogging,
    TestAddingSubStat,
    TestMultiStats,
    NULL
};

int main(int argc, char **argv) {
    int err = 0;
    for (int i = 0; tests[i] != NULL; ++i) {
        if ((err = tests[i]())) {
            std::cout << "FAILED" << std::endl;
            return err;
        }
        std::cout << "PASSED" << std::endl;
    }

    return err;
}
