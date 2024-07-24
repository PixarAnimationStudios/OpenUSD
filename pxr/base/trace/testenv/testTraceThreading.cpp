//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/trace/trace.h"
#include "pxr/base/trace/reporter.h"

#include "pxr/base/tf/errorMark.h"

#include <functional>
#include <iostream>
#include <thread>

PXR_NAMESPACE_USING_DIRECTIVE

std::atomic<int> threadStarted;
std::atomic<int> colCleared;

void TestScopeFunc()
{
    TfErrorMark mark;
    {
        TRACE_FUNCTION();
        // Signal the function has started
        threadStarted.store(1);
        //Spin until the collector has been cleared
        while (!colCleared.load()) {
        }
    }
    TF_AXIOM(mark.IsClean());
}

void TestBeginEndFunc()
{
    TfErrorMark mark;
    TraceCollector::GetInstance().BeginEvent("Test Func");
    // Signal the function has started
    threadStarted.store(1);
    //Spin until the collector has been cleared
    while (!colCleared.load()) {
    }

    TraceCollector::GetInstance().EndEvent("Test Func");
    TF_AXIOM(mark.IsClean());
}

void TestThreading(std::function<void(void)> callable, bool startCollecting)
 {
    using TestFunc = void(*)();
    TestFunc funcs[] = { &TestScopeFunc, &TestBeginEndFunc};
    for (auto testFunc : funcs) {
        TfErrorMark mark;
        threadStarted.store(0);
        colCleared.store(0);

        TraceCollector* _col = &TraceCollector::GetInstance();
        _col->SetEnabled(false);
        _col->Clear();
        TraceReporter::GetGlobalReporter()->ClearTree();
        _col->SetEnabled(startCollecting);

        std::thread testThread(testFunc);

        // Spin until the thread has begun the trace scope
        while (!threadStarted.load()) {}

        // Clear the collector and signal the thread
        callable();
        colCleared.store(1);

        testThread.join(); 

        TraceReporter::GetGlobalReporter()->Report(std::cout, 1);
        TF_AXIOM(mark.IsClean());
    }
 }


int
main(int argc, char *argv[])
{
    TraceCollector* _col = &TraceCollector::GetInstance();
    TraceReporterPtr _reporter = 
        TraceReporter::GetGlobalReporter();

    std::cout << "Testing TraceCollector::Enable" << std::endl;
    TestThreading([&]() { _col->SetEnabled(true);}, false);
    std::cout << "  Passed" << std::endl;

    std::cout << "Testing TraceCollector::Enable" << std::endl;
    TestThreading([&]() { _col->SetEnabled(false);}, true);
    std::cout << "  Passed" << std::endl;

    std::cout << "Testing TraceCollector::Clear" << std::endl;
    TestThreading([&]() { _col->Clear();}, true);
    std::cout << "  Passed" << std::endl;

    std::cout << "Testing TraceReporter::Report" << std::endl;
    TestThreading([&]() { _reporter->Report(std::cout, 1);}, true);
    std::cout << "  Passed" << std::endl;

    std::cout << "Testing None" << std::endl;
    TestThreading([]() {}, true);
    std::cout << "  Passed" << std::endl;

    return 0;
}