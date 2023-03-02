//
// Copyright 2022 Pixar
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

#include "pxr/pxr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/bigRWMutex.h"
#include "pxr/base/tf/spinRWMutex.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/stopwatch.h"

#include <tbb/spin_rw_mutex.h>
#include <tbb/queuing_rw_mutex.h>

PXR_NAMESPACE_USING_DIRECTIVE

struct BigRW
{
    static constexpr const char *Label = "TfBigRwMutex";
    using MutexType = TfBigRWMutex;
    using LockType = TfBigRWMutex::ScopedLock;
};

struct SpinRW
{
    static constexpr const char *Label = "TfSpinRWMutex";
    using MutexType = TfSpinRWMutex;
    using LockType = TfSpinRWMutex::ScopedLock;
};

struct TbbSpinRW
{
    static constexpr const char *Label = "tbb::spin_rw_mutex";
    using MutexType = tbb::spin_rw_mutex;
    using LockType = tbb::spin_rw_mutex::scoped_lock;
};

struct TbbQRW
{
    static constexpr const char *Label = "tbb::queuing_rw_mutex";
    using MutexType = tbb::queuing_rw_mutex;
    using LockType = tbb::queuing_rw_mutex::scoped_lock;
};


template <class Mutex>
static void
Test_RWMutexThroughput()
{
    int value = 0;
    const float numSeconds = 2.0f;

    typename Mutex::MutexType mutex;

    // Make a bunch of threads that mostly read, but occasionally write.

    const int numThreads = std::thread::hardware_concurrency()-1;
    std::vector<std::thread> threads(numThreads);
    for (auto &t: threads) {
        t = std::thread([&mutex, &value, numSeconds]() {
            size_t sum = 0;
            size_t niters = 0;
            TfStopwatch stopwatch;
            do {
                stopwatch.Start();
                for (int i = 0; i != 1024; ++i) {
                    // Read the value.
                    typename Mutex::LockType lock(mutex, /*write=*/false);
                    sum += value;
                }
                // Increment the value.
                {
                    typename Mutex::LockType lock(mutex, /*write=*/true);
                    ++value;
                }
                stopwatch.Stop();
                ++niters;
            } while(stopwatch.GetSeconds() < numSeconds);
            printf("%s: %zu iters in %.3f seconds (%.1f/sec), summed to %zu\n",
                   Mutex::Label,
                   niters, stopwatch.GetSeconds(),
                   (double)niters/stopwatch.GetSeconds(), sum);
        });
    }
    
    for (std::thread &t: threads) {
        t.join();
    }

    printf("%s: final value = %d\n", Mutex::Label, value);

}

static bool
Test_TfRWMutexes()
{
    Test_RWMutexThroughput<SpinRW>();
    Test_RWMutexThroughput<BigRW>();
    Test_RWMutexThroughput<TbbSpinRW>();
    Test_RWMutexThroughput<TbbQRW>();
    return true;
}

TF_ADD_REGTEST(TfRWMutexes);
