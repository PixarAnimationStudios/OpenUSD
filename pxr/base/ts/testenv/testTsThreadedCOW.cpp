//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/spline.h"

#include "pxr/base/vt/value.h"
#include "pxr/base/tf/diagnosticLite.h"

#include <thread>
#include <cstdlib>
#include <vector>
#include <functional>
#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

using TestFunction = std::function<void()>;


// Execute a function which returns a T and verify
// that the returned value is equal to expectedResult
template <typename T>
void ExecuteAndCompare(
    const std::function<T()> &function,
    const T &expectedResult)
{
    T result = function();
    TF_AXIOM(result == expectedResult);
}

// Set a keyframe to val at time.
TsSpline SetKeyFrame(TsSpline source, double time, double val) {
    source.SetKeyFrame(TsKeyFrame(time,val));
    return source;
}

// Create a TestFunction which sets some value at some time
TestFunction CreateSetKeyFrameTest(const TsSpline &baseSpline)
{
    double time = rand() % 100;
    double value = rand()/((double)RAND_MAX);
    std::function<TsSpline()> f = 
        std::bind(SetKeyFrame,baseSpline,time,value);
    return std::bind(ExecuteAndCompare<TsSpline>, f, f());
}

VtValue Eval(TsSpline source, double time)
{
    return source.Eval(time);
}

TestFunction CreateEvalTest(const TsSpline &baseSpline)
{
    // Evaluate somewhere between 0 and 10
    double time = 10.0*(rand()/(double)RAND_MAX);
    std::function<VtValue()> f = std::bind(Eval, baseSpline, time);
    return std::bind(ExecuteAndCompare<VtValue>, f, f());
}

void RunTests(const std::vector<TestFunction> &tests, size_t iterations)
{
    const std::thread::id id = std::this_thread::get_id();
    std::cout << "Running " << iterations
              << " tests in thread " << id << std::endl;

    for (size_t i=0; i < iterations; i++) {
        tests[i%tests.size()]();
    }

    std::cout << "Done running tests in thread " << id << std::endl;
}

int main() {
    // Set up a base spline with several key frames already in it
    // All the operations will be on objects that share a COW representation
    // with this spline.
    TsSpline baseSpline;
    baseSpline.SetKeyFrame(TsKeyFrame(1,1.));
    baseSpline.SetKeyFrame(TsKeyFrame(5,5.));
    baseSpline.SetKeyFrame(TsKeyFrame(10,10.));

    // Set up a bunch of tests and their expected results
    std::vector<TestFunction> tests;
    for (int i=0; i < 10; i++) {
        tests.push_back(CreateSetKeyFrameTest(baseSpline));
        tests.push_back(CreateEvalTest(baseSpline));
    }

    // Create some threads which will each execute the tests many times
    size_t numIterations = 100000;
    std::vector<std::thread> threads;
    for (size_t i=0; i < 8; i++) {
        threads.emplace_back(std::bind(RunTests, tests, numIterations));
    }

    // Wait for all the test threads to complete
    for (std::thread &thread : threads) {
        thread.join();
    }
    std::cout << "PASSED" << std::endl;
    return 0;
}
