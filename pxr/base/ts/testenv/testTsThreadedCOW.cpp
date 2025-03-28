//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/ts/spline.h"
#include "pxr/base/tf/diagnosticLite.h"

#include <thread>
#include <vector>
#include <functional>
#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

using TestFunction = std::function<void()>;


// This test takes a single starting spline, then (repeatedly and in parallel)
// makes copies of the spline object, and either modifies them or evaluates
// them.  Modifications should always invoke copy-on-write behavior, so the
// evaluation steps should always get the same results, which use the original,
// unmodified spline data.


template <typename T>
void ExecuteAndCompare(
    const std::function<T()> &function,
    const T &expectedResult)
{
    T result = function();
    TF_AXIOM(result == expectedResult);
}

TsSpline SetKnot(TsSpline source, double time, double val) {
    TsKnot knot;
    knot.SetTime(time);
    knot.SetValue(val);
    source.SetKnot(knot);
    return source;
}

TestFunction CreateSetKnotTest(const TsSpline &baseSpline)
{
    // Choose some randomized arguments.
    double time = rand() % 100;
    double value = rand() / ((double) RAND_MAX);

    // Create a closure that will add a knot with the randomized arguments.
    // This closure contains a copy of the baseSpline object, and when the
    // closure is invoked, an additional copy will be made to pass to the spline
    // parameter of the SetKnot function.  This should mean that neither the
    // original spline, nor the copy stored in the closure, will be modified
    // when SetKnot is called.
    std::function<TsSpline()> f = 
        std::bind(SetKnot, baseSpline, time, value);

    // Call the closure once now, make a copy of its result (the modified
    // spline), and store that copy in a second closure.  Each time this step is
    // run, the inner closure will be called again, and the result will be
    // checked against the result copy, verifying that the same modification is
    // taking place each time.
    TsSpline result = f();
    TF_AXIOM(result != baseSpline);
    return std::bind(ExecuteAndCompare<TsSpline>, f, result);
}

double Eval(TsSpline source, double time)
{
    double result = 0;
    source.Eval(time, &result);
    return result;
}

TestFunction CreateEvalTest(const TsSpline &baseSpline)
{
    // Choose a randomized time argument.
    double time = 10.0 * (rand() / (double) RAND_MAX);

    // Create a closure that will evaluate the spline at the randomized time.
    std::function<double()> f = std::bind(Eval, baseSpline, time);

    // Call the closure once now, make a copy of its result (the evaluated
    // value), and store that copy in a second closure.  Each time this step is
    // run, the inner closure will be called again, and the result will be
    // checked against the result copy, verifying that the spline data we are
    // evaluating has not been affected by the modifications that are taking
    // place in the SetKnotTest steps.
    double result = f();
    return std::bind(ExecuteAndCompare<double>, f, result);
}

void RunTests(const std::vector<TestFunction> &tests, size_t iterations)
{
    const std::thread::id id = std::this_thread::get_id();
    std::cout << "Running " << iterations
              << " tests in thread " << id << std::endl;

    for (size_t i = 0; i < iterations; i++) {
        tests[i % tests.size()]();
    }

    std::cout << "Done running tests in thread " << id << std::endl;
}

void AddKnot(TsSpline *spline, double time, double value)
{
    TsKnot knot;
    knot.SetTime(time);
    knot.SetValue(value);
    spline->SetKnot(knot);
}

int main()
{
    TsSpline baseSpline;
    AddKnot(&baseSpline, 1.0, 1.0);
    AddKnot(&baseSpline, 5.0, 5.0);
    AddKnot(&baseSpline, 10.0, 10.0);

    // We create:
    // - 10 randomized modification steps.
    // - 10 randomized evaluation steps, interleaved with the modifications.
    // - 8 threads that all run the same steps in parallel and out of sync.
    // - 100,000 iterations per thread, cycling through the 20 steps.

    // Create steps.
    std::vector<TestFunction> tests;
    for (int i = 0; i < 10; i++) {
        tests.push_back(CreateSetKnotTest(baseSpline));
        tests.push_back(CreateEvalTest(baseSpline));
    }

    // Start threads.
    size_t numIterations = 100'000;
    std::vector<std::thread> threads;
    for (size_t i = 0; i < 8; i++) {
        threads.emplace_back(std::bind(RunTests, tests, numIterations));
    }

    // Wait for all threads to complete.
    for (std::thread &thread : threads) {
        thread.join();
    }

    std::cout << "PASSED" << std::endl;
    return 0;
}
