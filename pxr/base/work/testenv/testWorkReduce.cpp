//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/work/reduce.h"

#include "pxr/base/work/threadLimits.h"

#include "pxr/base/tf/stopwatch.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/arch/fileSystem.h"

#include <functional>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

using namespace std::placeholders;

PXR_NAMESPACE_USING_DIRECTIVE

static int
sum(size_t begin, size_t end, int val, const std::vector<int>& v)
{
    for (size_t i = begin; i < end; ++i)
        val += v[i];
    return val;
}


static int
plus(int lhs, int rhs)
{
    return lhs + rhs;
}


static void
_PopulateVector(size_t arraySize, std::vector<int> &v)
{
    v.clear();
    v.reserve(arraySize);
    for (size_t i = 0; i < arraySize; ++i) {
        v.push_back(i);
    }
}

// Returns the number of seconds it took to complete this operation.
double
_DoTBBTest(bool verify, const int arraySize, const size_t numIterations)
{
    std::vector<int> v;
    _PopulateVector(arraySize, v);

    TfStopwatch sw;
    sw.Start();
    int res = 0;
    for (size_t i = 0; i < numIterations; i++) {
         res = WorkParallelReduceN(0,
            arraySize,
            std::bind(&sum, _1, _2, _3, v),
            std::bind(&plus, _1, _2));
    }

    if (verify) {
        TF_AXIOM(numIterations == 1);
        TF_AXIOM(res = arraySize*(arraySize-1)/2);
    }

    sw.Stop();
    return sw.GetSeconds();
}

// Make sure that the API for WorkParallelReduceN can be
// interchanged.  
void
_DoSignatureTest()
{
    struct F
    {
        // Test that this can be non-const
        int operator()(size_t start, size_t end, int val) {
            return val;
        }
    };

    F f;

    struct B
    {
        // The reduction operator has to be const
        int operator()(int lhs, int rhs) const {
            return lhs + rhs;
        }
    };

    B b;
    int initial = 0;
    WorkParallelReduceN(initial, 100, f, b);

    WorkParallelReduceN(initial, 100, F(), B());
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

    double tbbSeconds = _DoTBBTest(!perfMode, arraySize, numIterations);

    std::cout << "TBB parallel_reduce.h took: " << tbbSeconds << " seconds" 
        << std::endl;

    _DoSignatureTest();

    if (perfMode) {

        // XXX:perfgen only accepts metric names ending in _time.  See bug 97317
        FILE *outputFile = ArchOpenFile("perfstats.raw", "w");
        fprintf(outputFile,
        "{'profile':'TBB Reduce_time','metric':'time','value':%f,'samples':1}\n",
            tbbSeconds);        
        fclose(outputFile);

    }

    return 0;
}
