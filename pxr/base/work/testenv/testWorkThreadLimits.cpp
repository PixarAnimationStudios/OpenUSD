//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/threadLimits.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/getenv.h"
#include "pxr/base/tf/staticData.h"

#include <algorithm>
#include <functional>

#include <cstdlib>
#include <iostream>
#include <mutex>
#include <set>
#include <thread>

using namespace std::placeholders;

PXR_NAMESPACE_USING_DIRECTIVE

static TfStaticData< std::set<std::thread::id> > _uniqueThreads;
static TfStaticData< std::mutex > _uniqueThreadsMutex;

static void
_CountThreads(size_t begin, size_t end)
{
    // Do something to take up some time
    for (size_t i = begin; i < end; ++i) {
        srand(rand() * rand() * rand() * rand());
    }
    std::lock_guard<std::mutex> lock(*_uniqueThreadsMutex);
    _uniqueThreads->insert(std::this_thread::get_id());
}

static size_t
_ExpectedLimit(const int envVal, const size_t n)
{
    // If envVal is non-zero, it wins over n!
    // envVal may also be a negative number, which means all but that many
    // cores in a granular thread limiting implementation. If the implementation
    // is non granular then envVal will limit to the max physical concurrency if 
    // it's value has not been set to 1. 
    if (WorkSupportsGranularThreadLimits()) {
        const size_t val = envVal ? 
        (envVal < 0 ?
            std::max<int>(1, envVal+WorkGetPhysicalConcurrencyLimit()) : envVal)
        : n;
        return val;
    }
    const size_t val = envVal ? 
        (envVal == 1 ?
            envVal : WorkGetPhysicalConcurrencyLimit())
        : n;
    return val;
}

static void
_TestThreadLimit(const int envVal, const size_t n)
{
    const size_t expectedN = _ExpectedLimit(envVal, n);
    if (expectedN != n) {
        std::cout << "   env setting overrides n = " << n << "\n";
    }

    const size_t numSamples = 1000000;
    std::cout << "   expecting maximum " << expectedN << " threads\n";

    _uniqueThreads->clear();

    WorkParallelForN(numSamples, std::bind(&_CountThreads, _1, _2));

    std::cout << "   Used " << _uniqueThreads->size() << '\n';

    if (_uniqueThreads->size() > expectedN) {
        TF_FATAL_ERROR("Expected less than or equal to %zu threads, got %zu",
                       expectedN, _uniqueThreads->size());
    }

}

// When the thread limit is greater than 1, a non granular implementation should
// default to physical concurrency whereas a granular implementation
// should support some amount of concurrency.
static bool
_IsValidLimit(const int limit, const int expected)
{
    if (WorkSupportsGranularThreadLimits()) {
        return limit <= expected;
    }
    return limit == expected;

}

static void
_TestArguments(const int envVal)
{
    // Note that if envVal is set (i.e. non-zero) it will always win over the
    // value supplied through the API calls.

    // Set to maximum concurrency, which should remain within envVal.
    const int numCores = WorkGetPhysicalConcurrencyLimit();
    WorkSetConcurrencyLimitArgument(numCores);
    TF_AXIOM(WorkGetConcurrencyLimit() == _ExpectedLimit(envVal, numCores));

    // n = 0, means "no change"
    WorkSetConcurrencyLimitArgument(0);
    TF_AXIOM(WorkGetConcurrencyLimit() == _ExpectedLimit(envVal, numCores));

    // n = 1 means no threading
    WorkSetConcurrencyLimitArgument(1);
    TF_AXIOM(WorkGetConcurrencyLimit() == _ExpectedLimit(envVal, 1));

    // n = 3 means 3
    WorkSetConcurrencyLimitArgument(3);
    TF_AXIOM(_IsValidLimit(WorkGetConcurrencyLimit(), 
        _ExpectedLimit(envVal, numCores)));

    // n = 1000 means 1000
    WorkSetConcurrencyLimitArgument(1000);
    if(!WorkSupportsGranularThreadLimits()) {
        TF_AXIOM(WorkGetConcurrencyLimit() == _ExpectedLimit(envVal, numCores));
    } else {
        TF_AXIOM(WorkGetConcurrencyLimit() <= _ExpectedLimit(envVal, 1000));
    }

    // n = -1 means numCores - 1, with a minimum of 1
    WorkSetConcurrencyLimitArgument(-1);
    TF_AXIOM(_IsValidLimit(WorkGetConcurrencyLimit(), 
            _ExpectedLimit(envVal, std::max(1, numCores))));

    // n = -3 means numCores - 3, with a minimum of 1
    WorkSetConcurrencyLimitArgument(-3);
    TF_AXIOM(_IsValidLimit(WorkGetConcurrencyLimit(), 
            _ExpectedLimit(envVal, std::max(1, numCores))));

    // n = -numCores means 1 (no threading)
    WorkSetConcurrencyLimitArgument(-numCores);
    TF_AXIOM(WorkGetConcurrencyLimit() == _ExpectedLimit(envVal, 1));

    // n = -numCores*10 means 1 (no threading)
    WorkSetConcurrencyLimitArgument(-numCores*10);
    TF_AXIOM(WorkGetConcurrencyLimit() == _ExpectedLimit(envVal, 1));
}

int
main(int argc, char **argv)
{
    // Read the env setting used to limit threading
    const int envVal = TfGetenvInt("PXR_WORK_THREAD_LIMIT", 0);
    std::cout << "PXR_WORK_THREAD_LIMIT = " << envVal << '\n';
    
    // 0 means all cores.
    if (envVal == 0) {
        WorkSetMaximumConcurrencyLimit();
    }
    const size_t limit = WorkGetConcurrencyLimit();
    const int numCores = WorkGetPhysicalConcurrencyLimit();

    // Make sure that we get the default thread limit
    std::cout << "Testing that the thread limit defaults to "
        "PXR_WORK_THREAD_LIMIT by default...\n";
    _TestThreadLimit(envVal, limit);


    // Test with full concurrency.
    std::cout << "Testing full concurrency...\n";
    WorkSetMaximumConcurrencyLimit();
    TF_AXIOM(WorkGetConcurrencyLimit() ==
        _ExpectedLimit(envVal, numCores));
    _TestThreadLimit(envVal, numCores);

    // Test with no concurrency.
    std::cout << "Testing turning off concurrency...\n";
    WorkSetConcurrencyLimit(1);
    TF_AXIOM(WorkGetConcurrencyLimit() ==
        _ExpectedLimit(envVal, 1));
    _TestThreadLimit(envVal, 1);

    // Max value for thread limits dependant on if the thread limit has been set
    // higher than the number of cores available.
    size_t upperBoundLimit;

    // Test with 2 threads.
    std::cout << "Testing with 2 threads...\n";
    WorkSetConcurrencyLimit(2);
    upperBoundLimit = std::max(2, numCores);
    if(WorkSupportsGranularThreadLimits()) {
        TF_AXIOM(WorkGetConcurrencyLimit() <=
            _ExpectedLimit(envVal, upperBoundLimit));
        _TestThreadLimit(envVal, upperBoundLimit);
    } else {
        TF_AXIOM(WorkGetConcurrencyLimit() ==
            _ExpectedLimit(envVal, numCores));
        _TestThreadLimit(envVal, numCores);
    }

    // Test with 4 threads.
    std::cout << "Testing with 4 threads...\n";
    WorkSetConcurrencyLimit(4);
    upperBoundLimit = std::max(4, numCores);
    if(WorkSupportsGranularThreadLimits()) {
        TF_AXIOM(WorkGetConcurrencyLimit() <=
            _ExpectedLimit(envVal, upperBoundLimit));
        _TestThreadLimit(envVal, upperBoundLimit);
    } else {
        TF_AXIOM(WorkGetConcurrencyLimit() ==
            _ExpectedLimit(envVal, numCores));
        _TestThreadLimit(envVal, numCores);
    }

    // Test with 1000 threads.
    std::cout << "Testing with 1000 threads...\n";
    WorkSetConcurrencyLimit(1000);
    upperBoundLimit = std::max(1000, numCores);
    if(WorkSupportsGranularThreadLimits()) {
        TF_AXIOM(WorkGetConcurrencyLimit() <=
            _ExpectedLimit(envVal, upperBoundLimit));
        _TestThreadLimit(envVal, upperBoundLimit);
    } else {
        TF_AXIOM(WorkGetConcurrencyLimit() ==
            _ExpectedLimit(envVal, numCores));
        _TestThreadLimit(envVal, numCores);
    }

    // Test argument parsing
    std::cout << "Testing argument parsing...\n";
    _TestArguments(envVal);
    return 0;
}
