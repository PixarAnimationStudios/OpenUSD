//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/work/loops.h"

#include "pxr/base/work/threadLimits.h"

#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/arch/fileSystem.h"

#include <tbb/concurrent_vector.h>
#include <tbb/concurrent_unordered_set.h>
#include <functional>

#include <cstdio>
#include <cstring>
#include <numeric>
#include <iostream>
#include <thread>
#include <vector>

using namespace std::placeholders;

PXR_NAMESPACE_USING_DIRECTIVE

static void
_Double(size_t begin, size_t end, std::vector<int> *v)
{
    for (size_t i = begin; i < end; ++i)
        (*v)[i] *= 2;
}

static void
_DoubleAll(std::vector<int> &v)
{
    for (int &i : v) {
        i *= 2;
    }
}

template < typename Container>
static void
_VerifyDoubled(const Container &v)
{
    for (size_t i = 0; i < v.size(); ++i) {
        if (static_cast<size_t>(v[i]) != (2*i)) {
            std::cout << "found error at index " << i << " is " 
                      << v[i] << std::endl;
            TF_AXIOM(static_cast<size_t>(v[i]) == (2*i));
        }
    }
}

static void
_PopulateVector(size_t arraySize, std::vector<int> *v)
{
    v->resize(arraySize);
    std::iota(v->begin(), v->end(), 0);
}

// Returns the number of seconds it took to complete this operation.
double
_DoForNTest(bool verify, const size_t arraySize, const size_t numIterations)
{
    std::vector<int> v;
    _PopulateVector(arraySize, &v);

    TfStopwatch sw;
    sw.Start();
    for (size_t i = 0; i < numIterations; i++) {

        WorkParallelForN(arraySize, std::bind(&_Double, _1, _2, &v));       

    }

    if (verify) {
        TF_AXIOM(numIterations == 1);
        _VerifyDoubled(v);
    }

    sw.Stop();
    return sw.GetSeconds();
}


// Returns the number of seconds it took to complete this operation.
double
_DoForEachTest(
    bool verify, const size_t arraySize, const size_t numIterations)
{
    static const size_t partitionSize = 20;
    std::vector< std::vector<int> > vs(partitionSize);
    for (std::vector<int> &v : vs) {
        _PopulateVector(arraySize / partitionSize, &v);
    }

    TfStopwatch sw;
    sw.Start();
    for (size_t i = 0; i < numIterations; i++) {

        WorkParallelForEach(vs.begin(), vs.end(), _DoubleAll);

    }

    if (verify) {
        TF_AXIOM(numIterations == 1);
        for (const auto& v : vs) {
            _VerifyDoubled(v);
        }
    }

    sw.Stop();
    return sw.GetSeconds();
}

// Returns the number of seconds it took to complete this operation.
double
_DoForTBBRangeTest(bool verify, const size_t arraySize, 
                   const size_t numIterations)
{
    tbb::concurrent_vector<int> v(arraySize);
    std::iota(v.begin(), v.end(), 0);

    TfStopwatch sw;
    sw.Start();
    for (size_t i = 0; i < numIterations; i++) {

        WorkParallelForTBBRange(v.range(), 
            [&](const tbb::concurrent_vector<int>::range_type & range) {
                for (auto it = range.begin(); it != range.end(); ++it) {
                    int val = *it;
                    *it = val*2;
                }
        });       
    }

    if (verify) {
        TF_AXIOM(numIterations == 1);
        _VerifyDoubled(v);
    }

    sw.Stop();
    return sw.GetSeconds();
}


// Returns the number of seconds it took to complete this operation.
double
_DoErrorTest(bool verify, const size_t numIterations)
{
    const size_t arraySize = 100;
    tbb::concurrent_unordered_set<std::thread::id,
                                  std::hash<std::thread::id>> threadIds;
    std::vector<int> v;
    _PopulateVector(arraySize, &v);
    
    TfErrorMark m;

    TfStopwatch sw;
    sw.Start();
    for (size_t i = 0; i < numIterations; i++) {

        WorkParallelForN(arraySize, [&](size_t begin, size_t end){
            threadIds.insert(std::this_thread::get_id());
            TF_RUNTIME_ERROR("Cross-thread transfer test error");
        });

        if (threadIds.size() < 2) {
            TF_WARN("ParallelFor only executed with one worker. All errors are "
                    "by default posted to the main thread's error list.");
        }
    }

    if (verify) {
        TF_AXIOM(numIterations == 1);
        TF_AXIOM(!m.IsClean() && 
        (static_cast<size_t>(std::distance(m.begin(),m.end())) == arraySize));
    }

    sw.Stop();
    return sw.GetSeconds();
}

void
_DoSerialTest()
{
    const size_t N = 200;
    std::vector<int> v;
    _PopulateVector(N, &v);
    WorkSerialForN(N, std::bind(&_Double, _1, _2, &v));
    _VerifyDoubled(v);
}

// Make sure that the API for WorkParallelForN and WorkSerialForN can be
// interchanged.  
void
_DoSignatureTest()
{
    struct F
    {
        // Test that this can be non-const
        void operator()(size_t start, size_t end) {
        }
    };

    F f;

    WorkParallelForN(100, f);
    WorkSerialForN(100, f);

    WorkParallelForN(100, F());
    WorkSerialForN(100, F());
}


int
main(int argc, char **argv)
{
    const bool perfMode = ((argc > 1) && !strcmp(argv[1], "--perf")); 
    const size_t arraySize = 1000000;
    const size_t numIterations = perfMode ? 1000 : 1;

    WorkSetMaximumConcurrencyLimit();

    std::cout << "Initialized with " << 
        WorkGetPhysicalConcurrencyLimit() << " cores..." << std::endl;


    double forNSeconds = _DoForNTest(!perfMode, arraySize, numIterations);

    std::cout << " parallel_for took: " << forNSeconds << " seconds" 
        << std::endl;


    double forEachSeconds = _DoForEachTest(!perfMode, arraySize, numIterations);

    std::cout << " parallel_for_each took: " << forEachSeconds
        << " seconds" << std::endl;

    double forTbbRangeSeconds = _DoForTBBRangeTest(
        !perfMode, arraySize, numIterations);

    std::cout << "parallel_for_tbb_range tests took: " << forTbbRangeSeconds
        << " seconds" << std::endl;

    double errorSeconds = _DoErrorTest(false, numIterations);

    std::cout << "handling_errors tests took: " << errorSeconds
        << " seconds" << std::endl;


    _DoSerialTest();

    _DoSignatureTest();

    if (perfMode) {

        // XXX:perfgen only accepts metric names ending in _time.  See bug 97317
        FILE *outputFile = ArchOpenFile("perfstats.raw", "w");
        fprintf(outputFile,
            "{'profile':'Loops_time','metric':'time','value':%f,'samples':1}\n",
            forNSeconds);
        fprintf(outputFile,
            "{'profile':'for_each Loops_time','metric':'time','value':%f,'samples':1}\n",
            forEachSeconds);
        fprintf(outputFile,
            "{'profile':'for_tbb_range Loops_time','metric':'time','value':%f,'samples':1}\n",
            forTbbRangeSeconds);
        fprintf(outputFile,
            "{'profile':'error_handling_time','metric':'time','value':%f,'samples':1}\n",
            errorSeconds);
        fclose(outputFile);

    }

    return 0;
}
