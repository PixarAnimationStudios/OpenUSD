//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/regTest.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/diagnosticLite.h"

#include <atomic>
#include <chrono>
#include <thread>


PXR_NAMESPACE_USING_DIRECTIVE

using namespace std::chrono_literals;

std::atomic<uint32_t> countA = 0, countB = 0;

// The desired behavior for SubScribeTo in a multi-threaded situation is that it
// should not return before all registration functions in its category have
// completed.
// The tests in the ThreadA category purposely introduce some sleep time in
// order to engineer the case where simultaneous calls to SubscribeTo work off
// registrations from the queue at different rates.  Thread B functions will
// complete much quicker than thread A, but should not cause ThreadA's
// SubscribeTo invocation it to return before CountA is 2.

class ThreadA{};

TF_REGISTRY_FUNCTION(ThreadA) {
    std::this_thread::sleep_for(25ms);
    countA +=1;
}

TF_REGISTRY_FUNCTION(ThreadA) {
    std::this_thread::sleep_for(50ms);
    countA +=1;
}

class ThreadB {};

TF_REGISTRY_FUNCTION(ThreadB) {
    countB +=1;
}

TF_REGISTRY_FUNCTION(ThreadB) {
    countB +=1;
}


static bool
Test_TfRegistryManagerMultithreaded()
{
    std::thread threadA([]() {
        TfRegistryManager::GetInstance().SubscribeTo<ThreadA>();
        TF_AXIOM(countA == 2);
    });

    std::thread threadB([]() {
        TfRegistryManager::GetInstance().SubscribeTo<ThreadB>();
        TF_AXIOM(countB == 2);
    });

    threadA.join();
    threadB.join();

    return true;
}

TF_ADD_REGTEST(TfRegistryManagerMultithreaded);
