//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/base/work/loops.h"
#include "pxr/base/work/threadLimits.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/staticData.h"

#include <boost/bind.hpp>

#include <iostream>
#include <mutex>
#include <pthread.h>
#include <set>
#include <stdio.h>


extern TfEnvSetting<int> PXR_WORK_THREAD_LIMIT;

static TfStaticData< std::set<pthread_t> > _uniqueThreads;
static TfStaticData< std::mutex > _uniqueThreadsMutex;

static void
_CountThreads(size_t begin, size_t end)
{
    // Do something to take up some time
    for (size_t i = begin; i < end; ++i) {
        int *a = new int[i];
        delete [] a;
    }
    std::lock_guard<std::mutex> lock(*_uniqueThreadsMutex);
    _uniqueThreads->insert(pthread_self());
}

static void
_TestThreadLimit(size_t n)
{
    const size_t numSamples = 1000000;
    std::cout << "   maximum " << n << " threads\n";

    _uniqueThreads->clear();

    WorkParallelForN(numSamples, boost::bind(&_CountThreads, _1, _2));

    std::cout << "   TBB used " << _uniqueThreads->size() << '\n';

    if (_uniqueThreads->size() > n) {
        TF_FATAL_ERROR("TBB expected less than or equal to %zu threads, got %zu",
                       n, _uniqueThreads->size());
    }

}

static void
_TestArguments()
{
    size_t numCores = WorkGetMaximumConcurrencyLimit();

    // These tests assume we have at least 4 cores.
    TF_AXIOM(numCores >= 4);

    // n = 0, means full threading
    WorkSetConcurrencyLimitArgument(0);
    TF_AXIOM(WorkGetConcurrencyLimit() == numCores);

    // n = 1 means no threading
    WorkSetConcurrencyLimitArgument(1);
    TF_AXIOM(WorkGetConcurrencyLimit() == 1);

    // n = 3 means 3
    WorkSetConcurrencyLimitArgument(3);
    TF_AXIOM(WorkGetConcurrencyLimit() == 3);

    // n = numCores means numCores
    WorkSetConcurrencyLimitArgument(numCores);
    TF_AXIOM(WorkGetConcurrencyLimit() == numCores);

    // n = 1000 means max
    WorkSetConcurrencyLimitArgument(1000);
    TF_AXIOM(WorkGetConcurrencyLimit() == numCores);

    // n = -1 means numCores - 1
    WorkSetConcurrencyLimitArgument(-1);
    TF_AXIOM(WorkGetConcurrencyLimit() == numCores-1);

    // n = -3 means numCores - 3
    WorkSetConcurrencyLimitArgument(-3);
    TF_AXIOM(WorkGetConcurrencyLimit() == numCores-3);

    // n = -numCores means 1 (no threading)
    WorkSetConcurrencyLimitArgument(-numCores);
    TF_AXIOM(WorkGetConcurrencyLimit() == 1);

    // n = -numCores*10 means 1 (no threading)
    WorkSetConcurrencyLimitArgument(-numCores*10);
    TF_AXIOM(WorkGetConcurrencyLimit() == 1);
}

struct _RawTBBCounter
{
    void operator()(const tbb::blocked_range<size_t> &r) const {
        _CountThreads(r.begin(), r.end());
    }
};

int
main(int argc, char **argv)
{
    // Read the env setting used to limit threading
    size_t limit = TfGetEnvSetting(PXR_WORK_THREAD_LIMIT);
    std::cout << "PXR_WORK_THREAD_LIMIT = " << limit << '\n';

    // Test to make sure that a call to tbb that happens before any of the
    // libWork API is touched is unrestricted.  We need to do this for now to
    // make sure that we don't break existing tbb code just by having libWork
    // linked in.
    //
    // Note that if we test this, we can't run the rest of the test because once
    // tbb is initialized by default (by just using its API) then there doesn't
    // seem to be a way to limit it again.  That's why we test this
    // functionality by itself.
    if ((argc == 2) and (strcmp(argv[1], "--rawtbb") == 0)) {
        TF_AXIOM(WorkGetMaximumConcurrencyLimit() >= 4);

        std::cout << "Testing that libWork automatically limits tbb "
            "threading when PXR_WORK_THREAD_LIMIT is set...\n";
        _uniqueThreads->clear();
        tbb::parallel_for(
            tbb::blocked_range<size_t>(0, 100000), _RawTBBCounter());
        std::cout << "   default TBB used " << _uniqueThreads->size() 
                  << " threads\n";
        
        if (limit == 0) {
            if (_uniqueThreads->size() < 2) {
                TF_FATAL_ERROR("tbb only used %zu threads when it should be "
                               "unlimited\n", _uniqueThreads->size());
            }
        }
        else if (_uniqueThreads->size() > limit) {
            TF_FATAL_ERROR("tbb used %zu threads, which is greater than "
                           "PXR_WORK_THREAD_LIMIT=%zu.", _uniqueThreads->size(),
                           limit);
        }

        // Stop the test, now that we've initialized tbb, there's no going
        // back.
        return 0;
    }

    // 0 means all cores.
    if (limit == 0) {
        WorkSetMaximumConcurrencyLimit();
        limit = WorkGetConcurrencyLimit();
    }
    
    TF_AXIOM(limit > 0 and limit <= WorkGetMaximumConcurrencyLimit());

    // Make sure that we get the default thread limit
    std::cout << "Testing that the thread limit defaults to "
        " PXR_WORK_THREAD_LIMIT by default...\n";
    _TestThreadLimit(limit);

    // Now that we've invoked libWork, make sure that raw TBB API also defaults
    // to PXR_WORK_THREAD_LIMIT.
    std::cout << "Testing that raw tbb code is now also unlimited "
        " after first invocation of libWork API...\n";

    _uniqueThreads->clear();
    tbb::parallel_for(tbb::blocked_range<size_t>(0, 100000), _RawTBBCounter());
    std::cout << "   raw tbb used " << _uniqueThreads->size() << " threads\n";
    if (_uniqueThreads->size() > limit) {
        TF_FATAL_ERROR("it appears as though libWork hasn't been initialized "
                       "with PXR_WORK_THREAD_LIMIT.");
    }

    // Test with full concurrency.
    std::cout << "Testing full concurrency...\n";
    WorkSetMaximumConcurrencyLimit();
    _TestThreadLimit(WorkGetMaximumConcurrencyLimit());

    // Test with no concurrency.
    std::cout << "Testing turning off concurrency...\n";
    WorkSetConcurrencyLimit(1);
    _TestThreadLimit(1);

    // Test with 4 threads.
    std::cout << "Testing with 4 threads...\n";
    WorkSetConcurrencyLimit(4);
    _TestThreadLimit(4);

    // Test argument parsing
    std::cout << "Testing argument parsing...\n";
    _TestArguments();
    return 0;
}
