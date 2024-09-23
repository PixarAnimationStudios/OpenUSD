//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/base/work/sort.h"
#include "pxr/base/work/threadLimits.h"

#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stopwatch.h"

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

static void
_PopulateVector(size_t arraySize, std::vector<int>* v)
{
    std::srand(std::time(NULL));
    v->clear();
    v->reserve(arraySize);
    for (size_t i = 0; i < arraySize; ++i) {
        v->push_back(std::rand());
    }
}

// Returns the number of seconds it took to complete this operation.
double
_DoTBBTest(const size_t arraySize, const size_t numIterations)
{
    std::vector<int> v;
    _PopulateVector(arraySize, &v);
    std::vector<int> save = v;

    TfStopwatch sw;
    sw.Start();
    std::vector<int> filterv;
    for (size_t i = 0; i < numIterations; i++) {
        v = save;
         WorkParallelSort(&v);
    }

    TF_AXIOM(numIterations == 1);
    for(unsigned int i = 1; i < v.size(); ++i){
        TF_AXIOM(v[i-1] <= v[i]);
    }

    sw.Stop();
    return sw.GetSeconds();
}


int
main(int argc, char **argv)
{
    const size_t arraySize = 1000000;
    const size_t numIterations = 1;

    WorkSetMaximumConcurrencyLimit();

    std::cout << "Initialized with " << 
        WorkGetPhysicalConcurrencyLimit() << " cores..." << std::endl;

    const double tbbSeconds = _DoTBBTest(arraySize, numIterations);

    std::cout << "TBB parallel_sort.h took: " << tbbSeconds << " seconds" 
        << std::endl;

    return 0;
}
